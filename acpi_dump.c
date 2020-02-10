// https://blog.fpmurphy.com/2015/01/list-acpi-tables-from-uefi-shell.html
// Copyright (c) 2015  Finnbarr P. Murphy. All rights reserved.
// Display list of ACPI tables 
// License: BSD License

#include <efi.h>
#include <efilib.h>
#include "acpi_dump.h"
#include "nstdlib.h"

VOID
Ascii2UnicodeStr(CHAR8* String, CHAR16* UniString, UINT8 length)
{
	int len = length;

	while (*String != '\0' && len > 0)
	{
		*(UniString++) = (CHAR16)* (String++);
		len--;
	}
	*UniString = '\0';
}


int
ParseRSDP(EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER* Rsdp, CHAR16* GuidStr)
{
	EFI_ACPI_SDT_HEADER *Xsdt, *Entry;
	CHAR16 SigStr[20], OemStr[20];
	UINT32 EntryCount;
	UINT64* EntryPtr;
	int Index;

	Print(L"\n\nACPI GUID: %s\n", GuidStr);
	Ascii2UnicodeStr((CHAR8*)(Rsdp->OemId), OemStr, 6);
	Print(L"\nFound RSDP. Version: %d  OEM ID: %s\n", (int)(Rsdp->Revision), OemStr);

	if (Rsdp->Revision >= EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION)
	{
		Xsdt = (EFI_ACPI_SDT_HEADER*)(Rsdp->XsdtAddress);
	}
	else
	{
		Print(L"ERROR: No XSDT table found.\n");
		return 1;
	}

	if (strncmp8("XSDT", (CHAR8*)(VOID*)(Xsdt->Signature), 4))
	{
		Print(L"ERROR: Invalid XSDT table found.\n");
		return 1;
	}

	Ascii2UnicodeStr((CHAR8*)(Xsdt->OemId), OemStr, 6);
	EntryCount = (Xsdt->Length - sizeof(EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);
	Print(L"Found XSDT. OEM ID: %s  Entry Count: %d\n\n", OemStr, EntryCount);

	EntryPtr = (UINT64*)(Xsdt + 1);
	for (Index = 0; Index < EntryCount; Index++, EntryPtr++)
	{
		Entry = (EFI_ACPI_SDT_HEADER*)((UINTN)(*EntryPtr));
		Ascii2UnicodeStr((CHAR8*)(Entry->Signature), SigStr, 4);
		Ascii2UnicodeStr((CHAR8*)(Entry->OemId), OemStr, 6);
		Print(L"Found ACPI table: %s  Version: %d  OEM ID: %s\n", SigStr, (int)(Entry->Revision), OemStr);
	}

	return 0;
}


VOID
Guid2String(CHAR16* Buffer, EFI_GUID* Guid)
{
	SPrint(Buffer, 0, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
	       Guid->Data1,
	       Guid->Data2,
	       Guid->Data3,
	       Guid->Data4[0],
	       Guid->Data4[1],
	       Guid->Data4[2],
	       Guid->Data4[3],
	       Guid->Data4[4],
	       Guid->Data4[5],
	       Guid->Data4[6],
	       Guid->Data4[7]);
}
