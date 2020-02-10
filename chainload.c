#include "chainload.h"
#include "memory_management.h"

EFI_DEVICE_PATH_PROTOCOL* GetCurrentDisk(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
	EFI_LOADED_IMAGE* loaded_image = NULL;
	EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;

	const EFI_STATUS status = uefi_call_wrapper(SystemTable->BootServices->HandleProtocol, 3, ImageHandle,
	                                            &loaded_image_protocol, (void**)&loaded_image);
	if (EFI_ERROR(status))
	{
		Print(L"%GetCurrentDisk() error: %r%N\n", status);
	}

	Print(L"Image path: %s\n", loaded_image->FilePath);
	const EFI_HANDLE DeviceHandle = loaded_image->DeviceHandle;
	FreePool(loaded_image);

	return DeviceHandle;
}

// load a UEFI executable from the same drive of this executable and execute it
// Device: a drive handle (GetCurrentDisk(ImageHandle, gST))
// Path: L"path\to\your\program.efi"
// ImageHandle: the global ImageHandle from main()
EFI_STATUS Chainload(EFI_HANDLE Device, CHAR16* Path, EFI_HANDLE ImageHandle)
{
	EFI_STATUS Status;
	Print(L"%HChainloading file %s%N\n", Path);
	EFI_DEVICE_PATH_PROTOCOL* DevicePath = FileDevicePath(Device, Path);

	// Load the image with:
	// FALSE - not from boot manager and NULL, 0 being not already in memory
	EFI_HANDLE NewHandle;
	Status = gBS->LoadImage(FALSE, ImageHandle, DevicePath, NULL, 0, &NewHandle);

	if (EFI_ERROR(Status))
	{
		if (NewHandle != NULL)
		{
			gBS->UnloadImage(NewHandle);
		}
		Print(L"%EChainload() LoadImage failed: %r%N\n", Status);
		return (Status);
	}

	// now start the image, passing up exit data if the caller requested it
	UINTN ExitDataSizePtr;
	Status = gBS->StartImage(NewHandle, &ExitDataSizePtr, NULL);
	if (EFI_ERROR(Status))
	{
		if (NewHandle != NULL)
		{
			gBS->UnloadImage(NewHandle);
		}
		Print(L"%EChainload() StartImage failed: %r%N\n", Status);
		return (Status);
	}

	gBS->UnloadImage(NewHandle);
	return EFI_SUCCESS;
}
