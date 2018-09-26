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

#define APFS_DRIVER_INFO_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('A', 'F', 'J', 'S')

//
// Container Superblock magic
//
#define APFS_CSB_SIGNATURE  SIGNATURE_32 ('N', 'X', 'S', 'B')
//
// Volume Superblock magic
//
#define APFS_VSB_SIGNATURE  SIGNATURE_32 ('A', 'P', 'S', 'B')
//
// EfiBootRecord block magic
//
#define APFS_EFIBOOTRECORD_SIGNATURE  SIGNATURE_32 ('J', 'S', 'D', 'R')

typedef struct UNKNOWNFIELD_
{
  UINT32      Unknown1;
  EFI_HANDLE  Handle;
  EFI_HANDLE  AgentHandle;
  UINT8       Unknown2[88];
  UINT64      Unknown3;
} UNKNOWNFIELD;

//
// Private ApfsJumpStart structure
//
typedef struct _APFS_DRIVER_INFO_PRIVATE_DATA
{
  UINT32                            Magic;
  EFI_HANDLE                        ControllerHandle;
  EFI_HANDLE                        DriverBindingHandle;
  APFS_EFIBOOTRECORD_LOCATION_INFO  EfiBootRecordLocationInfo;
  UINT8                             Unknown1[24];
  EFI_EVENT                         NotifyEvent;
  VOID                              *ApfsDriverPtr;
  UINT32                            ApfsDriverSize;
  UINT32                            ContainerBlockSize;
  UINT64                            ContainerTotalBlocks;
  UINT8                             Unknown2[4];
  UINT32                            Unknown3;
  EFI_BLOCK_IO_PROTOCOL             *BlockIoInterface;
  UNKNOWNFIELD                      *Unknown4;
  UINT64                            UnknownAddress;
} APFS_DRIVER_INFO_PRIVATE_DATA;

#define APFS_EFIBOOTRECORD_INFO_PRIVATE_DATA_FROM_THIS(a) \
          CR(a, APFS_DRIVER_INFO_PRIVATE_DATA, EfiBootRecordLocationInfo, APFS_DRIVER_INFO_PRIVATE_DATA_SIGNATURE)

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
typedef struct APFS_CSB_
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
  // UUID of the container
  //
  EFI_GUID           Uuid;
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
} APFS_CSB;
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
  // should be 1 ?
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

#endif // APFS_DRIVER_LOADER_H_
