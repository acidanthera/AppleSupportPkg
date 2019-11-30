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

#include <Base.h>
#include <Uefi.h>
#include <PiDxe.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcAppleImageVerificationLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/AppleLoadImage.h>
#include "AppleImageLoader.h"
#include "Core/CoreMain.h"
#include <AppleSupportPkgVersion.h>

STATIC EFI_IMAGE_LOAD  mOriginalLoadImage = NULL;

EFI_STATUS
ParseAppleEfiFatBinary (
  VOID   *SourceBuffer,
  UINTN  SourceSize,
  VOID   **ImageBuffer,
  UINTN  *ImageSize
  )
{
  APPLE_EFI_FAT_HEADER  *Hdr         = NULL;
  UINTN                 Index        = 0;
  UINT64                SizeOfBinary = 0;

  //
  // Cause when image loaded from memory
  //
  if (SourceSize < sizeof (APPLE_EFI_FAT_HEADER)) {
    DEBUG ((DEBUG_INFO, "AppleImageLoader: Malformed binary\n"));
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
    DEBUG ((DEBUG_INFO, "AppleImageLoader: Binary isn't AppleEfiFat\n"));
    return EFI_UNSUPPORTED;
  }
  DEBUG ((DEBUG_INFO, "AppleImageLoader: FatBinary matched\n"));
  SizeOfBinary = sizeof (APPLE_EFI_FAT_HEADER)
                  + sizeof (APPLE_EFI_FAT_ARCH_HEADER)
                    * Hdr->NumArchs;

  if (SizeOfBinary > SourceSize) {
    DEBUG ((DEBUG_INFO, "AppleImageLoader: Malformed AppleEfiFat header\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Loop over number of arch's
  //
  for (Index = 0; Index < Hdr->NumArchs; Index++) {
    //
    // Arch dependency parse
    //
#if defined(MDE_CPU_IA32)
    if (Hdr->Archs[Index].CpuType == CPUYPE_X86) {
#elif defined(MDE_CPU_X64)
    if (Hdr->Archs[Index].CpuType == CPUYPE_X86_64) {
#else
#error "Unsupported Platform"
#endif
      DEBUG ((
        DEBUG_INFO,
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
        DEBUG ((
          DEBUG_INFO,
          "AppleImageLoader: Wrong offset of Image or it's size\n"
          ));
        return EFI_INVALID_PARAMETER;
      }

      DEBUG ((
        DEBUG_INFO,
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

  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
LoadImageEx (
  IN   BOOLEAN                   BootPolicy,
  IN   EFI_HANDLE                ParentImageHandle,
  IN   EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN   VOID                      *SourceBuffer      OPTIONAL,
  IN   UINTN                     SourceSize,
  OUT  EFI_HANDLE                *ImageHandle)
{
  EFI_STATUS                 Status                = EFI_INVALID_PARAMETER;
  VOID                       *FileBuffer           = NULL;
  UINTN                      FileSize              = 0;
  VOID                       *ImageBuffer          = NULL;
  UINTN                      ImageSize             = 0;
  UINT32                     AuthenticationStatus  = 0;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage          = NULL;
  EFI_HANDLE                 DeviceHandle          = 0;
  EFI_DEVICE_PATH_PROTOCOL   *RemainingDevicePath  = FilePath;

  if (SourceBuffer == NULL && FilePath != NULL) {
    FileBuffer = GetFileBufferByFilePath (
      BootPolicy,
      FilePath,
      &FileSize,
      &AuthenticationStatus
      );
    SourceBuffer = FileBuffer;
    SourceSize = FileSize;
  }

  //
  // Try to parse as AppleEfiFatBinary
  //
  if (SourceBuffer != NULL && SourceSize > 0 && SourceSize <= (UINT32) 0xFFFFFFFFULL) {
    Status = ParseAppleEfiFatBinary (
      SourceBuffer,
      SourceSize,
      &ImageBuffer,
      &ImageSize
      );

    if (!EFI_ERROR (Status)) {
      SourceBuffer = ImageBuffer;
      SourceSize = ImageSize;

      if (SourceSize > 0 && SourceSize <= (UINT32) 0xFFFFFFFFULL) {
        //
        // Verify ApplePeImage signature
        //

        //
        //
        //
        Status = VerifyApplePeImageSignature (SourceBuffer, &SourceSize);
      } else {
        Status = EFI_INVALID_PARAMETER;
      }

      if (EFI_ERROR (Status)) {
        if (FileBuffer != NULL) {
          FreePool (FileBuffer);
        }
        return Status;
      }
    }

    Status = gBS->LocateDevicePath (
      &gEfiDevicePathProtocolGuid,
      &RemainingDevicePath,
      &DeviceHandle
      );

    if (EFI_ERROR (Status)) {
      if (FileBuffer != NULL) {
        FreePool (FileBuffer);
      }
      return Status;
    }

    Status = mOriginalLoadImage (
      BootPolicy,
      ParentImageHandle,
      FilePath,
      SourceBuffer,
      SourceSize,
      ImageHandle
      );

    //
    // dmazar: some boards do not put device handle to
    // EfiLoadedImageProtocol->DeviceHandle when image is loaded from SrcBuffer,
    // and then boot.efi fails. We'll fix EfiLoadedImageProtocol here.
    //
    if (!EFI_ERROR (Status) && DeviceHandle != 0) {
      Status = gBS->OpenProtocol (
        *ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID **)&LoadedImage,
        gImageHandle,
        NULL,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );

      if (!EFI_ERROR (Status) && LoadedImage->DeviceHandle != DeviceHandle) {
        LoadedImage->DeviceHandle = DeviceHandle;
        LoadedImage->FilePath     = DuplicateDevicePath (RemainingDevicePath);
      }
    }
  }

  if (FileBuffer != NULL) {
    FreePool (FileBuffer);
  }

  return Status;
}

EFI_STATUS
EFIAPI
AppleLoadImage (
  BOOLEAN                   BootPolicy,
  EFI_HANDLE                ParentImageHandle,
  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  VOID                      *SourceBuffer,
  UINTN                     SourceSize,
  EFI_HANDLE                *ImageHandle,
  UINT64                    Version
  )
{
  EFI_STATUS                 Status                = EFI_INVALID_PARAMETER;
  VOID                       *FileBuffer           = NULL;
  UINTN                      FileSize              = 0;
  VOID                       *ImageBuffer          = NULL;
  UINTN                      ImageSize             = 0;
  UINT32                     AuthenticationStatus  = 0;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage          = NULL;
  EFI_HANDLE                 DeviceHandle          = 0;
  EFI_DEVICE_PATH_PROTOCOL   *RemainingDevicePath  = FilePath;

  if (SourceBuffer == NULL && FilePath != NULL) {
    FileBuffer = GetFileBufferByFilePath (
      BootPolicy,
      FilePath,
      &FileSize,
      &AuthenticationStatus
      );
    SourceBuffer = FileBuffer;
    SourceSize = FileSize;
  }

  // Verify ApplePeImage signature
  if (SourceBuffer != NULL && SourceSize != 0) {
    //
    // Parse fat structure
    //
    Status = ParseAppleEfiFatBinary (
      SourceBuffer,
      SourceSize,
      &ImageBuffer,
      &ImageSize
      );

    if (!EFI_ERROR (Status)) {
      SourceBuffer = ImageBuffer;
      SourceSize = ImageSize;

      if (SourceSize > 0 && SourceSize <= (UINT32) 0xFFFFFFFFULL) {
        Status = VerifyApplePeImageSignature (SourceBuffer, &SourceSize);
      } else {
        Status = EFI_INVALID_PARAMETER;
      }

      if (EFI_ERROR (Status)) {
        if (FileBuffer != NULL) {
          FreePool (FileBuffer);
        }
        return Status;
      }
    } else {
      if (SourceSize > 0 && SourceSize <= (UINT32) 0xFFFFFFFFULL) {
        Status = VerifyApplePeImageSignature (SourceBuffer, &SourceSize);
      } else {
        Status = EFI_INVALID_PARAMETER;
      }

      if (EFI_ERROR (Status)) {
        if (FileBuffer != NULL) {
          FreePool (FileBuffer);
        }
        return Status;
      }
    }

    Status = gBS->LocateDevicePath (
      &gEfiDevicePathProtocolGuid,
      &RemainingDevicePath,
      &DeviceHandle
      );

    if (EFI_ERROR (Status)) {
      if (FileBuffer != NULL) {
        FreePool (FileBuffer);
      }
      return Status;
    }

    Status = mOriginalLoadImage (
      BootPolicy,
      ParentImageHandle,
      FilePath,
      SourceBuffer,
      SourceSize,
      ImageHandle
      );

    //
    // dmazar: some boards do not put device handle to
    // EfiLoadedImageProtocol->DeviceHandle when image is loaded from SrcBuffer,
    // and then boot.efi fails. We'll fix EfiLoadedImageProtocol here.
    //
    if (!EFI_ERROR (Status) && DeviceHandle != 0) {
      Status = gBS->OpenProtocol (
        *ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID **)&LoadedImage,
        gImageHandle,
        NULL,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );

      if (!EFI_ERROR (Status) && LoadedImage->DeviceHandle != DeviceHandle) {
        LoadedImage->DeviceHandle = DeviceHandle;
        LoadedImage->FilePath = DuplicateDevicePath (RemainingDevicePath);
      }
    }
  }

  if (FileBuffer != NULL) {
    FreePool (FileBuffer);
  }

  return Status;
}

/*STATIC APPLE_LOAD_IMAGE_PROTOCOL mAppleLoadImageProtocol = {
  AppleLoadImage
};*/

EFI_STATUS
EFIAPI
AppleImageLoaderEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  //EFI_STATUS                    Status;
  //EFI_HANDLE                    NewHandle                = NULL;
  //APPLE_LOAD_IMAGE_PROTOCOL     *AppleLoadImageInterface = NULL;

  DEBUG ((
    DEBUG_INFO,
    "Starting AppleImageLoader ver. %s\n",
    APPLE_SUPPORT_VERSION
    ));

  /*Status = gBS->LocateProtocol (
    &gAppleLoadImageProtocolGuid,
    NULL,
    (VOID **)&AppleLoadImageInterface
    );

  if (EFI_ERROR (Status)){
    //
    // Install AppleLoadImage protocol
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
      &NewHandle,
      &gAppleLoadImageProtocolGuid,
      &mAppleLoadImageProtocol,
      NULL
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "AppleLoadImage install failed with Status: %r\n",
        Status
        ));
    }
  } else {
    DEBUG ((DEBUG_INFO, "AppleLoadImage already present\n"));
  }
  */

  //
  // Override Edk2LoadImage protocol for AppleFatBinary support
  //
  /*mOriginalLoadImage = gBS->LoadImage;
  gBS->LoadImage = CoreLoadImage;
  gBS->StartImage = CoreStartImage;
  gBS->UnloadImage = CoreUnloadImage;
  gBS->Exit = CoreExit;
  gBS->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 (gBS, sizeof (EFI_BOOT_SERVICES), &gBS->Hdr.CRC32);*/

  return EFI_SUCCESS;
}
