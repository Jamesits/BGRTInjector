// https://github.com/tqh/efi-example/blob/master/disk_example.c
// https://edk2-devel.narkive.com/WhxSiG6I/directory-file-system-traversal-example

#include "dirtool.h"
#include "nstdlib.h"

// initialize a DIRTOOL state and get a list of Drives
EFI_STATUS dirtool_init(DIRTOOL_STATE* state, EFI_HANDLE ImageHandle)
{
	if (state->initialized)
	{
		// ignore double call
		return EFI_UNSUPPORTED;
	}

	state->ImageHandle = ImageHandle;

	EFI_STATUS Status;

	// enumerate drives
	UINTN HandleCount;
	EFI_HANDLE* Handles;

	Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &Handles);
	if (EFI_ERROR(Status)) {
		return EFI_UNSUPPORTED;
	}

	state->DriveCount = HandleCount;
	state->Drives = malloc(HandleCount * sizeof(DIRTOOL_DRIVE));
	if (state->Drives == NULL)
	{
		return EFI_OUT_OF_RESOURCES;
	}

	// initialize individual drive data structure
	for (UINTN i = 0; i < HandleCount; ++i)
	{
		state->Drives[i].Handle = Handles[i];
		state->Drives[i].available = false;

		// get EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
		Status = gBS->OpenProtocol(Handles[i], &gEfiSimpleFileSystemProtocolGuid, &(state->Drives[i].FileSystem), NULL, state->ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
		if (EFI_ERROR(Status)) {
			DEBUG((EFI_D_ERROR, "Missing EFI_SIMPLE_FILE_SYSTEM_PROTOCOL on handle.\n"));
			continue;
		}

		state->Drives[i].isOpened = false;
		state->Drives[i].available = true;
	}

	state->initialized = true;
	return EFI_SUCCESS;
}

// call to create a drive iterator
DIRTOOL_DRIVE_ITERATOR *dirtool_drive_iterator_start(DIRTOOL_STATE* state)
{
	if (!state->initialized) return NULL;
	DIRTOOL_DRIVE_ITERATOR* ret = malloc(sizeof(DIRTOOL_DRIVE_ITERATOR));
	if (ret == NULL) return NULL;
	ret->current = 0;
	return ret;
}

// get next drive
// usage:
/*
	DIRTOOL_STATE DirToolState;
	DirToolState.initialized = 0;
	EFI_STATUS status = dirtool_init(&DirToolState, ImageHandle);
	if (EFI_ERROR(status)) {
		return status;
	}

	DIRTOOL_DRIVE_ITERATOR* iterator = dirtool_drive_iterator_start(&DirToolState);
	while ((DIRTOOL_DRIVE *drive = dirtool_drive_iterator_next(&DirToolState, iterator)) != NULL)
	{
		Print(L"Opening drive 0x%x\n", &drive);
		DIRTOOL_FILE* pwd = dirtool_open_drive(&DirToolState, drive);
		if (pwd == NULL) continue;
		pwd = dirtool_cd_multi(pwd, L"Path\\to\\your\\file");
		if (pwd) { do_something(); }
		dirtool_close_drive(&DirToolState, drive);
	}
	dirtool_drive_iterator_end(&DirToolState, iterator);
*/
DIRTOOL_DRIVE *dirtool_drive_iterator_next(DIRTOOL_STATE* state, DIRTOOL_DRIVE_ITERATOR *iterator)
{
	if (!state->initialized) return NULL;
	if (iterator->current >= state->DriveCount) return NULL;
	DIRTOOL_DRIVE* ret = state->Drives + iterator->current;
	iterator->current += 1;
	return ret;
}

// call to dispose a drive iterator
EFI_STATUS dirtool_drive_iterator_end(DIRTOOL_STATE* state, DIRTOOL_DRIVE_ITERATOR* iterator)
{
	free(iterator);
	return EFI_SUCCESS;
}

// get the drive current executable is on
// if ImageHandle == NULL then use global ImageHandle
DIRTOOL_DRIVE *dirtool_get_current_drive(DIRTOOL_STATE* state, EFI_HANDLE ImageHandle)
{
	if (!state->initialized) return NULL;

	if (ImageHandle == NULL) ImageHandle = state->ImageHandle;
	EFI_LOADED_IMAGE* loaded_image = NULL;
	EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;

	const EFI_STATUS status = uefi_call_wrapper(gST->BootServices->HandleProtocol, 3, ImageHandle,
		&loaded_image_protocol, (void**)&loaded_image);
	if (EFI_ERROR(status))
	{
		Print(L"%LOADED_IMAGE_PROTOCOL error: %r%N\n", status);
	}

	const EFI_HANDLE DeviceHandle = loaded_image->DeviceHandle;
	free(loaded_image); // TODO: INVALID_PARAMETER?

	for (UINTN i = 0; i < state->DriveCount; ++i)
	{
		if (state->Drives[i].Handle == DeviceHandle)
		{
			return &(state->Drives[i]);
		}
	}

	return NULL;
}

