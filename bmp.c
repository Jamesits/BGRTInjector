#include "bmp.h"
#include "nstdlib.h"

// do basic sanity check on an BMP image
// if FileSize == 0 then bypass file length check (useful when the file is compiled in)
// return true if check passed
// return false if something is wrong
bool bmp_sanity_check(CHAR8 *buf, UINTN FileSize)
{
	// check BMP magic
	if (strncmp8((CHAR8*)"BM", buf, 2))
	{
		Print(L"%HImage file format not recognized%N\n");
		return false;
	}

	// check incorrect BMP header size to prevent buffer overflow to some extent
	BMP_IMAGE_HEADER* bmp = (BMP_IMAGE_HEADER*)buf;
	if (FileSize != 0 && FileSize != bmp->bfSize)
	{
		Print(L"%HImage file size incorrect%N\n");
		return false;
	}

	// TODO: check if image is 24bit or 32bit

	return true;
}