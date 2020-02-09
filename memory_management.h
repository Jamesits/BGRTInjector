#pragma once
#ifndef __MEMORY_MANAGEMENT_H__
#define __MEMORY_MANAGEMENT_H__

#include <efi.h>
#include <efilib.h>

void* malloc(UINTN poolSize);
void* malloc_acpi(UINTN poolSize);
void free(void* pool);
void memcpy2(CHAR8* dst, CHAR8* src, UINTN size);

#endif