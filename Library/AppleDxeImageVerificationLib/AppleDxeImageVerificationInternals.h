/** @file

AppleDxeImageVerificationLib

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_DXE_IMAGE_VERIFICATION_INTERNALS_H
#define APPLE_DXE_IMAGE_VERIFICATION_INTERNALS_H

#include <Base.h>
#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DebugSupport.h>
#include "Sha256.h"
#include "Rsa2048Sha256.h"
#include <IndustryStandard/PeImage.h>

#define APPLE_SIGNATURE_SECENTRY_SIZE 8

//
// The context structure used while PE/COFF image is being loaded and relocated.
//
typedef struct APPLE_PE_COFF_LOADER_IMAGE_CONTEXT_ {
    UINT64                           ImageAddress;
    UINT64                           ImageSize;
    UINT64                           EntryPoint;
    UINT64                           SizeOfHeaders;
    UINT16                           ImageType;
    UINT16                           NumberOfSections;
    UINT32                           *OptHdrChecksum;
    UINT32                           SizeOfOptionalHeader;
    UINT64                           SumOfSectionBytes;
    EFI_IMAGE_SECTION_HEADER         *FirstSection;
    EFI_IMAGE_DATA_DIRECTORY         *RelocDir;
    EFI_IMAGE_DATA_DIRECTORY         *SecDir;
    UINT64                           NumberOfRvaAndSizes;
    UINT16                           PeHdrMagic;
    EFI_IMAGE_OPTIONAL_HEADER_UNION  *PeHdr;
} APPLE_PE_COFF_LOADER_IMAGE_CONTEXT;

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

#endif //APPLE_DXE_IMAGE_VERIFICATION_INTERNALS_H
