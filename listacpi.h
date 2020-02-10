#pragma once
#ifndef __LISTACPI_H__
#define __LISTACPI_H__

typedef struct
{
	CHAR8 Signature[8];
	UINT8 Checksum;
	UINT8 OemId[6];
	UINT8 Revision;
	UINT32 RsdtAddress;
	UINT32 Length;
	UINT64 XsdtAddress;
	UINT8 ExtendedChecksum;
	UINT8 Reserved[3];
} EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER;


// XSDT is the main System Description Table. 
// There are many kinds of SDT. An SDT may be split into two parts - 
// A common header and a data section which is different for each table.
typedef struct
{
	CHAR8 Signature[4];
	UINT32 Length;
	UINT8 Revision;
	UINT8 Checksum;
	CHAR8 OemId[6];
	CHAR8 OemTableId[8];
	UINT32 OemRevision;
	UINT32 CreatorId;
	UINT32 CreatorRevision;
} EFI_ACPI_SDT_HEADER;


#define EFI_ACPI_TABLE_GUID \
    { 0xeb9d2d30, 0x2d88, 0x11d3, {0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d }}
#define EFI_ACPI_20_TABLE_GUID \
    { 0x8868e871, 0xe4f1, 0x11d3, {0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 }}

#define EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION 0x02

UINTN
myStrnCmpA(CHAR8* s1, CHAR8* s2, UINTN len);

VOID
Ascii2UnicodeStr(CHAR8* String, CHAR16* UniString, UINT8 length);

int
ParseRSDP(EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER* Rsdp, CHAR16* GuidStr);

VOID
Guid2String(CHAR16* Buffer, EFI_GUID* Guid);

#endif
