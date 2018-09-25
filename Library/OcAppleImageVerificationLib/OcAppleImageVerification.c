/** @file

AppleDxeImageVerificationLib
- Apple key-storage
- Apple Authenticode PE-256 hash calculation
- Verifying Rsa2048Sha256 signature

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Guid/AppleEfiSignature.h>
#include "OcAppleImageVerification.h"
#include "ApplePublicKeyDb.h"

UINT16
GetPeHeaderMagicValue (
  EFI_IMAGE_OPTIONAL_HEADER_UNION  *Hdr
  )
{
  /**
     NOTE: Some versions of Linux ELILO for Itanium have an incorrect magic value
           in the PE/COFF Header.  If the MachineType is Itanium(IA64) and the
           Magic value in the OptionalHeader is  EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC
           then override the returned value to EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC
  **/
  if (Hdr->Pe32.FileHeader.Machine == IMAGE_FILE_MACHINE_IA64 &&
      Hdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    return EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
  }

  //
  // Return the magic value from the PC/COFF Optional Header
  //
  return Hdr->Pe32.OptionalHeader.Magic;
}

EFI_STATUS
BuildPeContext (
  VOID                                *Image,
  UINT32                              ImageSize,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context
  )
{
  EFI_IMAGE_DOS_HEADER             *DosHdr              = NULL;
  EFI_IMAGE_OPTIONAL_HEADER_UNION  *PeHdr               = NULL;
  UINT16                           PeHdrMagic           = 0;
  UINT32                           HeaderWithoutDataDir = 0;
  UINT32                           SectionHeaderOffset  = 0;
  EFI_IMAGE_SECTION_HEADER         *SectionCache        = NULL;
  UINT32                           Index                = 0;
  UINT32                           MaxHeaderSize        = 0;

  //
  // Check context
  //
  if (Context == NULL) {
    DEBUG ((DEBUG_WARN, "Null context\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Verify Image size
  //
  if (sizeof (EFI_IMAGE_DOS_HEADER) >= sizeof (EFI_IMAGE_OPTIONAL_HEADER_UNION)) {
    MaxHeaderSize = sizeof (EFI_IMAGE_DOS_HEADER);
  } else {
    MaxHeaderSize = sizeof (EFI_IMAGE_OPTIONAL_HEADER_UNION);
  }
  if (ImageSize < MaxHeaderSize) {
    DEBUG ((DEBUG_WARN, "Invalid image\n"));
    return EFI_INVALID_PARAMETER;
  }

  DosHdr = Image;

  //
  // Verify DosHdr magic
  //
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    if (DosHdr->e_lfanew > ImageSize 
      || DosHdr->e_lfanew < sizeof (EFI_IMAGE_DOS_HEADER)) {
      DEBUG ((DEBUG_WARN, "Invalid PE offset\n"));
      return EFI_INVALID_PARAMETER;
    }

    PeHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *) ((UINT8 *) Image
                                                 + DosHdr->e_lfanew);
    if ((UINT8 *) Image + ImageSize -
      sizeof (EFI_IMAGE_OPTIONAL_HEADER_UNION) < (UINT8 *) PeHdr) {
      DEBUG ((DEBUG_WARN, "Invalid PE location\n"));
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // DosHdr truncated
    //
    PeHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *) Image;
  }

  PeHdrMagic = GetPeHeaderMagicValue (PeHdr);

  if (PeHdrMagic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Pe32 part
    //

    //
    // Check image header size
    //
    if (EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES <
        PeHdr->Pe32.OptionalHeader.NumberOfRvaAndSizes) {
      DEBUG ((DEBUG_WARN, "Image header too small\n"));
      return EFI_INVALID_PARAMETER;
    }

    //
    // Check image header aligment
    //
    HeaderWithoutDataDir = sizeof (EFI_IMAGE_OPTIONAL_HEADER32) -
                           sizeof (EFI_IMAGE_DATA_DIRECTORY) *
                           EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES;
    if (PeHdr->Pe32.FileHeader.SizeOfOptionalHeader < HeaderWithoutDataDir
      || PeHdr->Pe32.FileHeader.SizeOfOptionalHeader - HeaderWithoutDataDir
      != PeHdr->Pe32.OptionalHeader.NumberOfRvaAndSizes * sizeof (EFI_IMAGE_DATA_DIRECTORY)) {
      DEBUG ((DEBUG_WARN, "Image header overflows data directory\n"));
      return EFI_INVALID_PARAMETER;
    }

    //
    // Check image sections overflow
    //
    SectionHeaderOffset = DosHdr->e_lfanew + sizeof (UINT32)
        + sizeof (EFI_IMAGE_FILE_HEADER)
        + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader;

    if (PeHdr->Pe32.OptionalHeader.SizeOfImage < SectionHeaderOffset ||
      ((PeHdr->Pe32.OptionalHeader.SizeOfImage - SectionHeaderOffset)
      / EFI_IMAGE_SIZEOF_SECTION_HEADER) <= PeHdr->Pe32.FileHeader.NumberOfSections) {
      DEBUG ((DEBUG_WARN, "Image sections overflow image size\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (PeHdr->Pe32.OptionalHeader.SizeOfHeaders < SectionHeaderOffset
      || ((PeHdr->Pe32.OptionalHeader.SizeOfHeaders - SectionHeaderOffset)
      / EFI_IMAGE_SIZEOF_SECTION_HEADER)
      < (UINT32) PeHdr->Pe32.FileHeader.NumberOfSections) {
        DEBUG ((DEBUG_WARN, "Image sections overflow section headers\n"));
        return EFI_INVALID_PARAMETER;
    }
  } else if (PeHdrMagic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    //
    // Pe32+ part
    //

    //
    // Check image header size
    //
    if (EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES <
        PeHdr->Pe32Plus.OptionalHeader.NumberOfRvaAndSizes) {
      DEBUG ((DEBUG_WARN, "Image header too small\n"));
      return EFI_INVALID_PARAMETER;
    }

    //
    // Check image header aligment
    //
    HeaderWithoutDataDir = sizeof (EFI_IMAGE_OPTIONAL_HEADER64) -
                           sizeof (EFI_IMAGE_DATA_DIRECTORY) *
                           EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES;
    if (PeHdr->Pe32Plus.FileHeader.SizeOfOptionalHeader < HeaderWithoutDataDir
      || (PeHdr->Pe32Plus.FileHeader.SizeOfOptionalHeader - HeaderWithoutDataDir) !=
      PeHdr->Pe32Plus.OptionalHeader.NumberOfRvaAndSizes * sizeof (EFI_IMAGE_DATA_DIRECTORY)) {
      DEBUG ((DEBUG_WARN, "Image header overflows data directory\n"));
      return EFI_INVALID_PARAMETER;
    }

    //
    // Check image sections overflow
    //
    SectionHeaderOffset = DosHdr->e_lfanew
                          + sizeof (UINT32)
                          + sizeof (EFI_IMAGE_FILE_HEADER)
                          + PeHdr->Pe32Plus.FileHeader.SizeOfOptionalHeader;

    if (PeHdr->Pe32Plus.OptionalHeader.SizeOfImage < SectionHeaderOffset
      || ((PeHdr->Pe32Plus.OptionalHeader.SizeOfImage - SectionHeaderOffset)
      / EFI_IMAGE_SIZEOF_SECTION_HEADER) <= PeHdr->Pe32Plus.FileHeader.NumberOfSections) {
      DEBUG ((DEBUG_WARN, "Image sections overflow image size\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (PeHdr->Pe32Plus.OptionalHeader.SizeOfHeaders < SectionHeaderOffset
      || ((PeHdr->Pe32Plus.OptionalHeader.SizeOfHeaders - SectionHeaderOffset)
      / EFI_IMAGE_SIZEOF_SECTION_HEADER) < (UINT32) PeHdr->Pe32Plus.FileHeader.NumberOfSections) {
      DEBUG ((DEBUG_WARN, "Image sections overflow section headers\n"));
      return EFI_INVALID_PARAMETER;
    }
  } else {
    DEBUG ((DEBUG_WARN, "Unsupported PE header magic\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (PeHdr->Te.Signature != EFI_IMAGE_NT_SIGNATURE) {
    DEBUG ((DEBUG_WARN, "Unsupported image type\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (PeHdr->Pe32.FileHeader.Characteristics & EFI_IMAGE_FILE_RELOCS_STRIPPED) {
    DEBUG ((DEBUG_WARN, "Unsupported image - Relocations have been stripped\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (PeHdrMagic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Fill context for Pe32
    //
    Context->PeHdr = PeHdr;
    Context->ImageAddress = PeHdr->Pe32.OptionalHeader.ImageBase;
    Context->ImageSize = PeHdr->Pe32.OptionalHeader.SizeOfImage;
    Context->SizeOfOptionalHeader = PeHdr->Pe32.FileHeader.SizeOfOptionalHeader;
    Context->OptHdrChecksum= &Context->PeHdr->Pe32.OptionalHeader.CheckSum;
    Context->SizeOfHeaders = PeHdr->Pe32.OptionalHeader.SizeOfHeaders;
    Context->EntryPoint = PeHdr->Pe32.OptionalHeader.AddressOfEntryPoint;
    Context->RelocDir = &PeHdr->Pe32.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
    Context->NumberOfRvaAndSizes = PeHdr->Pe32.OptionalHeader.NumberOfRvaAndSizes;
    Context->NumberOfSections = PeHdr->Pe32.FileHeader.NumberOfSections;
    Context->FirstSection = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) PeHdr
                            + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader
                            + sizeof (UINT32)
                            + sizeof (EFI_IMAGE_FILE_HEADER));
    if (Context->NumberOfSections >= EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
      Context->SecDir = &PeHdr->Pe32.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY];
    }
  } else {
    //
    // Fill context for Pe32+
    //
    Context->PeHdr = PeHdr;
    Context->ImageAddress = PeHdr->Pe32Plus.OptionalHeader.ImageBase;
    Context->ImageSize = PeHdr->Pe32Plus.OptionalHeader.SizeOfImage;
    Context->SizeOfOptionalHeader = PeHdr->Pe32.FileHeader.SizeOfOptionalHeader;
    Context->OptHdrChecksum= &Context->PeHdr->Pe32Plus.OptionalHeader.CheckSum;
    Context->SizeOfHeaders = PeHdr->Pe32Plus.OptionalHeader.SizeOfHeaders;
    Context->EntryPoint = PeHdr->Pe32Plus.OptionalHeader.AddressOfEntryPoint;
    Context->RelocDir = &PeHdr->Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
    Context->NumberOfRvaAndSizes = PeHdr->Pe32Plus.OptionalHeader.NumberOfRvaAndSizes;
    Context->NumberOfSections = PeHdr->Pe32.FileHeader.NumberOfSections;
    Context->FirstSection = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) PeHdr
                            + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader
                            + sizeof (UINT32)
                            + sizeof (EFI_IMAGE_FILE_HEADER));
    if (Context->NumberOfSections >= EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
      Context->SecDir = &PeHdr->Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY];
    }
  }

  //
  // Fill sections info
  //
  Context->PeHdrMagic = PeHdrMagic;
  SectionCache = Context->FirstSection;

  for (Index = 0, Context->SumOfSectionBytes = 0;
       Index < Context->NumberOfSections; Index++, SectionCache++) {
    if (Context->SumOfSectionBytes + SectionCache->SizeOfRawData
      < Context->SumOfSectionBytes) {
      DEBUG ((DEBUG_WARN, "Malformed binary: %x %x\n", (UINT32) Context->SumOfSectionBytes, ImageSize));
      return EFI_INVALID_PARAMETER;
    }
    Context->SumOfSectionBytes += SectionCache->SizeOfRawData;
  }

  if (Context->SumOfSectionBytes >= ImageSize) {
    DEBUG ((DEBUG_WARN, "Malformed binary: %x %x\n", (UINT32) Context->SumOfSectionBytes, ImageSize));
    return EFI_INVALID_PARAMETER;
  }

  if (Context->ImageSize < Context->SizeOfHeaders) {
    DEBUG ((DEBUG_WARN, "Invalid image\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (Context->SecDir != NULL) {
    if ((UINT32) ((UINT8 *) Context->SecDir - (UINT8 *) Image) >
        (ImageSize - sizeof (EFI_IMAGE_DATA_DIRECTORY))) {
      DEBUG ((DEBUG_WARN, "Invalid image\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (Context->SecDir->VirtualAddress >= ImageSize) {
      DEBUG ((DEBUG_WARN, "Malformed security header\n"));
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}


EFI_STATUS
GetApplePeImageSignature (
  VOID                                *Image,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  UINT8                               *PkLe,
  UINT8                               *PkBe,
  UINT8                               *SigLe,
  UINT8                               *SigBe
  )
{
  EFI_STATUS                 Status                    = EFI_UNSUPPORTED;
  UINTN                      Index                     = 0;
  APPLE_SIGNATURE_DIRECTORY  *SignatureDirectory       = NULL;

  //
  // Check SecDir extistence
  //
  if (Context->SecDir != NULL) {

    //
    // Extract AppleSignatureDirectory
    //
    SignatureDirectory = (APPLE_SIGNATURE_DIRECTORY *)
                         ((UINT8 *) Image + Context->SecDir->VirtualAddress);

    //
    // Apple EFI_IMAGE_DIRECTORY_ENTRY_SECURITY is always 8 bytes.
    //
    if (Context->SecDir->Size != APPLE_SIGNATURE_SECENTRY_SIZE) {
      DEBUG ((DEBUG_WARN, "AppleSignature entry size mismatch\n"));
      return Status;
    }

    //
    // Verify Apple Signature GUID
    //
    
    if (!CompareGuid (&SignatureDirectory->AppleSignatureGuid, &gAppleEfiSignatureGuid)) {
      return Status;
    }

    //
    // Load PublicKey and Signature into memory
    //
    CopyMem (PkLe, SignatureDirectory->PublicKey, 256);
    CopyMem (SigLe, SignatureDirectory->Signature, 256);

    for (Index = 0; Index < 256; Index++) {
      PkBe[256 - 1 - Index] = PkLe[Index];
      SigBe[256 - 1 - Index] = SigLe[Index];
    }

    Status = EFI_SUCCESS;

  } else {
    DEBUG ((DEBUG_WARN, "Signature entry not exist\n"));
  }

  return Status;
}

VOID
SanitizeApplePeImage (
  VOID                                *Image,
  UINT32                              *RealImageSize,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context
  )
{
  UINT32                   ImageSize            = *RealImageSize;
  EFI_IMAGE_DOS_HEADER     *DosHdr              = Image;

  //
  // Check DOS header existence
  //  

  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // Drop DOS stub if it present
    //
    if ((((EFI_IMAGE_DOS_HEADER *) Image)->e_lfanew 
          - sizeof (EFI_IMAGE_DOS_HEADER)) != 0) {
      ZeroMem (
          (UINT8 *) Image + sizeof (EFI_IMAGE_DOS_HEADER),
          ((EFI_IMAGE_DOS_HEADER *) Image)->e_lfanew - sizeof (EFI_IMAGE_DOS_HEADER)
          );
    }
  }

  //
  // Calculate real image size
  //
  DEBUG ((DEBUG_WARN, "Real image size: %lu\n", *RealImageSize));

  *RealImageSize = Context->SecDir->VirtualAddress
                   + Context->SecDir->Size 
                   + sizeof (APPLE_SIGNATURE_DIRECTORY);  
  
  if (*RealImageSize < ImageSize) {
    DEBUG ((DEBUG_WARN, "Droping tail with size: %lu\n", ImageSize - *RealImageSize));
    //
    // Drop tail
    //
    ZeroMem (
      (UINT8 *) Image + *RealImageSize,
      ImageSize - *RealImageSize
      );
  }
}

EFI_STATUS
GetApplePeImageSha256 (
  VOID                                *Image,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  UINT8                               *CalcucatedHash
  )
{
  UINT64                   HashSize           = 0;
  UINT8                    *HashBase          = NULL;
  SHA256_CONTEXT           HashContext;
  
  //
  // Initialise a SHA hash context
  //
  Sha256Init (&HashContext);

  //
  // Hash DOS header and skip DOS stub
  //
  Sha256Update (&HashContext, Image, sizeof (EFI_IMAGE_DOS_HEADER));

  /**
    Measuring PE/COFF Image Header;
    But CheckSum field and SECURITY data directory (certificate) are excluded.
    Calculate the distance from the base of the image header to the image checksum address
    Hash the image header from its base to beginning of the image checksum
  **/

  HashBase = (UINT8 *) Image + ((EFI_IMAGE_DOS_HEADER *) Image)->e_lfanew;
  HashSize = (UINT8 *) Context->OptHdrChecksum - HashBase;
  Sha256Update (&HashContext, HashBase, HashSize);

  //
  // Hash everything from the end of the checksum to the start of the Cert Directory.
  //
  HashBase = (UINT8 *) Context->OptHdrChecksum + sizeof (UINT32);
  HashSize = (UINT8 *) Context->SecDir - HashBase;
  Sha256Update (&HashContext, HashBase, HashSize);

  //
  // Hash from the end of SecDirEntry till SecDir data
  //
  HashBase = (UINT8 *) Context->RelocDir;
  HashSize = (UINT8 *) Image + Context->SecDir->VirtualAddress - HashBase;
  Sha256Update (&HashContext, HashBase, HashSize);

  Sha256Final (&HashContext, CalcucatedHash);
  return EFI_SUCCESS;
}

EFI_STATUS
VerifyApplePeImageSignature (
  IN OUT VOID                                *PeImage,
  IN OUT UINT32                              *ImageSize,
  IN OUT APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context OPTIONAL
  )
{
  SHA256_CONTEXT                     HashContext;
  UINT8                              PkLe[256];
  UINT8                              PkBe[256];
  UINT8                              SigLe[256];
  UINT8                              SigBe[256];
  UINT8                              CalcucatedHash[32];
  UINT8                              PkHash[32];
  UINT32                             WorkBuf32[RSANUMWORDS*3];
  RSA_PUBLIC_KEY                     *Pk                      = NULL;
  
  //
  // Build context if not present
  //
  if (Context == NULL) {
    Context = AllocateZeroPool (sizeof (APPLE_PE_COFF_LOADER_IMAGE_CONTEXT));
    if (Context == NULL) {
      DEBUG ((DEBUG_WARN, "Pe context allocation failure\n"));
      return EFI_INVALID_PARAMETER;
    }
    //
    // Build PE context
    //
    if (EFI_ERROR (BuildPeContext (PeImage, *ImageSize, Context))) {
      DEBUG ((DEBUG_WARN, "Malformed ApplePeImage\n"));
      FreePool (Context);
      return EFI_INVALID_PARAMETER;
    }    
  }

  //
  // Sanitzie ApplePeImage
  //
  SanitizeApplePeImage (PeImage, ImageSize, Context);

  //
  // Extract AppleSignature from PEImage
  //
  if (EFI_ERROR (GetApplePeImageSignature (PeImage, Context, PkLe, PkBe, SigLe, SigBe))) {
    DEBUG ((DEBUG_WARN, "AppleSignature broken or not present!\n"));
    FreePool (Context);
    return EFI_UNSUPPORTED;
  }

  //
  // Calcucate PeImage hash via AppleAuthenticode algorithm
  //
  if (EFI_ERROR (GetApplePeImageSha256 (PeImage, Context, CalcucatedHash))) {
    DEBUG ((DEBUG_WARN, "Couldn't calcuate hash of PeImage\n"));
    FreePool (Context);
    return EFI_INVALID_PARAMETER;
  }

  FreePool (Context);

  //
  // Calculate Sha256 of extracted public key
  //
  Sha256Init (&HashContext);
  Sha256Update (&HashContext, PkLe, sizeof (PkLe));
  Sha256Final (&HashContext, PkHash);

  //
  // Verify existence in DataBase
  //
  for (int Index = 0; Index < NUM_OF_PK; Index++) {
    if (CompareMem (PkDataBase[Index].Hash, PkHash, sizeof (PkHash)) == 0) {
      //
      // PublicKey valid. Extract prepared publickey from database
      //
      Pk = (RSA_PUBLIC_KEY *) PkDataBase[Index].PublicKey;
    }
  }

  if (Pk == NULL) {
    DEBUG ((DEBUG_WARN, "Unknown publickey or malformed AppleSignature directory!\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // Verify signature
  //
  if (RsaVerify (Pk, SigBe, CalcucatedHash, WorkBuf32) == 1 ) {
    DEBUG ((DEBUG_WARN, "Signature verified!\n"));
    return EFI_SUCCESS;
  }

  return EFI_SECURITY_VIOLATION;
}
