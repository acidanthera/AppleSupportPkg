/** @file

APFS Driver Loader - loads apfs.efi from EfiBootRecord block

Copyright (c) 2017-2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APFS_DRIVER_LOADER_H_
#define APFS_DRIVER_LOADER_H_

#include <Base.h>
#include <Uefi.h>
#include <Uefi/UefiGpt.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DebugSupport.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/DiskIo.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/PartitionInfo.h>
#include <Protocol/ApplePartitionInfo.h>
#include "NullTextOutputProtocol.h"

//
// Container Superblock magic
// 'NXSB'
//
STATIC CONST UINT32 CsbMagic           = 0x4253584e;
//
// Volume Superblock magic
// 'APSB'
//
STATIC CONST UINT32 VsbMagic           = 0x42535041;
//
// EfiBootRecord block magic
// 'JSDR'
//
STATIC CONST UINT32 EfiBootRecordMagic = 0x5244534a;

#pragma pack(push, 1)
typedef struct APFS_BLOCK_HEADER_
{
    //
    // Fletcher checksum, 64-bit. All metadata blocks
    //
    UINT64             Checksum;
    //
    // Probably plays a role in the Btree structure NXSB=01 00
    // APSB=02 04, 06 04 and 08 04
    // nid
    //
    UINT64             NodeId;
    //
    // Checkpoint Id ( transaction id )
    // xid
    //
    UINT64             CheckpointId;
    //
    // Node type:
    // 0x00000002:  Directory Root
    // 0x00000003:  Directory
    // 0x0000000D:  Volume Superblock
    // 0x40000002:  Mapping Root
    // 0x40000003:  Mapping
    // 0x40000007:  Bitmap Block List
    // 0x4000000B:  Pointer to Header
    // 0x4000000C:  Another Mapping
    // 0x40000014:  EfiBootRecord Block
    // 0x80000001:  Container Superblock
    // 0x80000002:  Unknown
    // 0x80000003:  Unknown
    // 0x80000005:  Unknown
    // 0x80000011:  Unknown
    //
    UINT32             NodeType;
    //
    // ????
    //
    UINT16             ContentType;
    //
    // Just a padding
    // Unknown behavior
    //
    UINT16             Padding;
} APFS_BLOCK_HEADER;
#pragma pack(pop)

/**
  NXSB Container Superblock
  The container superblock is the entry point to the filesystem.
  Because of the structure with containers and flexible volumes,
  allocation needs to handled on a container level.
  The container superblock contains information on the blocksize,
  the number of blocks and pointers to the spacemanager for this task.
  Additionally the block IDs of all volumes are stored in the superblock.
  To map block IDs to block offsets a pointer to a block map b-tree is stored.
  This b-tree contains entries for each volume with its ID and offset.
**/
#pragma pack(push, 1)
typedef struct APFS_NXSB_
{
    APFS_BLOCK_HEADER  BlockHeader;
    //
    // Magic: NXSB
    //
    UINT32             MagicNumber;
    //
    // Size of each allocation unit: 4096 bytes
    // (by default)
    //
    UINT32             BlockSize;
    //
    // Number of blocks in the container
    //
    UINT64             TotalBlocks;
    UINT8              Reserved_1[24];
    //
    // GUID of the container, must be equal APFS_CONTAINER_GUID
    //
    EFI_GUID           Guid;
    //
    // Next free block id
    //
    UINT64             NextFreeBlockId;
    //
    // What is the next CSB id
    //
    UINT64             NextCsbNodeId;
    UINT64             Reserved_2;
    //
    // The base block is used to calculate current and previous CSBD/ CSB.
    //
    UINT32             BaseBlock;
    UINT32             Reserved_3[3];
    //
    // This is the block where the CSBD from previous state is found and is 
    // located in block "Base block" + PreviousCsbdInBlock. The CSBD for 
    // previous state is in block PreviousCsbdInBlock+1 and the CSB for 
    // the same state in block PreviousCsbdInBlock+2
    //
    UINT32             PreviousCsbdInBlock;
    UINT32             Reserved_4;
    //
    // The current state CSBD is located in block "Base block" in offset 0x70,
    // 0x01 + OriginalCsbdInBlock. The CSBD for the current state of the file 
    // system is in block 0x01 + OriginalCsbdInBlock. The original CSB is in 
    // the succeeding block, 0x01 + OriginalCsbdInBlock.
    //
    UINT32             OriginalCsbdInBlock;
    //
    // Oldest CSBD in block "Base block" + 0x02. The oldest CSBD is in block 
    // 0x03 and the CSB for that state is in the succeeding block. OldestCsbd +  
    //  "Base block".
    //
    UINT32             OldestCsbd;
    UINT64             Reserved_5;
    UINT64             SpacemanId;
    UINT64             BlockMapBlock;
    UINT64             UnknownId;
    UINT32             Reserved_6;
    //
    // Count of Volume IDs
    // (by default 100)
    //
    UINT32             ListOfVolumeIds;
    //
    // Array of 8 byte VolumeRootIds
    // Length of array - ListOfVolumeIds
    //
    UINT64             VolumesRootIds[100];
    UINT64             UnknownBlockId;
    UINT8              Reserved_7[280];
    //
    // Pointer to JSDR block (EfiBootRecordBlock)
    //
    UINT64   EfiBootRecordBlock;
} APFS_NXSB;
#pragma pack(pop)

