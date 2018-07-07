/** @file

Apple FileSystem EfiBootRecord container info

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_FILESYSTEM_EFIBOOTRECORD_INFO_PROTOCOL_H_
#define APPLE_FILESYSTEM_EFIBOOTRECORD_INFO_PROTOCOL_H_

#define APPLE_FILESYSTEM_DRIVER_INFO_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('A', 'F', 'J', 'S')

#define APPLE_FILESYSTEM_EFIBOOTRECORD_INFO_PROTOCOL_GUID \
  { 0x03B8D751, 0xA02F, 0x4FF8, {0x9B, 0x1A, 0x55, 0x24, 0xAF, 0xA3, 0x94, 0x5F } }


typedef struct _UNKNOWNFIELD
{
    UINT32                                       Unknown1;
    EFI_HANDLE                                   Handle;
    EFI_HANDLE                                   AgentHandle;
    UINT8                                        Unknown2[88];
    UINT64                                       Unknown3;
} UNKNOWNFIELD;


typedef struct  _APPLE_FILESYSTEM_EFIBOOTRECORD_LOCATION_INFO
{
	//
	// Handle of partition which contain EfiBootRecord section
	//
    EFI_HANDLE                                  ControllerHandle;
    //
    // UUID of GPT container partition
    //
    EFI_GUID                                    ContainerUuid; 
} APPLE_FILESYSTEM_EFIBOOTRECORD_LOCATION_INFO;

//
// ApfsJumpStart private data structure gathered from
// reverse-engineered Apple's implementation
//
typedef struct _APPLE_FILESYSTEM_DRIVER_INFO_PRIVATE_DATA 
{
    UINT32                                       Magic;
    EFI_HANDLE                                   ControllerHandle;
    EFI_HANDLE                                   DriverBindingHandle;
    APPLE_FILESYSTEM_EFIBOOTRECORD_LOCATION_INFO EfiBootRecordLocationInfo;
    UINT8                                        Unknown1[24];
    EFI_EVENT                                    NotifyEvent;
    VOID                                         *ApfsDriverPtr;
    UINT32                                       ApfsDriverSize;
    UINT32                                       ContainerBlockSize;
    UINT64                                       ContainerTotalBlocks;
    UINT8                                        Unknown2[4];
    UINT32                                       Unknown3;
    EFI_BLOCK_IO_PROTOCOL                        *BlockIoInterface;
    UNKNOWNFIELD                                 *Unknown4;
    UINT64                                       UnknownAddress;
} APPLE_FILESYSTEM_DRIVER_INFO_PRIVATE_DATA;


#define APPLE_FILESYSTEM_EFIBOOTRECORD_INFO_PRIVATE_DATA_FROM_THIS(a) \
          CR(a, APPLE_FILESYSTEM_DRIVER_INFO_PRIVATE_DATA, EfiBootRecordLocationInfo, APPLE_FILESYSTEM_DRIVER_INFO_PRIVATE_DATA_SIGNATURE)

extern EFI_GUID gAppleFileSystemEfiBootRecordInfoProtocolGuid;

#endif // APPLE_FILESYSTEM_EFIBOOTRECORD_INFO_PROTOCOL_H_
