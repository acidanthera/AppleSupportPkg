/** @file

Apple LoadImage protocol. 

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_LOAD_IMAGE_PROTOCOL_H_
#define APPLE_LOAD_IMAGE_PROTOCOL_H_

//
// If this protocol not present, then ApfsJumpStart from Apple firmware,
// couldn't load apfs.efi into memory.
// So this wrapper gives you availability to load native driver.
//
#define APPLE_LOAD_IMAGE_PROTOCOL_GUID \
  { 0x6C6148A4, 0x97B8, 0x429C, {0x95, 0x5E, 0x41, 0x03, 0xE8, 0xAC, 0xA0, 0xFA } }

typedef EFI_STATUS (EFIAPI *APPLE_LOAD_IMAGE) (
  IN BOOLEAN                  BootPolicy,
  IN EFI_HANDLE               ParentImageHandle,
  IN EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  IN VOID                     *SourceBuffer,
  IN UINTN                    SourceSize,
  IN EFI_HANDLE               *ImageHandle,
  IN  UINT64                  Version
);

typedef struct {
  APPLE_LOAD_IMAGE   LoadImage;
} APPLE_LOAD_IMAGE_PROTOCOL;

extern EFI_GUID gAppleLoadImageProtocolGuid;

#endif // APPLE_LOAD_IMAGE_PROTOCOL_H_
