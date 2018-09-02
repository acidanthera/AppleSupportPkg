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

#define APPLE_FILESYSTEM_EFIBOOTRECORD_INFO_PROTOCOL_GUID \
  { 0x03B8D751, 0xA02F, 0x4FF8, {0x9B, 0x1A, 0x55, 0x24, 0xAF, 0xA3, 0x94, 0x5F } }

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

extern EFI_GUID gAppleFileSystemEfiBootRecordInfoProtocolGuid;

#endif // APPLE_FILESYSTEM_EFIBOOTRECORD_INFO_PROTOCOL_H_
