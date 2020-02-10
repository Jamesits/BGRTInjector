#pragma once
#ifndef __GOP_H__
#define __GOP_H__

#include <efi.h>
#include <efilib.h>
#include "acpi_dump.h"

EFI_GRAPHICS_OUTPUT_PROTOCOL*
GetGOP();

#endif
