// https://github.com/tqh/efi-example/blob/master/disk_example.c
// https://edk2-devel.narkive.com/WhxSiG6I/directory-file-system-traversal-example
#include <efi.h>
#include <efilib.h>
#include "file.h"
#include "nstdlib.h"
#include <stdbool.h>

const static EFI_GUID BlockIoProtocolGUID = BLOCK_IO_PROTOCOL;
const static EFI_GUID DevicePathGUID = DEVICE_PATH_PROTOCOL;
static EFI_GUID gEfiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

void printInt(SIMPLE_TEXT_OUTPUT_INTERFACE* conOut, int value) {
    CHAR16 out[32];
    CHAR16* ptr = out;
    if (value == 0) {
        conOut->OutputString(conOut, L"0");
        return;
    }

    ptr += 31;
    *--ptr = 0;
    int tmp = value;// >= 0 ? value : -value; 

    while (tmp) {
        *--ptr = '0' + tmp % 10;
        tmp /= 10;
    }
    if (value < 0) *--ptr = '-';
    conOut->OutputString(conOut, ptr);
}

void printLabeledInt(SIMPLE_TEXT_OUTPUT_INTERFACE* conOut, CHAR16* label, int value) {
    conOut->OutputString(conOut, label);
    printInt(conOut, value);
    // conOut->OutputString(conOut, L"\r\n");
}

CHAR16 asChar(UINT8 nibble) {
    return nibble + (nibble < 10 ? '0' : '7');
}

void printUUID(SIMPLE_TEXT_OUTPUT_INTERFACE* conOut, UINT8 uuid[16]) {
    int charPos = 0;
    int i;
    CHAR16* uuidStr = L"00000000-0000-0000-0000-000000000000";
    for (i = 3; i >= 0; i--) {
        uuidStr[charPos++] = asChar(uuid[i] >> 4);
        uuidStr[charPos++] = asChar(uuid[i] & 0xF);
    }
    charPos++;
    for (i = 5; i >= 4; i--) {
        uuidStr[charPos++] = asChar(uuid[i] >> 4);
        uuidStr[charPos++] = asChar(uuid[i] & 0xF);
    }
    charPos++;
    for (i = 7; i >= 6; i--) {
        uuidStr[charPos++] = asChar(uuid[i] >> 4);
        uuidStr[charPos++] = asChar(uuid[i] & 0xF);
    }
    charPos++;
    for (i = 8; i <= 9; i++) {
        uuidStr[charPos++] = asChar(uuid[i] >> 4);
        uuidStr[charPos++] = asChar(uuid[i] & 0xF);
    }

    for (i = 10; i < 16; i++) {
        if (i == 4 || i == 6 || i == 8 || i == 10) charPos++;
        uuidStr[charPos++] = asChar(uuid[i] >> 4);
        uuidStr[charPos++] = asChar(uuid[i] & 0xF);
    }
    conOut->OutputString(conOut, L"\r\n");
    conOut->OutputString(conOut, uuidStr);
}


void printDevicePath(SIMPLE_TEXT_OUTPUT_INTERFACE* conOut, EFI_DEVICE_PATH* devicePath) {
    EFI_DEVICE_PATH* node = devicePath;
    // https://github.com/vathpela/gnu-efi/blob/master/lib/dpath.c for printing device paths.
    Print(L" Device type: ");

    for (; !IsDevicePathEnd(node); node = NextDevicePathNode(node)) {
        Print(L"\\%u.%u", node->Type, node->SubType);

        if (node->Type == MEDIA_DEVICE_PATH && node->SubType == MEDIA_HARDDRIVE_DP) {

            HARDDRIVE_DEVICE_PATH* hdPath = (HARDDRIVE_DEVICE_PATH*)node;
            Print(L"\n  nr=%u, start=%llu, size=%llu, MBR_type=%u, sig_type=%u", hdPath->PartitionNumber, hdPath->PartitionStart, hdPath->PartitionSize, hdPath->MBRType, hdPath->SignatureType);

            printUUID(conOut, hdPath->Signature);
            conOut->OutputString(conOut, L"\r\n");

            // try list files

            

            pause(gST);
        }
    }
    conOut->OutputString(conOut, L"\r\n");
}

