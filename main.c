#include <stdbool.h>
#include <efi.h>
#include <efilib.h>
#include "listacpi.h"
#include "bgrt.h"
#include "acpi_checksum.h"
#include "gop.h"
#include "pause.h"
#include "bmp.h"
#include "memory_management.h"
#include "xsdt.h"

const char default_bootimage[] = {
#include "default_bootimage.bmp.inc"
};

// Application entrypoint (must be set to 'efi_main' for gnu-efi crt0 compatibility)
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
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
		Print(L"%HUnable to get GOP screen size");
		return EFI_UNSUPPORTED;
	}

	// load image file
	BMP_IMAGE_HEADER* boot_image = &default_bootimage;
	// TODO: check if image is 24bit or 32bit
	if (myStrnCmpA("BM", boot_image, 2))
	{
		Print(L"Image file format not recognized");
		return EFI_UNSUPPORTED;
	}

	// calculate offset
	UINT32 offsetX = (gop->Mode->Info->HorizontalResolution - boot_image->biWidth) / 2;
	UINT32 offsetY = (gop->Mode->Info->VerticalResolution - boot_image->biHeight) / 2;

	Print(L"Screen size: %u*%u, Image size: %d*%d (%u bytes), Top left point: %u*%u\n", 
		gop->Mode->Info->HorizontalResolution, gop->Mode->Info->VerticalResolution,
		boot_image->biWidth, boot_image->biHeight, boot_image->bfSize,
		offsetX, offsetY
	);

	// craft a new BGRT table
	// TODO: check if malloc() succeeds
	EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE* newBgrtTable = malloc_acpi(sizeof(EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE));
	
	newBgrtTable->Header.Signature = 'TRGB'; // using multibyte char so we are inverted
	newBgrtTable->Header.Length = sizeof(EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE);
	newBgrtTable->Header.Revision = 1;
	memcpy2(newBgrtTable->Header.OemId, "YJSNPI", 6);
	memcpy2(&(newBgrtTable->Header.OemTableId), "YJSNPI  ", 8);
	newBgrtTable->Header.OemRevision = 1919;
	memcpy2(&(newBgrtTable->Header.CreatorId), "1919", 4);
	newBgrtTable->Header.CreatorRevision = 1919;
	newBgrtTable->Version = EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE_REVISION;
	// Status: set to 0 (INVALID) and Windows loader will draw the image; set to 1(VALID) then Windows loader will retain whatever on the screen not redrawing framebuffer
	// set bit[1:2] to 1, 2, 3 for rotation 90, 180, 270 degrees
	newBgrtTable->Status = EFI_ACPI_5_0_BGRT_STATUS_INVALID;
	newBgrtTable->ImageType = EFI_ACPI_5_0_BGRT_IMAGE_TYPE_BMP;
	newBgrtTable->ImageOffsetX = offsetX;
	newBgrtTable->ImageOffsetY = offsetY;

	// TODO: check if malloc() succeeds
	BMP_IMAGE_HEADER* newBmpImage = malloc_acpi(boot_image->bfSize);
	memcpy2(newBmpImage, default_bootimage, boot_image->bfSize);
	newBgrtTable->ImageAddress = newBmpImage;

	// checksum
	UINT8 checksum_diff = AcpiChecksum(newBgrtTable, newBgrtTable->Header.Length);
	newBgrtTable->Header.Checksum -= checksum_diff;

	// TODO: free original image if needed

	// pause(SystemTable);

	EFI_CONFIGURATION_TABLE* ect = SystemTable->ConfigurationTable;
	EFI_GUID AcpiTableGuid = ACPI_TABLE_GUID;
	EFI_GUID Acpi2TableGuid = ACPI_20_TABLE_GUID;
	EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER* rsdp = NULL;
	EFI_ACPI_SDT_HEADER* Xsdt = NULL;

	bool patchSuccessful = false;
	UINT64 ret = EFI_SUCCESS;

	// locate RSDP (Root System Description Pointer) 
	for (int SystemTableIndex = 0; SystemTableIndex < SystemTable->NumberOfTableEntries; SystemTableIndex++) {
		Print(L"Table #%d/%d: ", SystemTableIndex + 1, SystemTable->NumberOfTableEntries);

		if (!CompareGuid(&SystemTable->ConfigurationTable[SystemTableIndex].VendorGuid, &AcpiTableGuid) && !CompareGuid(&SystemTable->ConfigurationTable[SystemTableIndex].VendorGuid, &Acpi2TableGuid)) {
			Print(L"Not ACPI\n");
			goto next_table;
		}

		if (myStrnCmpA((unsigned char*)"RSD PTR ", (CHAR8*)ect->VendorTable, 8)) {
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
		if (rsdp->Revision < EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION) {
			Print(L"No XSDT\n");
			rsdp = NULL;
			goto next_table;
		}

		// validate XSDT signature
		Xsdt = (EFI_ACPI_SDT_HEADER*)(rsdp->XsdtAddress);
		if (myStrnCmpA("XSDT", (CHAR8*)(VOID*)(Xsdt->Signature), 4)) {
			Print(L"Invalid XSDT\n");
			Xsdt = NULL;
			goto next_table;
		}

		// yeah we got XSDT!
		CHAR16 OemStr[20];
		Ascii2UnicodeStr((CHAR8*)(Xsdt->OemId), OemStr, 6);
		UINT32 EntryCount = (Xsdt->Length - sizeof(EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);
		Print(L"%HXSDT OEM ID: %s Tables: %d%N\n", OemStr, EntryCount);

		// iterate XSDT tables
		UINT64* EntryPtr;
		CHAR16 SigStr[20];
		EntryPtr = (UINT64*)(Xsdt + 1);
		for (UINT32 XsdtTableIndex = 0; XsdtTableIndex < EntryCount; XsdtTableIndex++, EntryPtr++) {
			EFI_ACPI_SDT_HEADER* Entry = (EFI_ACPI_SDT_HEADER*)((UINTN)(*EntryPtr));
			Ascii2UnicodeStr((CHAR8*)(Entry->Signature), SigStr, 4);
			Ascii2UnicodeStr((CHAR8*)(Entry->OemId), OemStr, 6);
			Print(L"  ACPI table #%d/%d: %s Rev %d OEM ID: %s\n", XsdtTableIndex + 1, EntryCount, SigStr, (int)(Entry->Revision), OemStr);

			// See Advanced Configuration and Power Interface Specification Version 6.2
			// Table 5-104 Boot Graphics Resource Table Fields
			if (!myStrnCmpA((unsigned char *)"BGRT", (CHAR8*)(Entry->Signature), 4))
			{
				// blow up the old table and replace with our brand new table
				memcpy2((CHAR8 *)*EntryPtr, L"FUCK", 4);
				*EntryPtr = newBgrtTable;
				patchSuccessful = true;
				Print(L"%NBGRT table has been replaced\n");
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
		memcpy2(newXsdt, Xsdt, Xsdt->Length);

		// insert entry
		newXsdt->Header.Length += sizeof(UINT64);
		UINT32 EntryCount = (Xsdt->Length - sizeof(EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);
		newXsdt->Entry[EntryCount - 1] = newBgrtTable;

		// debug mark
		memcpy2(newXsdt->Header.CreatorId, "YJSNPI", 6);

		// re-calculate XSDT checksum
		CHAR8 new_xsdt_checksum_diff = AcpiChecksum(newXsdt, newXsdt->Header.Length);
		newXsdt->Header.Checksum -= new_xsdt_checksum_diff;

		// replace old XSDT
		memcpy2(Xsdt, "FUCK", 4);
		rsdp->XsdtAddress = newXsdt;

		// re-calculate RSDP extended checksum
		CHAR8 new_rsdt_checksum_diff = AcpiChecksum(rsdp, rsdp->Length);
		rsdp->ExtendedChecksum -= new_rsdt_checksum_diff;

		Print(L"%HNew BGRT table inserted%N\n");
	} else if (rsdp == NULL) {
		Print(L"%EERROR: RSDP is not found%N\n");
		ret = EFI_UNSUPPORTED;
	}
	else if (Xsdt == NULL)
	{
		Print(L"%EERROR: XSDT is not found%N\n");
		ret = EFI_UNSUPPORTED;
	} else if (patchSuccessful)
	{
		// do nothing
	}
	else
	{
		Print(L"%EError: Something happened%N\n");
		ret = EFI_UNSUPPORTED;
	}

	pause(SystemTable);

#if defined(_DEBUG)
	Print(L"%EBGRTInjector done, press any key to continue.%N\n\n");

	pause(SystemTable);

	// If running in debug mode, use the EFI shut down call to close QEMU
	Print(L"%EResetting system%N\n\n");
	SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
#else
	// if we are running as an EFI driver, then just quit and let other things load
	Print(L"%EBGRTInjector done%N\n\n");
#endif

	return ret;
}
