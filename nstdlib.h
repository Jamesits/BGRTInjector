#pragma once
#ifndef __MEMORY_MANAGEMENT_H__
#define __MEMORY_MANAGEMENT_H__

#include <efi.h>
#include <efilib.h>

void* malloc(UINTN poolSize);
void* malloc_acpi(UINTN poolSize);
void free(void* pool);
void memcpy8(CHAR8* dst, CHAR8* src, UINTN size);
void memcpy16(CHAR16* dst, CHAR16* src, UINTN size);
UINTN strlen8(CHAR8* str);
UINTN strlen16(CHAR16* str);
UINTN strncmp8(CHAR8* s1, CHAR8* s2, UINTN len); 
void ClearInputBuf();
void pause();
#endif
