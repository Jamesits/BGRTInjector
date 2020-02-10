// ACPI table definition from https://github.com/tianocore/edk2
#pragma once
#ifndef __BGRT_H__
#define __FADT__BGRT_H___H__

#include <efi.h>
#include <efilib.h>

#pragma pack(1)
/// edk2/blob/master/MdePkg/Include/IndustryStandard/Acpi10.h
///
/// The common ACPI description table header.  This structure prefaces most ACPI tables.
///
typedef struct
{
	UINT32 Signature;
	UINT32 Length;
	UINT8 Revision;
	UINT8 Checksum;
	UINT8 OemId[6];
	UINT64 OemTableId;
	UINT32 OemRevision;
	UINT32 CreatorId;
	UINT32 CreatorRevision;
} EFI_ACPI_DESCRIPTION_HEADER;

/// edk2/blob/master/MdePkg/Include/IndustryStandard/Acpi50.h
///
/// Boot Graphics Resource Table definition.
///
typedef struct
{
	EFI_ACPI_DESCRIPTION_HEADER Header;
	///
	/// 2-bytes (16 bit) version ID. This value must be 1.
	///
	UINT16 Version;
	///
	/// 1-byte status field indicating current status about the table.
	///     Bits[7:1] = Reserved (must be zero)
	///     Bit [0] = Valid. A one indicates the boot image graphic is valid.
	///
	UINT8 Status;
	///
	/// 1-byte enumerated type field indicating format of the image.
	///     0 = Bitmap
	///     1 - 255  Reserved (for future use)
	///
	UINT8 ImageType;
	///
	/// 8-byte (64 bit) physical address pointing to the firmware's in-memory copy
	/// of the image bitmap.
	///
	UINT64 ImageAddress;
	///
	/// A 4-byte (32-bit) unsigned long describing the display X-offset of the boot image.
	/// (X, Y) display offset of the top left corner of the boot image.
	/// The top left corner of the display is at offset (0, 0).
	///
	UINT32 ImageOffsetX;
	///
	/// A 4-byte (32-bit) unsigned long describing the display Y-offset of the boot image.
	/// (X, Y) display offset of the top left corner of the boot image.
	/// The top left corner of the display is at offset (0, 0).
	///
	UINT32 ImageOffsetY;
} EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE;

///
/// BGRT Revision
///
#define EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE_REVISION 1

///
/// BGRT Version
///
#define EFI_ACPI_5_0_BGRT_VERSION         0x01

///
/// BGRT Status
///
#define EFI_ACPI_5_0_BGRT_STATUS_NOT_DISPLAYED 0x00
#define EFI_ACPI_5_0_BGRT_STATUS_DISPLAYED     0x01
#define EFI_ACPI_5_0_BGRT_STATUS_INVALID       EFI_ACPI_5_0_BGRT_STATUS_NOT_DISPLAYED
#define EFI_ACPI_5_0_BGRT_STATUS_VALID         EFI_ACPI_5_0_BGRT_STATUS_DISPLAYED

///
/// BGRT Image Type
///
#define EFI_ACPI_5_0_BGRT_IMAGE_TYPE_BMP  0x00

#pragma pack()
#endif
