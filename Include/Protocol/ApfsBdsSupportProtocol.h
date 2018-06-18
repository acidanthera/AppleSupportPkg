/** @file

Apple FileSystem BDS stage protocol to inform Apfs Loader 
about Apfs boot availability.

Copyright (c) 2017-2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_FILESYSTEM_BDS_SUPPORT_PROTOCOL_H_
#define APPLE_FILESYSTEM_BDS_SUPPORT_PROTOCOL_H_

//
// If this protocol not present, then firmware doesn't support Apfs boot,
// so AppleApfsJumpStart won't load
//
#define APPLE_FILESYSTEM_BDS_SUPPORT_PROTOCOL_GUID \
  { 0xA196A7CA, 0x14C6, 0x11E7, {0xB9, 0x06, 0xB8, 0xE8, 0x56, 0x2C, 0xBA, 0xFA } }

extern EFI_GUID gAppleFileSystemBdsSupportProtocolGuid;

#endif // APPLE_FILESYSTEM_BDS_SUPPORT_PROTOCOL_H_