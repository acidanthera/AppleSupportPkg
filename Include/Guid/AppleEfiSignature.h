/** @file

AppleEfiSignature structure

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_EFI_SIGNATURE_H
#define APPLE_EFI_SIGNATURE_H

#define APPLE_EFI_SIGNATURE_GUID\
  { 0x45E7BC51, 0x913C, 0x42AC, {0x96, 0xA2, 0x10, 0x71, 0x2F, 0xFB, 0xEB, 0xA7 } }

typedef struct APPLE_SIGNATURE_DIRECTORY_ {
  UINT32    ImageSize;
  UINT32    SignatureDirectorySize;
  UINT32    SignatureSize;
  UINT16    CompressionType;
  UINT16    EfiSignature;
  EFI_GUID  AppleSignatureGuid;
  EFI_GUID  CertType;
  UINT8     PublicKey[256];
  UINT8     Signature[256];
}  APPLE_SIGNATURE_DIRECTORY;

extern EFI_GUID gAppleEfiSignatureGuid;

#endif //APPLE_EFI_SIGNATURE_H
