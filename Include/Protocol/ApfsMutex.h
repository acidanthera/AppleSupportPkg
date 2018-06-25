/** @file

Apple FileSystem Mutex protocol used in Driverbinding

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_FILESYSTEM_MUTEX_PROTOCOL_H_
#define APPLE_FILESYSTEM_MUTEX_PROTOCOL_H_


#define APPLE_FILESYSTEM_MUTEX_PROTOCOL_GUID \
  { 0x03B8D751, 0x0A02, 0x4FF8, {0xB9, 0x1A, 0x55, 0x24, 0xAF, 0xA3, 0x94, 0x5F } }

extern EFI_GUID gAppleFileSystemMutexProtocolGuid;

#endif // APPLE_FILESYSTEM_MUTEX_PROTOCOL_H_