// open a drive and return the root folder file object
DIRTOOL_FILE *dirtool_open_drive(DIRTOOL_STATE* state, DIRTOOL_DRIVE *drive)
{
	if (!drive->available) return NULL;
	if (drive->isOpened) return &(drive->RootFile);

	EFI_STATUS status;

	// get EFI_DEVICE_PATH_PROTOCOL
	// Print(L"EFI_DEVICE_PATH_PROTOCOL\n");
	if (drive->RootFile.DevicePath == NULL) // first time init
	{
		status = gBS->OpenProtocol(drive->Handle, &gEfiDevicePathProtocolGuid, &(drive->RootFile.DevicePath), NULL, state->ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
		if (EFI_ERROR(status)) {
			DEBUG((EFI_D_ERROR, "Missing EFI_DEVICE_PATH_PROTOCOL on handle.\n"));
			return NULL;
		}
	}

	// open root directory
	// Print(L"OpenVolume\n");
	status = drive->FileSystem->OpenVolume(drive->FileSystem, &(drive->RootFile.FileHandle));
	if (EFI_ERROR(status)) {
		return NULL;
	}

	drive->RootFile.FileInfo = LibFileInfo(drive->RootFile.FileHandle);
	if (drive->RootFile.FileInfo == NULL) {
		return NULL;
	}

	drive->RootFile.Path[0] = 0;
	drive->RootFile.Drive = drive;
	drive->RootFile.ParentFolder = NULL;
	drive->RootFile.ChildFile = NULL;

	drive->isOpened = true;
	return &(drive->RootFile);
}

// cd to next directory level or get a file in current directory
DIRTOOL_FILE *dirtool_cd(DIRTOOL_FILE *pwd, CHAR16 *NewFileName)
{
	if (pwd == NULL || NewFileName == NULL) {
		Print(L"%Edirtool_cd() got NULL parameter%N\n");
		return NULL;
	}
	Print(L"dirtool_cd() %s -> %s\n", pwd->Path, NewFileName);
	DIRTOOL_FILE *NewFile = malloc(sizeof(DIRTOOL_FILE));

	// Print(L"FileHandle->Open()\n");
	EFI_STATUS status = pwd->FileHandle->Open(pwd->FileHandle, &(NewFile->FileHandle), NewFileName, EFI_FILE_MODE_READ, 0);
	if (status != EFI_SUCCESS) {
		free(NewFile);
		return NULL;
	}

	NewFile->ParentFolder = pwd;
	NewFile->ChildFile = NULL;
	pwd->ChildFile = NewFile;
	NewFile->Drive = pwd->Drive;
	// reset file read position; not really needed on directories but it is in some examples
	Print(L"FileHandle->SetPosition()\n");
	NewFile->FileHandle->SetPosition(NewFile->FileHandle, 0);

	Print(L"LibFileInfo()\n");
	NewFile->FileInfo = LibFileInfo(NewFile->FileHandle);

	// FileHandle->Read only reads directory's child files, not its own
	//NewFile->FileInfo = malloc(MAX_FILE_INFO_SIZE);
	//if (NewFile->FileInfo == NULL) {
	//	free(NewFile->FileInfo);
	//	free(NewFile);
	//	return NULL;
	//}
	//UINTN FileInfoSize = MAX_FILE_INFO_SIZE;
	//status = NewFile->FileHandle->Read(NewFile->FileHandle, &FileInfoSize, (VOID*)NewFile->FileInfo);
	//if (EFI_ERROR(status)) {
	//	free(NewFile->FileInfo);
	//	free(NewFile);
	//	return NULL;
	//}


	UINTN lastFilePathLength = strlen16(pwd->Path);
	UINTN newFileNameLength = strlen16(NewFile->FileInfo->FileName);
	if (lastFilePathLength + newFileNameLength + 2 >= MAX_PATH_LENGTH)
	{
		free(NewFile->FileInfo);
		free(NewFile);
		return NULL;
	}
	memcpy16(NewFile->Path, pwd->Path, lastFilePathLength);
	NewFile->Path[lastFilePathLength] = '\\';
	memcpy16(NewFile->Path + lastFilePathLength + 1, NewFile->FileInfo->FileName, newFileNameLength);
	NewFile->Path[lastFilePathLength + newFileNameLength + 1] = 0;

	Print(L"FileDevicePath() %x %s\n", pwd->Drive->Handle, NewFile->Path);
	NewFile->DevicePath = FileDevicePath(pwd->Drive->Handle, NewFile->Path);
	if (NewFile->DevicePath == NULL) {
		free(NewFile->FileInfo);
		free(NewFile);
		return NULL;
	}

	Print(L"dirtool_cd() done %s\n", NewFile->Path);
	return NewFile;
}

// also cd, but with string path
DIRTOOL_FILE* dirtool_cd_multi(DIRTOOL_FILE* pwd, CHAR16* Path)
{
	const CHAR16 split = '\\';
	// Print(L"dirtool_cd_multi() %s\n", Path);

	if (pwd == NULL) return NULL;
	DIRTOOL_FILE* ret = pwd;

	while(*Path != 0)
	{
		Print(L"dirtool_cd_multi() Path=%s, Path[0]=%u, Len=%u\n", Path, Path[0], strlen16(Path));
		while (*Path == split) ++Path;
		UINTN i;
		for (i = 0; Path[i] != split && Path[i] != 0; ++i) {};
		Print(L"dirtool_cd_multi() len=%u\n", i);
		CHAR16* PathSegment = malloc(sizeof(CHAR16) * (i + 1));
		memcpy16(PathSegment, Path, i);
		PathSegment[i] = 0;
		Print(L"dirtool_cd_multi() PathSegment=%s\n", PathSegment);
		ret = dirtool_cd(ret, PathSegment);

		Path += i; // do not +1 since it will go over the boundary in the final round
		if (ret == NULL) break; // file does not exist
	}

	if (ret == NULL)
	{
		// this is bad, we need to clean up
		if (pwd->ChildFile) __dirtool_release_file(pwd->ChildFile);
	}
	return ret;
}

// dispose a DIRTOOL_FILE (and all of its ChildFiles)
EFI_STATUS __dirtool_release_file(DIRTOOL_FILE* file)
{
	// Print(L"__dirtool_release_file() Path=%s\n", file->Path);

	if (file->ChildFile) __dirtool_release_file(file->ChildFile);
	file->ChildFile = NULL;

	if (file->DevicePath != NULL) free(file->DevicePath);
	if (file->FileInfo != NULL) free(file->FileInfo);

	file->ParentFolder->ChildFile = NULL;
	free(file);

	return EFI_SUCCESS;
}

// cd .. (and dispose all its ChildFiles)
DIRTOOL_FILE* dirtool_go_up(DIRTOOL_FILE* pwd)
{
	if (pwd->ParentFolder == NULL) return pwd; // we are at root and it cannot be released
	DIRTOOL_FILE* parent = pwd->ParentFolder;
	__dirtool_release_file(pwd);
	return parent;
}

// close a drive, release the whole directory tree
void dirtool_close_drive(DIRTOOL_STATE* state, DIRTOOL_DRIVE* drive)
{
	// first release all files made by cd
	DIRTOOL_FILE *LastFile = &(drive->RootFile);
	while (LastFile->ChildFile) LastFile = LastFile->ChildFile;
	while (LastFile->ParentFolder != NULL) LastFile = dirtool_go_up(LastFile);

	// de-init drive
	// do not re-init EFI_DEVICE_PATH_PROTOCOL; otherwise FileDevicePath() will die
	/*if (drive->RootFile.DevicePath != NULL) {
		free(drive->RootFile.DevicePath);
		drive->RootFile.DevicePath = NULL;
	}*/
	if (drive->RootFile.FileInfo != NULL) {
		free(drive->RootFile.FileInfo);
		drive->RootFile.FileInfo = NULL;
	}

	drive->isOpened = false;
}

// dispose a DIRTOOL_STATE
EFI_STATUS dirtool_deinit(DIRTOOL_STATE* state)
{
	if (!state->initialized) return EFI_UNSUPPORTED;

	// release drives
	for (UINTN i = 0; i < state->DriveCount; ++i)
	{
		dirtool_close_drive(state, &(state->Drives[i]));
	}
	free(state->Drives);

	// release itself
	state->DriveCount = 0;
	state->initialized = false;

	free(state);

	return EFI_SUCCESS;
}

// read a file into buffer; caller should free() after use
CHAR8 *dirtool_read_file(DIRTOOL_FILE *file)
{
	if (!file)
	{
		Print(L"%Edirtool_read_file() got an empty parameter%N\n");
		return NULL;
	}
	if (file->FileInfo->Attribute & EFI_FILE_DIRECTORY) {
		Print(L"%Edirtool_read_file() got a directory%N\n");
		return NULL;
	}

	EFI_STATUS Status;
	EFI_FILE_HANDLE File;

	Print(L"dirtool_read_file() %s\n", file->FileInfo->FileName);

	// read the file into a buffer
	Status = file->ParentFolder->FileHandle->Open(file->ParentFolder->FileHandle, &File, file->FileInfo->FileName, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(Status)) {
		return NULL;
	}

	// reset position just in case
	File->SetPosition(File, 0);

	// TODO: Size, FileSize, PhysicalSize?
	UINTN bufsize = file->FileInfo->FileSize;
	CHAR8* buf = malloc(bufsize);

	Print(L"dirtool_read_file() buf_size=%u\n", bufsize);
	Status = File->Read(File, &bufsize, buf);
	Print(L"dirtool_read_file() buf_size=%u\n", bufsize);

	file->ParentFolder->FileHandle->Close(File);

	return buf;
}