EFI_STATUS PrintDevices(EFI_HANDLE image, EFI_SYSTEM_TABLE* systemTable)
{

    SIMPLE_TEXT_OUTPUT_INTERFACE* conOut = systemTable->ConOut;
    EFI_EVENT event = systemTable->ConIn->WaitForKey;
    UINTN index;
    EFI_HANDLE handles[100];
    EFI_DEVICE_PATH* devicePath;
    EFI_BLOCK_IO* blockIOProtocol;
    UINTN bufferSize = 100 * sizeof(EFI_HANDLE);
    int noOfHandles;

    EFI_STATUS status = gBS->LocateHandle(
        ByProtocol,
        &BlockIoProtocolGUID,
        NULL, /* Ignored for AllHandles or ByProtocol */
        &bufferSize, handles);

    noOfHandles = bufferSize == 0 ? 0 : bufferSize / sizeof(EFI_HANDLE);
    printLabeledInt(conOut, L"No of handles reported: ", noOfHandles);
    if (EFI_ERROR(status)) {
        conOut->OutputString(conOut, L"Failed to LocateHandles!\r\n");
        return status;
    }
    for (int i = 0; i < noOfHandles; i++) {
        status = gBS->HandleProtocol(handles[i], &DevicePathGUID, (void*)&devicePath);
        if (EFI_ERROR(status) || devicePath == NULL) {
            conOut->OutputString(conOut, L"Skipped handle, device path error!\r\n");
            continue;
        }
        status = gBS->HandleProtocol(handles[i], &BlockIoProtocolGUID, (void*)&blockIOProtocol);
        if (EFI_ERROR(status) || blockIOProtocol == NULL) {
            conOut->OutputString(conOut, L"Skipped handle, block io protocol error!\r\n");
            continue;
        }
        printDevicePath(conOut, devicePath);
        printLabeledInt(conOut, L"Media ID: ", blockIOProtocol->Media->MediaId);
    }

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI ProcessFilesInDir(IN EFI_FILE_HANDLE Dir, IN EFI_DEVICE_PATH CONST* DirDp);
EFI_STATUS EFIAPI PerFileFunc( IN EFI_FILE_HANDLE Dir, IN EFI_DEVICE_PATH* DirDp, IN EFI_FILE_INFO* FileInfo, IN EFI_DEVICE_PATH* Dp);

EFI_STATUS IterateFiles(IN EFI_HANDLE ImageHandle, IN FileSystemIterateCallback callback)
{
    EFI_STATUS Status;
    UINTN NumHandles;
    EFI_HANDLE* Handles;
    UINTN Index;
    VOID* Context;

    // gets all handles with simple file system installed
    Status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &NumHandles,
        &Handles
    );
    if (EFI_ERROR(Status)) {
        return Status;
    }

    // loop through all handles we just got
    for (Index = 0; Index < NumHandles; Index++) {
        EFI_FILE_HANDLE Root;
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Fs;
        EFI_DEVICE_PATH* Dp;

        // get simple file system protocol instance
        // from current handle
        Status = gBS->OpenProtocol(Handles[Index], &gEfiSimpleFileSystemProtocolGuid, &Fs, NULL, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "Missing EFI_SIMPLE_FILE_SYSTEM_PROTOCOL on handle.\n"));
                continue;
        }

        // get device path instance from current handle
        Status = gBS->OpenProtocol(Handles[Index], &gEfiDevicePathProtocolGuid, &Dp, NULL, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "Missing EFI_DEVICE_PATH_PROTOCOL on handle.\n"));
                continue;
        }

        // open root dir from current simple file system
        Status = Fs->OpenVolume(Fs, &Root);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "Unable to open volume.\n"));
            continue;
        }

        // recursively process files in root dir
        Context = NULL;
        Status = ProcessFilesInDir(Root, Dp);
        Root->Close(Root);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "ProcessFilesInDir error. Continuing with next volume...\n"));
                continue;
        }

    }

    // Print(L"IterateFiles() done\n");

    return EFI_SUCCESS;
}

