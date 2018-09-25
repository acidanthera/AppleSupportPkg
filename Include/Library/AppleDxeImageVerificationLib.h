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

#ifndef APPLE_DXE_IMAGE_VERIFICATION_H
#define APPLE_DXE_IMAGE_VERIFICATION_H

#include <IndustryStandard/PeImage.h>

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

//
// Function prototypes
//
UINT16
GetPeHeaderMagicValue (
  EFI_IMAGE_OPTIONAL_HEADER_UNION *Hdr
  );

EFI_STATUS
BuildPeContext (
  VOID                                *Image,
  UINT32                              ImageSize,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context
  );

VOID
SanitizeApplePeImage (
  VOID                                *Image,
  UINT32                              *RealImageSize,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context
  );

EFI_STATUS
GetApplePeImageSignature (
  VOID                               *Image,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT *Context,
  UINT8                              *PkLe,
  UINT8                              *PkBe,
  UINT8                              *SigLe,
  UINT8                              *SigBe
  );

EFI_STATUS
GetApplePeImageSha256 (
  VOID                                *Image,
  UINT32                              *ImageSize,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  UINT8                               *CalcucatedHash
  );

EFI_STATUS
VerifyApplePeImageSignature (
  IN OUT VOID                                *PeImage,
  IN OUT UINT32                              *ImageSize,
  IN OUT APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context OPTIONAL
  );

#endif //APPLE_DXE_IMAGE_VERIFICATION_H
