#include <stdbool.h>
#include <efi.h>
#include <efilib.h>
#include "acpi_dump.h"
#include "bgrt.h"
#include "acpi_checksum.h"
#include "gop.h"
#include "bmp.h"
#include "nstdlib.h"
#include "xsdt.h"
#include "chainload.h"
#include "dirtool.h"

// if LOAD_WINDOWS is defined, it works under Windows loader mode, i.e. chainload Windows bootloader
// if not defined, it will quit after done everything, thus can be load as an EFI driver
#define LOAD_WINDOWS

// the path relative to ESP partition root to search for customized boot image, can have "\\" in it
#define USER_BOOT_IMAGE_PATH L"boot_image.bmp"

// the default image compiled into the program
// if user-provided image is not found, use this
const CHAR8 default_boot_image[] = {
#include "default_boot_image.bmp.inc"
};

// loads an efi executable if not a DEBUG build;
// otherwise try to read the file and verify if it is an efi executable, then quit
EFI_STATUS load_efi_image(DIRTOOL_FILE* file, EFI_HANDLE ImageHandle)
{
#if defined(_DEBUG)
	Print(L"ChainloadByDevicePath %s ignored in debug build\n", file->Path);
	CHAR8* buf = dirtool_read_file(file);
	Print(L"buf: %c%c\n", buf[0], buf[1]); // should print "MZ"
	return EFI_SUCCESS;
#else
	ClearInputBuf();
	EFI_STATUS ret = ChainloadByDevicePath(file->DevicePath, ImageHandle);
	return ret;
#endif
}

// Search this drive for user_provided boot image, load it into the memory and return the buffer.
// If not found, return NULL.
// The length of a BMP file is self-contained. This is unsafe if you load image from unknown sources.
CHAR8 *load_user_boot_image(CHAR16 *boot_image_path, EFI_HANDLE ImageHandle)
{
	CHAR8* buf = NULL;
	DIRTOOL_STATE DirToolState;
	DirToolState.initialized = 0;
	EFI_STATUS status = dirtool_init(&DirToolState, ImageHandle);
	if (EFI_ERROR(status)) {
		return NULL;
	}

	DIRTOOL_DRIVE* drive = dirtool_get_current_drive(&DirToolState, 0);
	dirtool_open_drive(&DirToolState, drive);
	DIRTOOL_FILE* pwd = dirtool_cd_multi(&(drive->RootFile), boot_image_path);
	if (pwd)
	{
		buf = dirtool_read_file(pwd);
		Print(L"%HUser provided boot image found at %s, file_size=%u%N\n", boot_image_path, pwd->FileInfo->FileSize);
		if (!bmp_sanity_check(buf, pwd->FileInfo->FileSize))
		{
			// something is wrong with the file
			free(buf);
			buf = NULL;
			Print(L"%ESanity check failed, not loading%N\n");
			pause();
		} else
		{
			Print(L"%HSanity check passed%N\n");
		}
	}
#if defined(_DEBUG)
	pause();
#endif
	dirtool_close_drive(&DirToolState, drive);
	dirtool_deinit(&DirToolState);

	return buf;
}

// Application entrypoint (must be set to 'efi_main' for gnu-efi crt0 compatibility)
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
#if defined(_GNU_EFI)
	InitializeLib(ImageHandle, SystemTable);
