#pragma once
#ifndef __FILE_H__
#define __FILE_H__

typedef EFI_STATUS EFIAPI FileSystemIterateCallback(IN EFI_FILE_HANDLE Dir, IN EFI_DEVICE_PATH* DirDp, IN EFI_FILE_INFO* FileInfo, IN EFI_DEVICE_PATH* Dp);

EFI_STATUS PrintDevices(EFI_HANDLE image, EFI_SYSTEM_TABLE* systemTable);
EFI_STATUS IterateFiles(
	IN EFI_HANDLE ImageHandle,
	IN EFI_SYSTEM_TABLE* SystemTable
);
EFI_STATUS EFIAPI ProcessFilesInDir(IN EFI_FILE_HANDLE Dir, IN EFI_DEVICE_PATH CONST* DirDp, IN FileSystemIterateCallback callback);
EFI_STATUS EFIAPI PerFileFuncExample(IN EFI_FILE_HANDLE Dir, IN EFI_DEVICE_PATH* DirDp, IN EFI_FILE_INFO* FileInfo, IN EFI_DEVICE_PATH* Dp);
#endif