//
// APSB volume header structure
//
#pragma pack(push, 1)
typedef struct APFS_APSB_
{
    APFS_BLOCK_HEADER  BlockHeader;
    //
    // Volume Superblock magic
    // Magic: APSB
    //
    UINT32             MagicNumber;
    //
    // Volume#. First volume start with 0, (0x00) 
    //
    UINT32             VolumeNumber;
    UINT8              Reserved_1[20];
    //
    // Case setting of the volume.
    // 1 = Not case sensitive
    // 8 = Case sensitive (0x01, Not C.S)
    //
    UINT32             CaseSetting;
    UINT8              Reserved_2[12];
    //
    // Size of volume in Blocks. Last volume has no
    // size set and has available the rest of the blocks
    //
    UINT64             VolumeSize;
    UINT64             Reserved_3;
    // 
    // Blocks in use in this volumes 
    //
    UINT64             BlocksInUseCount;
    UINT8              Reserved_4[32];
    //
    // Block# to initial block of catalog B-Tree Object
    // Map (BTOM)
    //
    UINT64             BlockNumberToInitialBTOM;
    //
    // Node Id of root-node 
    //
    UINT64             RootNodeId;
    //
    // Block# to Extents B-Tree,block#
    //
    UINT64             BlockNumberToEBTBlockNumber;
    //
    // Block# to list of Snapshots
    //
    UINT64             BlockNumberToListOfSnapshots;
    UINT8              Reserved_5[16];
    //
    // Next CNID
    //
    UINT64             NextCnid;
    //
    // Number of files on the volume
    //
    UINT64             NumberOfFiles;
    //
    // Number of folders on the volume
    //
    UINT64             NumberOfFolder;
    UINT8              Reserved_6[40];
    //
    // Volume UUID
    //
    EFI_GUID           VolumeUuid;
    //
    // Time Volume last written/modified
    //
    UINT64             ModificationTimestamp;
    UINT64             Reserved_7;
    //
    // Creator/APFS-version 
    // Ex. (hfs_convert (apfs- 687.0.0.1.7))
    //
    UINT8              CreatorVersionInfo[32];
    //
    // Time Volume created 
    //
    UINT64             CreationTimestamp;
    //
    // ???
    //
} APFS_APSB;
#pragma pack(pop)

//
// JSDR block structure
//
#pragma pack(push, 1)
typedef struct APFS_EFI_BOOT_RECORD_
{
    APFS_BLOCK_HEADER  BlockHeader;
    //
    // EfiBootRecord magic
    // Magic: JSDR
    //
    UINT32             MagicNumber;
    //
    // EfiBootRecord version
    // should be 1
    //
    UINT32             Version;
    UINT8              Reserved2[136];
    //
    // Apfs driver start LBA
    //
    UINT64             BootRecordLBA;
    //
    // Apfs driver size
    //
    UINT64             BootRecordSize;
} APFS_EFI_BOOT_RECORD;
#pragma pack(pop)

extern EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL mNullTextOutputProtocol;

extern
UINT64
ApfsBlockChecksumCalculate (
  UINT32  *Data,
  UINTN  DataSize
  );

extern
BOOLEAN
ApfsBlockChecksumVerify (
  UINT8   *Data,
  UINTN  DataSize
  );

#endif // APFS_DRIVER_LOADER_H_
