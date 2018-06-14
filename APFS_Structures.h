/*

APFS Driver Loader - loads apfs.efi from JSDR section in container

Copyright (c) 2017-2018, savvas
Copyright (c) 2018, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef APFS_STRUCTURES_H_
#define APFS_STRUCTURES_H_

#include <Base.h>
#include <Uefi.h>
#include <Uefi/UefiGpt.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DebugPort.h>
#include <Protocol/DebugSupport.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/DiskIo.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/LoadedImage.h>

//
// NXSB magic number
//
STATIC CONST UINT32 NXSB_MN = 0x4253584e;
//
// APSB magic number
//
STATIC CONST UINT32 APSB_MN = 0x42535041;
//
// JSDR magic number
//
STATIC CONST UINT32 JSDR_MN = 0x5244534a;

//
// APFS Container GUID
//
STATIC CONST EFI_GUID APFS_Container_GUID = \
  {0x7C3457EF, 0x0000, 0x11AA, \
  {0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC}};

/* NXSB Container Superblock
 * The container superblock is the entry point to the filesystem.
 * Because of the structure with containers and flexible volumes,
 * allocation needs to handled on a container level.
 * The container superblock contains information on the blocksize,
 * the number of blocks and pointers to the spacemanager for this task.
 * Additionally the block IDs of all volumes are stored in the superblock.
 * To map block IDs to block offsets a pointer to a block map b-tree is stored.
 * This b-tree contains entries for each volume with its ID and offset.
 */
#pragma pack(push, 1)
typedef struct APFS_NXSB_
{
    UINT64   Checksum;
    UINT64   BlockId;
    UINT64   Version;
    UINT16   BlockType;
    UINT16   Flags;
    UINT16   VarType;
    UINT16   Reserved;
    UINT32   MagicNumber;
    UINT32   BlockSize;
    UINT64   TotalBlocks;
    UINT64   Reserved_1[3];
    EFI_GUID Guid;
    UINT64   NextFreeBlockId;
    UINT64   NextVersion;
    UINT64   Reserved_2[4];
    UINT32   PreviousContainerSBBlock;
    UINT8    Reserved_3[12];
    UINT64   SpacemanId;
    UINT64   BlockMapBlock;
    UINT64   UnknownId;
    UINT32   Reserved_4;
    UINT32   APFSPartitionCount;
    UINT64   OffsetAPFS;
} APFS_NXSB;
#pragma pack(pop)

//
// APSB volume header structure
//
#pragma pack(push, 1)
typedef struct APFS_APSB_
{
    UINT64   Checksum;
    UINT64   BlockId;
    UINT64   Version;
    UINT16   BlockType;
    UINT16   Flags;
    UINT16   VarType;
    UINT16   Reserved;
    UINT32   MagicNumber;
    UINT8    Reserved_1[92];
    UINT64   BlockMap;
    UINT64   RootDirId;
    UINT64   Pointer3;
    UINT64   Pointer4;
    UINT8    Reserved_2[80];
    EFI_GUID Guid;
    UINT64   Time1;
    UINT8    Reserved_3[40];
    UINT64   Time2;
    UINT8    Reserved_4[392];
    UINT8    PartitionName[8];
} APFS_APSB;
#pragma pack(pop)

//
// JSDR block structure
//
#pragma pack(push, 1)
typedef struct APFS_EFI_BOOT_RECORD_
{
    UINT64 Checksum;
    UINT64 BlockId;
    UINT64 Version;
    UINT16 BlockType;
    UINT16 Flags;
    UINT16 VarType;
    UINT16 Reserved1;
    UINT32 MagicNumber;
    UINT8  Reserved2[140];
    UINT64 BootRecordLBA;
    UINT64 BootRecordSize;
} APFS_EFI_BOOT_RECORD;
#pragma pack(pop)

#endif // APFS_STRUCTURES_H_