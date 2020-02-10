#include "nstdlib.h"

// from https://stackoverflow.com/a/51575086
void* malloc(UINTN poolSize)
{
	void* handle;
	EFI_STATUS status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, poolSize, &handle);

	if (status == EFI_OUT_OF_RESOURCES)
	{
		Print(L"malloc() OUT_OF_RESOURCES\n");
		return 0;
	}
	else if (status == EFI_INVALID_PARAMETER)
	{
		Print(L"malloc() INVALID_PARAMETER\n");
		return 0;
	}
	else
	{
		return handle;
	}
}

void* malloc_acpi(UINTN poolSize)
{
	void* handle;
	EFI_STATUS status = uefi_call_wrapper(BS->AllocatePool, 3, EfiACPIReclaimMemory, poolSize, &handle);

	if (status == EFI_OUT_OF_RESOURCES)
	{
		Print(L"malloc() OUT_OF_RESOURCES\n");
		return 0;
	}
	else if (status == EFI_INVALID_PARAMETER)
	{
		Print(L"malloc() INVALID_PARAMETER\n");
		return 0;
	}
	else
	{
		return handle;
	}
}

// from https://stackoverflow.com/a/51575086
void free(void* pool)
{
	EFI_STATUS status = uefi_call_wrapper(BS->FreePool, 1, pool);

	if (status == EFI_INVALID_PARAMETER)
	{
		Print(L"free() INVALID_PARAMETER\n");
	}
}

void memcpy8(CHAR8* dst, CHAR8* src, UINTN size)
{
	for (UINTN i = 0; i < size; ++i)
	{
		dst[i] = src[i];
	}
}

void memcpy16(CHAR16* dst, CHAR16* src, UINTN size)
{
	for (UINTN i = 0; i < size; ++i)
	{
		dst[i] = src[i];
	}
}

UINTN strlen8(CHAR8* str)
{
	int ret = 0;
	for (; str[ret]; ++ret){};
	return ret;
}

UINTN strlen16(CHAR16* str)
{
	int ret = 0;
	for (; str[ret]; ++ret) {};
	return ret;
}

UINTN strncmp8(CHAR8* s1, CHAR8* s2, UINTN len)
{
	while (*s1 && len)
	{
		if (*s1 != *s2)
		{
			break;
		}
		s1 += 1;
		s2 += 1;
		len -= 1;
	}

	return len ? *s1 - *s2 : 0;
}

void ClearInputBuf()
{
	// empty the console input buffer
	gST->ConIn->Reset(gST->ConIn, FALSE);
}

void pause()
{
	ClearInputBuf();

	// wait for a key
	UINTN Event;
	gST->BootServices->WaitForEvent(1, &gST->ConIn->WaitForKey, &Event);
}
