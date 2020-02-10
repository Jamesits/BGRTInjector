#include <efi.h>
#include <efilib.h>
#include "acpi_checksum.h"

// return 0 if checksum passes
UINT8 AcpiChecksum(UINT8* TablePtr, UINT32 size)
{
	UINT8 ChkSum = 0;

	for (; size > 0; --size, ++TablePtr)
	{
		ChkSum += (*TablePtr);
	}

	return ChkSum;
}
