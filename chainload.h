#pragma once
#ifndef __CHAINLOAD_H__
#define __CHAINLOAD_H__

#include <efi.h>
#include <efilib.h>

EFI_DEVICE_PATH_PROTOCOL* GetCurrentDisk(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable);
EFI_STATUS Chainload(EFI_HANDLE Device, CHAR16* Path, EFI_HANDLE ImageHandle);

#endif