//I have no idea what the max length of a file path is in EFI
#define MAX_FILE_INFO_SIZE 1024

/**
recurse through directory, calling a user defined
function for each file
*/
EFI_STATUS EFIAPI ProcessFilesInDir(IN EFI_FILE_HANDLE Dir, IN EFI_DEVICE_PATH CONST* DirDp, IN FileSystemIterateCallback callback)
{
    EFI_STATUS Status;
    EFI_FILE_INFO* FileInfo;
    CHAR16* FileName;
    UINTN FileInfoSize;
    EFI_DEVICE_PATH* Dp;

    // big enough to hold EFI_FILE_INFO struct and
    // the whole file path
    FileInfo = AllocatePool(MAX_FILE_INFO_SIZE);
    if (FileInfo == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }

    bool cont = true;
    for (; cont;) {
        // get the next file's info. there's an internal position
        // that gets incremented when you read from a directory
        // so that subsequent reads gets the next file's info
        FileInfoSize = MAX_FILE_INFO_SIZE;
        Status = Dir->Read(Dir, &FileInfoSize, (VOID*)FileInfo);
        if (EFI_ERROR(Status) || FileInfoSize == 0) { //this is how we eventually exit this function when we run out of files
                if (Status == EFI_BUFFER_TOO_SMALL) {
                    Print(L"EFI_FILE_INFO > MAX_FILE_INFO_SIZE. Increase the size\n");
                }
            FreePool(FileInfo);
            return Status;
        }

        FileName = FileInfo->FileName;

        // skip files named . or ..
        if (StrCmp(FileName, L".") == 0 || StrCmp(FileName, L"..") == 0) {
            continue;
        }

        // so we have absolute device path to child file/dir
        Dp = FileDevicePath(DirDp, FileName);
        if (Dp == NULL) {
            FreePool(FileInfo);
            return EFI_OUT_OF_RESOURCES;
        }

        // Do whatever processing on the file; if func returns non-zero then stop iterating
        Status = callback(Dir, DirDp, FileInfo, Dp);
        if (Status) {
            cont = false;
            goto exit_loop;
        }

        if (FileInfo->Attribute & EFI_FILE_DIRECTORY) {
            EFI_FILE_HANDLE NewDir;

            Status = Dir->Open(Dir, &NewDir, FileName, EFI_FILE_MODE_READ, 0);
            if (Status != EFI_SUCCESS) {
                FreePool(FileInfo);
                FreePool(Dp);
                return Status;
            }
            NewDir->SetPosition(NewDir, 0);

            Status = ProcessFilesInDir(NewDir, Dp, callback);
            Dir->Close(NewDir);
            if (Status != EFI_SUCCESS) {
                FreePool(FileInfo);
                FreePool(Dp);
                return Status;
            }
        }

        exit_loop:
        FreePool(Dp);
    }
}


// an example of FileSystemIterateCallback
EFI_STATUS EFIAPI PerFileFuncExample(IN EFI_FILE_HANDLE Dir, IN EFI_DEVICE_PATH* DirDp, IN EFI_FILE_INFO* FileInfo, IN EFI_DEVICE_PATH* Dp)
{
    EFI_STATUS Status;
    EFI_FILE_HANDLE File;

    Print(L"FileName = %s\n", FileInfo->FileName);

    // read the file into a buffer
    Status = Dir->Open(Dir, &File, FileInfo->FileName, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    // reset position just in case
    File->SetPosition(File, 0);

    // ****Do stuff on the file here****

    Dir->Close(File);

    return EFI_SUCCESS;
}

