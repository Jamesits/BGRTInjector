// ACPI table definition from https://github.com/tianocore/edk2
#pragma once
#ifndef __XSDT_H__
#define __XSDT_H__

#include <efi.h>
#include <efilib.h>
#include "bgrt.h"

#pragma pack(1)

typedef struct {
    EFI_ACPI_DESCRIPTION_HEADER Header;
    UINT64 Entry[0];
} XSDT;

#pragma pack()
#endif