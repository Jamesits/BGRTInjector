#include <efi.h>
#include "acpi_checksum.h"

// ACPI checksum algorithm: all the bytes in the table should sum to 0 including chksum byte
// return 0 if checksum passes, otherwise checksum fails
// if you have modified an ACPI table and is re-calculating checksum, just do table->chksum-=AcpiChecksum(table, table->header->size)
UINT8 AcpiChecksum(UINT8* TablePtr, UINT32 size)
{
	UINT8 ChkSum = 0;

	for (; size > 0; --size, ++TablePtr)
	{
		ChkSum += (*TablePtr);
	}

	return ChkSum;
}