#endif

	Print(L"\n%HBGRTInjector%N\n");
	Print(L"%Hhttps://github.com/Jamesits/BGRTInjector%N\n");
	Print(L"Firmware %s Rev %d\n\n", SystemTable->FirmwareVendor, SystemTable->FirmwareRevision);

	// get screen size
	EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = GetGOP();
	if (!gop)
	{
		Print(L"%EUnable to get GOP screen size%N\n");
		return EFI_UNSUPPORTED;
	}

	// load image file
	bool isDefaultBootImageUsed = false;
	BMP_IMAGE_HEADER* boot_image = (BMP_IMAGE_HEADER*)load_user_boot_image(USER_BOOT_IMAGE_PATH, ImageHandle);
	if (boot_image == NULL) {
		boot_image = (BMP_IMAGE_HEADER*)&default_boot_image;
		if (!bmp_sanity_check((CHAR8*)boot_image, sizeof(default_boot_image)))
		{
			Print(L"%EDefault boot image is corrupted, please re-download BGRTInjector!%N\n");
			return EFI_UNSUPPORTED;
		}
		isDefaultBootImageUsed = true;
	}

	// calculate offset for centering the image
	INT32 offsetX_temp = (gop->Mode->Info->HorizontalResolution - boot_image->biWidth) / 2;
	INT32 offsetY_temp = (gop->Mode->Info->VerticalResolution - boot_image->biHeight) / 2;

	UINT32 offsetX = offsetX_temp > 0 ? offsetX_temp : 0;
	UINT32 offsetY = offsetY_temp > 0 ? offsetY_temp : 0;

	Print(L"Screen size: %u*%u, Image size: %d*%d (%u bytes), Top left point: %u*%u\n",
	      gop->Mode->Info->HorizontalResolution, gop->Mode->Info->VerticalResolution,
	      boot_image->biWidth, boot_image->biHeight, boot_image->bfSize,
	      offsetX, offsetY
	);

	// craft a new BGRT table
	EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE* newBgrtTable = malloc_acpi(
		sizeof(EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE));
	if (newBgrtTable == NULL)
	{
		Print(L"%EBGRT table memory allocation failed%N\n");
		free(boot_image);
		return EFI_OUT_OF_RESOURCES;
	}

	newBgrtTable->Header.Signature = 'TRGB'; // using multibyte char hence inverted
	newBgrtTable->Header.Length = sizeof(EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE);
	newBgrtTable->Header.Revision = 1;
	memcpy8(newBgrtTable->Header.OemId, (CHAR8*)"YJSNPI", 6);
	memcpy8((CHAR8*)&(newBgrtTable->Header.OemTableId), (CHAR8*)"JAMESITS", 8);
	newBgrtTable->Header.OemRevision = 1919;
	memcpy8((CHAR8*)&(newBgrtTable->Header.CreatorId), (CHAR8*)"1919", 4);
	newBgrtTable->Header.CreatorRevision = 1919;
	newBgrtTable->Version = EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE_REVISION;
	// Status: 
	// bit[0] is "if the image is currently drawn on the screen buffer".
	// 		set to 0 (INVALID) and Windows loader will draw the image itself
	// 		set to 1 (VALID) then Windows loader will retain whatever on the screen not redrawing framebuffer
	// bit[1:2] is rotation direction. Set to 1, 2, 3 for 90, 180, 270 degrees.
	newBgrtTable->Status = EFI_ACPI_5_0_BGRT_STATUS_INVALID;
	newBgrtTable->ImageType = EFI_ACPI_5_0_BGRT_IMAGE_TYPE_BMP;
	newBgrtTable->ImageOffsetX = offsetX;
	newBgrtTable->ImageOffsetY = offsetY;

	BMP_IMAGE_HEADER* newBmpImage = malloc_acpi(boot_image->bfSize);
	if (newBmpImage == NULL)
	{
		Print(L"%EImage memory allocation failed%N\n");
		free(boot_image);
		free(newBgrtTable);
		return EFI_OUT_OF_RESOURCES;
	}
	memcpy8((CHAR8*)newBmpImage, (CHAR8*)boot_image, boot_image->bfSize);
	if (!isDefaultBootImageUsed) // image file loaded, we need to free it
	{
		free(boot_image);
		boot_image = NULL;
	}
	newBgrtTable->ImageAddress = (UINT64)newBmpImage;

	// checksum
	UINT8 checksum_diff = AcpiChecksum((CHAR8*)newBgrtTable, newBgrtTable->Header.Length);
	newBgrtTable->Header.Checksum -= checksum_diff;

	EFI_CONFIGURATION_TABLE* ect = SystemTable->ConfigurationTable;
	EFI_GUID AcpiTableGuid = ACPI_TABLE_GUID;
	EFI_GUID Acpi2TableGuid = ACPI_20_TABLE_GUID;
	EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER* rsdp = NULL;
	EFI_ACPI_SDT_HEADER* Xsdt = NULL;

	bool patchSuccessful = false;
	UINT64 ret = EFI_SUCCESS;

	// locate RSDP (Root System Description Pointer) 
	for (UINTN SystemTableIndex = 0; SystemTableIndex < SystemTable->NumberOfTableEntries; SystemTableIndex++)
	{
		Print(L"Table #%d/%d: ", SystemTableIndex + 1, SystemTable->NumberOfTableEntries);

		if (!CompareGuid(&SystemTable->ConfigurationTable[SystemTableIndex].VendorGuid, &AcpiTableGuid) && !CompareGuid(
			&SystemTable->ConfigurationTable[SystemTableIndex].VendorGuid, &Acpi2TableGuid))
		{
			Print(L"Not ACPI\n");
			goto next_table;
		}

		if (strncmp8((unsigned char*)"RSD PTR ", (CHAR8*)ect->VendorTable, 8))
		{
			Print(L"Not RSDP\n");
			goto next_table;
		}

		rsdp = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER*)ect->VendorTable;
		Print(L"RSDP Rev %u @0x%x | ", rsdp->Revision, rsdp);

		//CHAR16 GuidStr[100];
		//Guid2String(GuidStr, &(SystemTable->ConfigurationTable[Index].VendorGuid));
		//Print(L"%s", GuidStr);
		//ParseRSDP(rsdp, GuidStr); // dump tables

		// check if we have XSDT
		if (rsdp->Revision < EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION)
		{
			Print(L"%ENo XSDT\n");
			rsdp = NULL;
			goto next_table;
		}

		// validate XSDT signature
		Xsdt = (EFI_ACPI_SDT_HEADER*)(rsdp->XsdtAddress);
		if (strncmp8((CHAR8*)"XSDT", (CHAR8*)(VOID*)(Xsdt->Signature), 4))
		{
			Print(L"%EInvalid XSDT\n");
			Xsdt = NULL;
			goto next_table;
		}

		// yeah we got XSDT!
		CHAR16 OemStr[20];
		Ascii2UnicodeStr((CHAR8*)(Xsdt->OemId), OemStr, 6);
		UINT32 EntryCount = (Xsdt->Length - sizeof(EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);
		Print(L"%HXSDT OEM ID: %s Tables: %d%N\n", OemStr, EntryCount);

		// iterate XSDT tables to search for BGRT
		UINT64* EntryPtr;
		CHAR16 SigStr[20];
		EntryPtr = (UINT64*)(Xsdt + 1);
		for (UINT32 XsdtTableIndex = 0; XsdtTableIndex < EntryCount; XsdtTableIndex++, EntryPtr++)
		{
			EFI_ACPI_SDT_HEADER* Entry = (EFI_ACPI_SDT_HEADER*)((UINTN)(*EntryPtr));
			Ascii2UnicodeStr((CHAR8*)(Entry->Signature), SigStr, 4);
			Ascii2UnicodeStr((CHAR8*)(Entry->OemId), OemStr, 6);
			Print(L"  ACPI table #%d/%d: %s Rev %d OEM ID: %s\n", XsdtTableIndex + 1, EntryCount, SigStr,
			      (int)(Entry->Revision), OemStr);

			// See Advanced Configuration and Power Interface Specification Version 6.2
			// Table 5-104 Boot Graphics Resource Table Fields
			if (!strncmp8((unsigned char *)"BGRT", (CHAR8*)(Entry->Signature), 4)) // found existing BGRT
			{
				// blow up the old table and replace with our brand new table
				memcpy8((CHAR8*)*EntryPtr, (CHAR8*)"FUCK", 4);
				*EntryPtr = (UINT64)newBgrtTable;
				patchSuccessful = true;
				Print(L"%HBGRT table has been replaced%N\n");
				break;
			}
		}

		// break if we found the XSDT but there is no BGRT
		break;

	next_table:
		ect++;
	}

	if (rsdp && Xsdt && !patchSuccessful)
	{
		// if we found the XSDT but there is no BGRT

		// create new XSDT
		XSDT* newXsdt = malloc_acpi(Xsdt->Length + sizeof(UINT64));
		if (newXsdt == NULL)
		{
			Print(L"%EXSDT table memory allocation failed%N\n");
			free(boot_image);
			free(newBgrtTable);
			return EFI_OUT_OF_RESOURCES;
		}

		// copy over old entries
		memcpy8((CHAR8*)newXsdt, (CHAR8*)Xsdt, Xsdt->Length);

		// insert entry
		newXsdt->Header.Length += sizeof(UINT64);
		UINT32 EntryCount = (Xsdt->Length - sizeof(EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);
		newXsdt->Entry[EntryCount - 1] = (UINT64)newBgrtTable;

		// debug mark; use rweverything (http://rweverything.com/) to look for it under Windows
		memcpy8((CHAR8*)&(newXsdt->Header.CreatorId), (CHAR8*)"YJSNPI", 6);

		// re-calculate XSDT checksum
		const CHAR8 new_xsdt_checksum_diff = AcpiChecksum((UINT8*)newXsdt, newXsdt->Header.Length);
		newXsdt->Header.Checksum -= new_xsdt_checksum_diff;

		// invalidate old XSDT table signature and checksum
		memcpy8((CHAR8*)Xsdt, (CHAR8*)"FUCK", 4);

		// replace old XSDT
		rsdp->XsdtAddress = (UINT64)newXsdt;

		// re-calculate RSDP extended checksum
		const CHAR8 new_rsdt_checksum_diff = AcpiChecksum((UINT8*)rsdp, rsdp->Length);
		rsdp->ExtendedChecksum -= new_rsdt_checksum_diff;

		Print(L"%HNew BGRT table inserted%N\n");
	}
	else if (rsdp == NULL)
	{
		Print(L"%EERROR: RSDP is not found%N\n");
		ret = EFI_UNSUPPORTED;
	}
	else if (Xsdt == NULL)
	{
		Print(L"%EERROR: XSDT is not found%N\n");
		ret = EFI_UNSUPPORTED;
	}
	else if (patchSuccessful)
	{
		// do nothing
	}
	else
	{
		Print(L"%EError: Something happened%N\n");
		ret = EFI_UNSUPPORTED;
	}

#if defined(_DEBUG)
	Print(L"%EBGRTInjector done, press any key to continue.%N\n\n");

	pause();

	// If running in debug mode, use the EFI shut down call to close QEMU
	/*Print(L"%EResetting system%N\n\n");
	SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);*/
#else
	// if we are running as an EFI driver, then just quit and let other things load
	Print(L"%EBGRTInjector done%N\n\n");
#endif

#if defined(LOAD_WINDOWS)
	// directly load Windows
	Print(L"%HSearching Microsoft bootloader...\n");
	CHAR16 MSBootloaderPath1[] = L"EFI\\Microsoft\\boot\\bootmgfw.efi";
	DIRTOOL_STATE DirToolState;
	DirToolState.initialized = 0;
	// Print(L"dirtool_init\n");
	EFI_STATUS status = dirtool_init(&DirToolState, ImageHandle);
	if (EFI_ERROR(status)) {
		return status;
	}

	// first try current disk
	DIRTOOL_DRIVE* drive;
	drive = dirtool_get_current_drive(&DirToolState, 0);
	dirtool_open_drive(&DirToolState, drive);
	DIRTOOL_FILE* pwd = dirtool_cd_multi(&(drive->RootFile), MSBootloaderPath1);
	if (pwd)load_efi_image(pwd, ImageHandle);
	dirtool_close_drive(&DirToolState, drive);

	if (!pwd) {
		Print(L"%Ebootmgfw.efi not found on current ESP disk, searching all disks%N\n");

#if defined(_DEBUG)
		pause();
#endif

		// then try to search every readable disk for bootmgfw.efi
		DIRTOOL_DRIVE_ITERATOR* iterator = dirtool_drive_iterator_start(&DirToolState);
		while ((drive = dirtool_drive_iterator_next(&DirToolState, iterator)) != NULL)
		{
			DIRTOOL_FILE* pwd = dirtool_open_drive(&DirToolState, drive);
			if (pwd == NULL) continue;
			pwd = dirtool_cd_multi(pwd, MSBootloaderPath1);
			if (pwd) load_efi_image(pwd, ImageHandle);
#if defined(_DEBUG)
			Print(L"before dirtool_close_drive()\n");
			pause();
#endif
			dirtool_close_drive(&DirToolState, drive);
		}
		dirtool_drive_iterator_end(&DirToolState, iterator);
		iterator = NULL;
	}

	dirtool_deinit(&DirToolState);
#endif
	return ret;
}

