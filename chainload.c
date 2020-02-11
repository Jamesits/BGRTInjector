#include "chainload.h"
#include "nstdlib.h"

// load a UEFI executable and execute it if you know the volume (device) and the absolute path
// Device: a drive handle (get from DIRTOOL_DRIVE->Handle)
// Path: L"path\to\your\program.efi"
// ImageHandle: the global ImageHandle from main()
EFI_STATUS Chainload(EFI_HANDLE Device, CHAR16* Path, EFI_HANDLE ImageHandle)
{
	Print(L"%HChainloading file %s%N\n", Path);
	EFI_DEVICE_PATH* DevicePath = FileDevicePath(Device, Path);

    return ChainloadByDevicePath(DevicePath, ImageHandle);
}

// load a UEFI executable by a DevicePath
// DevicePath: you know it
// ImageHandle: the global ImageHandle from main()
EFI_STATUS ChainloadByDevicePath(EFI_DEVICE_PATH* DevicePath, EFI_HANDLE ImageHandle)
{
    EFI_STATUS Status;

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
