#pragma once
#ifndef __DIRTOOL_H__
#define __DIRTOOL_H__

#include <efi.h>
#include <efilib.h>
#include <stdbool.h>

#define MAX_FILE_INFO_SIZE 1024
#define MAX_PATH_LENGTH 512
struct _DIRTOOL_DRIVE;

typedef struct _DIRTOOL_FILE
{
	bool isDir;
	CHAR16 Path[MAX_PATH_LENGTH];
	EFI_FILE_HANDLE FileHandle;
	EFI_FILE_INFO* FileInfo;
	EFI_DEVICE_PATH* DevicePath;
	struct _DIRTOOL_DRIVE* Drive;
	struct _DIRTOOL_FILE* ParentFolder;
	struct _DIRTOOL_FILE* ChildFile;
} DIRTOOL_FILE;

typedef struct _DIRTOOL_DRIVE
{
	bool available;

	EFI_HANDLE Handle;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem;

	bool isOpened;
	DIRTOOL_FILE RootFile;
} DIRTOOL_DRIVE;

typedef struct _DIRTOOL_DRIVE_ITERATOR
{
	UINTN current;
} DIRTOOL_DRIVE_ITERATOR; 

typedef struct _DIRTOOL_STATE
{
	bool initialized;
	EFI_HANDLE ImageHandle;
	UINTN DriveCount;
	DIRTOOL_DRIVE* Drives;
} DIRTOOL_STATE;

EFI_STATUS dirtool_init(DIRTOOL_STATE* state, EFI_HANDLE ImageHandle);
DIRTOOL_DRIVE_ITERATOR* dirtool_drive_iterator_start(DIRTOOL_STATE* state);
DIRTOOL_DRIVE* dirtool_drive_iterator_next(DIRTOOL_STATE* state, DIRTOOL_DRIVE_ITERATOR* iterator);
EFI_STATUS dirtool_drive_iterator_end(DIRTOOL_STATE* state, DIRTOOL_DRIVE_ITERATOR* iterator);
DIRTOOL_DRIVE* dirtool_get_current_drive(DIRTOOL_STATE* state, EFI_HANDLE ImageHandle);
DIRTOOL_FILE* dirtool_open_drive(DIRTOOL_STATE* state, DIRTOOL_DRIVE* drive);
DIRTOOL_FILE* dirtool_cd(DIRTOOL_FILE* pwd, CHAR16* NewFileName);
DIRTOOL_FILE* dirtool_cd_multi(DIRTOOL_FILE* pwd, CHAR16* Path);
EFI_STATUS __dirtool_release_file(DIRTOOL_FILE* file);
DIRTOOL_FILE* dirtool_go_up(DIRTOOL_FILE* pwd);
void dirtool_close_drive(DIRTOOL_STATE* state, DIRTOOL_DRIVE* drive);
EFI_STATUS dirtool_deinit(DIRTOOL_STATE* state);
CHAR8* dirtool_read_file(DIRTOOL_FILE* file);
#endif
