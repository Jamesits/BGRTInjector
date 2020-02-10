#pragma once
#ifndef __CHAINLOAD_H__
#define __CHAINLOAD_H__

#include <efi.h>
#include <efilib.h>

#define MAX_FILE_INFO_SIZE 1024

EFI_DEVICE_PATH* GetCurrentDisk(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable);
EFI_STATUS Chainload(EFI_HANDLE Device, CHAR16* Path, EFI_HANDLE ImageHandle);
EFI_STATUS ChainloadByDevicePath(EFI_DEVICE_PATH* DevicePath, EFI_HANDLE ImageHandle);
EFI_DEVICE_PATH* SearchFile(CHAR16** Path, UINT8 PathLength, EFI_HANDLE ImageHandle);
#endif
