#include "chainload.h"
#include "nstdlib.h"
#include <stdbool.h>
#include "gop.h"

EFI_DEVICE_PATH* GetCurrentDisk(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
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
	//FreePool(loaded_image);

	return DeviceHandle;
}

// load a UEFI executable and execute it
// Device: a drive handle (GetCurrentDisk(ImageHandle, gST))
// Path: L"path\to\your\program.efi"
// ImageHandle: the global ImageHandle from main()
EFI_STATUS Chainload(EFI_HANDLE Device, CHAR16* Path, EFI_HANDLE ImageHandle)
{
	EFI_STATUS Status;
	Print(L"%HChainloading file %s%N\n", Path);
	EFI_DEVICE_PATH* DevicePath = FileDevicePath(Device, Path);

    return ChainloadByDevicePath(DevicePath, ImageHandle);
}

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

// TODO: fix FreePool position
EFI_DEVICE_PATH *SearchFile(CHAR16** Path, UINT8 PathLength, EFI_HANDLE ImageHandle)
{
    EFI_STATUS Status;
    UINTN NumHandles;
    EFI_HANDLE* Handles;
    UINTN Index;
    bool matched = false;

    Print(L"SearchFile()\n");

    // gets all handles with simple file system installed
    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &NumHandles, &Handles);
    if (EFI_ERROR(Status)) {
        return NULL;
    }

    Print(L"LocateHandleBuffer() got %u\n", NumHandles);

    // loop through all handles
    for (Index = 0; Index < NumHandles && !matched; Index++) {
        EFI_FILE_HANDLE pwd;
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Fs;
        EFI_DEVICE_PATH* FileDp;

        Print(L"Device #%d\n", Index);

        // get simple file system protocol instance
        // from current handle
        Status = gBS->OpenProtocol(Handles[Index], &gEfiSimpleFileSystemProtocolGuid, &Fs, NULL, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "Missing EFI_SIMPLE_FILE_SYSTEM_PROTOCOL on handle.\n"));
            continue;
        }

        // get device path instance from current handle
        Status = gBS->OpenProtocol(Handles[Index], &gEfiDevicePathProtocolGuid, &FileDp, NULL, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "Missing EFI_DEVICE_PATH_PROTOCOL on handle.\n"));
            continue;
        }

        // open root dir from current simple file system
        Status = Fs->OpenVolume(Fs, &pwd);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "Unable to open volume.\n"));
            continue;
        }

        // search for files
        UINT8 CurrentStage = 0;
        // recursively process files in root dir
        while (CurrentStage < PathLength && !matched)
        {
            EFI_STATUS Status;
            UINTN FileInfoSize;
            
            // big enough to hold EFI_FILE_INFO struct and the whole file path
            EFI_FILE_INFO* FileInfo = AllocatePool(MAX_FILE_INFO_SIZE);
            if (FileInfo == NULL) {
                // return EFI_OUT_OF_RESOURCES;
                return NULL;
            }
            
            for (;!matched;) { // loop current folder content
                // get the next file's info. there's an internal position
                // that gets incremented when you read from a directory
                // so that subsequent reads gets the next file's info
                FileInfoSize = MAX_FILE_INFO_SIZE;
                Status = pwd->Read(pwd, &FileInfoSize, (VOID*)FileInfo);
                if (EFI_ERROR(Status) || FileInfoSize == 0) { //this is how we eventually exit this function when we run out of files
                    if (Status == EFI_BUFFER_TOO_SMALL) {
                        Print(L"%EEFI_FILE_INFO too small%N\n");
                    }
                    //FreePool(FileInfo);
                    return NULL;
                }

                Print(L"Path %s\n", FileInfo->FileName);
                int len1 = strlen16(Path[CurrentStage]);
                int len2 = strlen16(FileInfo->FileName);
                if ( // matched
                    len1 == len2 && !strncmp8(Path[CurrentStage], FileInfo->FileName, len1) // file name is the same
                    && ((CurrentStage == PathLength - 1) || FileInfo->Attribute & EFI_FILE_DIRECTORY) // if it is not the last segment then it should be a folder
                )
                {
                    Print(L"Matched path segment %s\n", Path[CurrentStage]);
                    if (CurrentStage == PathLength - 1) // found the file
                    {
                        Print(L"File %s found!\n", FileInfo->FileName);
                        matched = true;
                        //FreePool(FileInfo);
                        // sync FileDp
                        EFI_DEVICE_PATH* OldFileDp = FileDp;
                        Print(L"Sync FileDp %s\n", FileInfo->FileName);
                        FileDp = FileDevicePath(OldFileDp, FileInfo->FileName);
                        //FreePool(OldFileDp);
                        if (FileDp == NULL) {
                            //FreePool(FileInfo);
                            // return EFI_OUT_OF_RESOURCES;
                            return NULL;
                        }

                        return FileDp;
                        break;
                    } else // a folder, go ahead
                    {
                        EFI_FILE_HANDLE NewDir;

                        Print(L"going down a level\n");
                        Status = pwd->Open(pwd, &NewDir, FileInfo->FileName, EFI_FILE_MODE_READ, 0);
                        
                        if (Status != EFI_SUCCESS) {
                            //FreePool(FileInfo);
                            //FreePool(FileDp);
                            return NULL;
                        }
                        NewDir->SetPosition(NewDir, 0);
                        pwd->Close(pwd);
                        if (Status != EFI_SUCCESS) {
                            //FreePool(FileInfo);
                            //FreePool(FileDp);
                            return NULL;
                        }

                        // sync FileDp
                        EFI_DEVICE_PATH* OldFileDp = FileDp;
                        FileDp = FileDevicePath(OldFileDp, FileInfo->FileName);
                        Print(L"Sync FileDp %s\n", FileInfo->FileName);
                        //FreePool(OldFileDp);
                        if (FileDp == NULL) {
                            //FreePool(FileInfo);
                            // return EFI_OUT_OF_RESOURCES;
                            return NULL;
                        }

                        pwd = NewDir;
                        CurrentStage++;
                        Print(L"Going into dir %s\n", FileInfo->FileName);
                    }
                } // if matched

                
            } // loop current folder content
            //FreePool(FileDp);

        } // while searching current level
        
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "SearchFile() error. Continuing with next volume...\n"));
            continue;
        }

    } // loop filesystems

    return NULL;
}