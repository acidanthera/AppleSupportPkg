/** @file

Apfs specific GUIDs for UEFI Variable Storage, version 1.0.

Copyright (c) 2017-2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APFS_CONTAINER_INFO_H_
#define APFS_CONTAINER_INFO_H_

#define APFS_CONTAINER_PARTITION_TYPE_GUID \
  { 0x7C3457EF, 0x0000, 0x11AA, {0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC } }

extern EFI_GUID mApfsContainerPartitionTypeGuid;

#endif // APPLE_APFS_CONTAINER_INFO_H_