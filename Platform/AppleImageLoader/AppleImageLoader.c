/** @file

AppleImageLoader

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "AppleImageLoader.h"

STATIC EFI_HANDLE       Handle            = NULL;
STATIC EFI_IMAGE_LOAD   OriginalLoadImage = NULL;

EFI_STATUS
ParseAppleEfiFatBinary (
  VOID  *SourceBuffer,
  UINTN SourceSize,
  VOID  **ImageBuffer,
  UINTN *ImageSize
  )
{
  APPLE_EFI_FAT_HEADER  *Hdr         = NULL;
  UINTN                 Index        = 0;
  UINT64                SizeOfBinary = 0;

  if (SourceBuffer != NULL && SourceSize != 0) {
    //
    // Cause when image loaded from memory
    //
    if (SourceSize < sizeof (APPLE_EFI_FAT_HEADER)) {
      DEBUG ((DEBUG_VERBOSE, "AppleImageLoader: Malformed binary\n"));
      return EFI_INVALID_PARAMETER;
    }

    //
    // Get AppleEfiFatHeader
    //     
    Hdr = (APPLE_EFI_FAT_HEADER *) SourceBuffer;

    //
    // Verify magic number
    //
    if (Hdr->Magic != APPLE_EFI_FAT_MAGIC) {
      DEBUG ((DEBUG_VERBOSE, "AppleImageLoader: Binary isn't AplpeEfiFat\n"));
      return EFI_UNSUPPORTED;
    }
    DEBUG ((DEBUG_VERBOSE, "AppleImageLoader: FatBinary matched\n"));    
    SizeOfBinary = sizeof (APPLE_EFI_FAT_HEADER) 
                    + sizeof (APPLE_EFI_FAT_ARCH_HEADER) 
                      * Hdr->NumArchs;
    
    if (SizeOfBinary > SourceSize) {
      DEBUG ((DEBUG_VERBOSE, "AppleImageLoader: Malformed AppleEfiFat header\n"));
      return EFI_INVALID_PARAMETER;
    }    

    //
    // Loop over number of arch's
    //
    for (Index = 0; Index < Hdr->NumArchs; Index++) {
      //
      // Arch dependency parse
      //
#if defined(EFI32) || defined(MDE_CPU_IA32)
      if (Hdr->Archs[Index].CpuType == CPUYPE_X86) { 
#elif defined(EFIX64) || defined(MDE_CPU_X64)
      if (Hdr->Archs[Index].CpuType == CPUYPE_X86_64) {
#else
#error "Undefined Platform"
#endif
        DEBUG ((
          DEBUG_VERBOSE, 
          "AppleImageLoader: ApplePeImage at offset %u\n", 
          Hdr->Archs[Index].Offset
          ));

        //
        // Check offset boundary and its size
        // 
        if (Hdr->Archs[Index].Offset < SizeOfBinary 
          || Hdr->Archs[Index].Offset >= SourceSize
          || SourceSize < ((UINT64) Hdr->Archs[Index].Offset 
                          + Hdr->Archs[Index].Size)) {
          DEBUG ((DEBUG_VERBOSE, "AppleImageLoader: Wrong offset of Image or it's size\n"));
          return EFI_INVALID_PARAMETER;
        }

        DEBUG ((
          DEBUG_VERBOSE, 
          "AppleImageLoader: ApplePeImage size %u\n", 
          Hdr->Archs[Index].Size
          ));

        //
        // Extract ApplePeImage and return EFI_SUCCESS
        //
        *ImageSize   = Hdr->Archs[Index].Size;
        *ImageBuffer = (UINT8 *) SourceBuffer + Hdr->Archs[Index].Offset;

        return EFI_SUCCESS;
      }
      SizeOfBinary = (UINT64) Hdr->Archs[Index].Offset + Hdr->Archs[Index].Size;
    }    
  }  
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
LoadImageEx (
  IN   BOOLEAN                  BootPolicy, 
  IN   EFI_HANDLE               ParentImageHandle,
  IN   EFI_DEVICE_PATH_PROTOCOL *FilePath,
  IN   VOID                     *SourceBuffer      OPTIONAL,
  IN   UINTN                    SourceSize, 
  OUT  EFI_HANDLE               *ImageHandle)
{
  EFI_STATUS        Status;
  VOID              *SrcBuf               = NULL;
  UINTN             SrcSize               = 0;
  VOID              *ImageBuffer          = NULL;
  UINTN             ImageSize             = 0;
  UINT32            AuthenticationStatus  = 0;
  
  Status = EFI_INVALID_PARAMETER;
  SrcBuf = SourceBuffer;
  SrcSize = SourceSize;
  

  if (SrcBuf == NULL && FilePath != NULL) {
    SrcBuf = GetFileBufferByFilePath (
      BootPolicy,
      FilePath,
      &SrcSize,
      &AuthenticationStatus
      );
  }

  //
  // Try to parse as AppleEfiFatBinary
  // 
  if (SrcBuf != NULL && SrcSize != 0) {
    Status = ParseAppleEfiFatBinary (
      SrcBuf, 
      SrcSize, 
      &ImageBuffer, 
      &ImageSize
      );

    if (!EFI_ERROR (Status)) {
      SrcBuf = ImageBuffer;
      SrcSize = ImageSize;
      //
      // Verify ApplePeImage signature
      //
      Status = VerifyApplePeImageSignature (SrcBuf, SrcSize);
      
      if (EFI_ERROR (Status)) {
        return Status;
      }      
    }

    //
    // Load image with original function
    //
    Status =  OriginalLoadImage (
      BootPolicy,
      ParentImageHandle,
      FilePath,
      SrcBuf,
      SrcSize,
      ImageHandle
      );     
  }

  return Status;
}


EFI_STATUS
EFIAPI
AppleLoadImage (
  BOOLEAN                  BootPolicy,
  EFI_HANDLE               ParentImageHandle,
  EFI_DEVICE_PATH_PROTOCOL *FilePath,
  VOID                     *SourceBuffer,
  UINTN                    SourceSize,
  EFI_HANDLE               *ImageHandle,
  UINT64                   Version
  )
{
  EFI_STATUS  Status;
  VOID        *SrcBuf               = NULL;
  UINTN       SrcSize               = 0;  
  VOID        *ImageBuffer          = NULL;
  UINTN       ImageSize             = 0;  
  UINT32      AuthenticationStatus  = 0;

  Status = EFI_INVALID_PARAMETER;
  SrcBuf = SourceBuffer;
  SrcSize = SourceSize;

  if (SrcBuf == NULL && FilePath != NULL) {
    SrcBuf = GetFileBufferByFilePath (
      BootPolicy,
      FilePath,
      &SrcSize,
      &AuthenticationStatus
      );
  }

  // Verify ApplePeImage signature  
  if (SrcBuf != NULL && SrcSize != 0) {
    //
    // Parse fat structure
    //
    Status = ParseAppleEfiFatBinary (
      SrcBuf, 
      SrcSize, 
      &ImageBuffer, 
      &ImageSize
      );

    if (!EFI_ERROR (Status)) {
      SrcBuf = ImageBuffer;
      SrcSize = ImageSize;

      Status = VerifyApplePeImageSignature (SrcBuf, SrcSize);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    } else {
      Status = VerifyApplePeImageSignature (SrcBuf, SrcSize);
      if (EFI_ERROR (Status)) {
        return Status;
      }    
    }

    Status = OriginalLoadImage (
      BootPolicy,
      ParentImageHandle,
      FilePath,
      SrcBuf, 
      SrcSize,
      ImageHandle
      );     
  }


  return Status;
}

STATIC APPLE_LOAD_IMAGE_PROTOCOL mAppleLoadImageProtocol = {
  AppleLoadImage
};

EFI_STATUS
EFIAPI
AppleImageLoaderEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_SERVICES             *gBS;
   
  gBS = SystemTable->BootServices;
  
  //
  // Install AppleLoadImage protocol
  // 
  Status = gBS->InstallProtocolInterface (
    &Handle, 
    &gAppleLoadImageProtocolGuid, 
    0, 
    &mAppleLoadImageProtocol
    );

  //
  // Override Edk2LoadImage protocol for AppleFatBinary support
  //
  OriginalLoadImage = gBS->LoadImage;
  gBS->LoadImage = LoadImageEx;
  gBS->Hdr.CRC32 = 0;
  gBS->CalculateCrc32(gBS, sizeof (EFI_BOOT_SERVICES), &gBS->Hdr.CRC32);

  return EFI_SUCCESS;
}
