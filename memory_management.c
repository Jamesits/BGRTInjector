#include "memory_management.h"

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

void free(void* pool)
{
	EFI_STATUS status = uefi_call_wrapper(BS->FreePool, 1, pool);

    if (status == EFI_INVALID_PARAMETER)
    {
        Print(L"free() INVALID_PARAMETER\n");
    }
}

void memcpy2(CHAR8* dst, CHAR8* src, UINTN size)
{
	for (UINTN i = 0; i < size; ++i)
	{
        dst[i] = src[i];
	}
}