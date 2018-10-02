/** @file

AppleImageCodec protocol

Copyright (C) 2018 savvas. All rights reserved.<BR>
Portions copyright (C) 2016 slice. All rights reserved.<BR>
Portions copyright (C) 2018 vit9696. All rights reserved.<BR>


All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/UgaDraw.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcPngLib.h>
#include <Protocol/AppleImageCodecProtocol.h>
#include "AppleImageCodec.h"

STATIC
EG_IMAGE *
CreateEfiGraphicsImage (
  INTN     Width,
  INTN     Height,
  BOOLEAN  HasAlpha
  )
{
  EG_IMAGE  *NewImage = NULL;

  NewImage = (EG_IMAGE *) AllocateZeroPool (sizeof (EG_IMAGE));
  if (NewImage == NULL) {
    return NULL;
  }

  NewImage->PixelData = (EFI_UGA_PIXEL *) AllocateZeroPool (
    (UINTN) (Width * Height * sizeof (EFI_UGA_PIXEL))
    );
  if (NewImage->PixelData == NULL) {
      FreePool (NewImage);
      return NULL;
  }

  NewImage->Width = Width;
  NewImage->Height = Height;
  NewImage->HasAlpha = HasAlpha;
  return NewImage;
}

STATIC
VOID
FreeEfiGraphicsImage (
  EG_IMAGE  *Image
  )
{
  if (Image != NULL) {
    if (Image->PixelData != NULL) {
      FreePool (Image->PixelData);
      Image->PixelData = NULL;
    }
    FreePool (Image);
  }
}

STATIC
EG_IMAGE *
DecodePngImage (
  UINT8  *FileData,
  UINTN  FileDataLength
  )
{
  EFI_STATUS        Status;
  EG_IMAGE          *NewImage    = NULL;
  EFI_UGA_PIXEL     *Pixel       = NULL;
  UINT8             *Data        = NULL;
  UINT8             *DataPtr     = NULL;
  INTN              X            = 0;
  INTN              Y            = 0;
  UINT32            Width        = 0;
  UINT32            Height       = 0;
  UINT32            HasAlphaType = 0;

  //
  // Decode PNG image
  //
  Status = DecodePng (
            FileData,
            FileDataLength,
            &Data,
            &Width,
            &Height,
            &HasAlphaType
            );

  if (EFI_ERROR (Status)) {
    return NULL;
  }
  //
  // Create efi graphics image
  //
  NewImage = CreateEfiGraphicsImage (
    Width,
    Height,
    (BOOLEAN) HasAlphaType
    );

  if (NewImage == NULL) {
    FreePng (Data);
    return NULL;
  }

  DataPtr = Data;
  Pixel = (EFI_UGA_PIXEL*) NewImage->PixelData;
  for (Y = 0; Y < NewImage->Height; Y++) {
    for (X = 0; X < NewImage->Width; X++) {
      Pixel->Red = *DataPtr++;
      Pixel->Green = *DataPtr++;
      Pixel->Blue = *DataPtr++;
      Pixel->Reserved = 0xFF - *DataPtr++;
      Pixel++;
    }
  }

  FreePng (Data);
  return NewImage;
}

STATIC
EFI_STATUS
EFIAPI
RecognizeImageData (
  IN  VOID   *ImageBuffer,
  IN  UINTN  ImageSize
  )
{
  EFI_STATUS  Status          = EFI_INVALID_PARAMETER;
  BOOLEAN     IsValidPngImage = FALSE;
  UINT8       PngHeader[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

  if (ImageBuffer != NULL && ImageSize >= 8) {
    IsValidPngImage = CompareMem (ImageBuffer, PngHeader, 8) == 0;
    if (IsValidPngImage) {
      Status = EFI_SUCCESS;
    }
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
GetImageDims (
  IN  VOID    *ImageBuffer,
  IN  UINTN   ImageSize,
  OUT UINT32  *ImageWidth,
  OUT UINT32  *ImageHeight
  )
{
  EG_IMAGE  *Image = NULL;

  Image = DecodePngImage ((UINT8*) ImageBuffer, ImageSize);
  if (Image == NULL) {
    return EFI_UNSUPPORTED;
  }

  *ImageWidth = (UINT32) Image->Width;
  *ImageHeight = (UINT32) Image->Height;

  FreeEfiGraphicsImage (Image);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
DecodeImageData (
  IN  VOID           *ImageBuffer,
  IN  UINTN          ImageSize,
  OUT EFI_UGA_PIXEL  **RawImageData,
  OUT UINTN          *RawImageDataSize
  )
{
  EFI_STATUS  Status  = 0;
  EG_IMAGE    *Image  = NULL;

  if (!RawImageData || !RawImageDataSize) {
    return EFI_INVALID_PARAMETER;
  }

  Image = DecodePngImage ((UINT8*) ImageBuffer, ImageSize);
  if (Image == NULL) {
    return EFI_UNSUPPORTED;
  }

  *RawImageDataSize = (UINT32) (
    Image->Width
    * Image->Height
    * sizeof(EFI_UGA_PIXEL)
    );

  Status = gBS->AllocatePool (
    EfiBootServicesData,
    *RawImageDataSize,
    (VOID **)RawImageData
    );
  if (!EFI_ERROR (Status)) {
    gBS->CopyMem (*RawImageData, (VOID*)Image->PixelData, *RawImageDataSize);
  }

  FreeEfiGraphicsImage (Image);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GetImageDimsVersion (
  VOID              *Buffer,
  UINTN             BufferSize,
  UINTN             Version,
  UINT32            *Width,
  UINT32            *Height
  )
{
  EFI_STATUS Status = EFI_INVALID_PARAMETER;
  if (Buffer && BufferSize && Version && Height && Width) {
    Status = EFI_UNSUPPORTED;
    if (Version <= 1) {
      Status = GetImageDims (Buffer, BufferSize, Width, Height);
    }
  }
  return Status;
}

EFI_STATUS
EFIAPI
DecodeImageDataVersion (
  VOID               *Buffer,
  UINTN              BufferSize,
  UINTN              Version,
  EFI_UGA_PIXEL      **RawImageData,
  UINTN              *RawImageDataSize
  )
{
  EFI_STATUS Status = EFI_INVALID_PARAMETER;
  if (Buffer && BufferSize && Version && RawImageData && RawImageDataSize) {
    Status = EFI_UNSUPPORTED;
    if (Version <= 1) {
      Status = DecodeImageData (Buffer, BufferSize, RawImageData, RawImageDataSize);
    }
  }
  return Status;
}

//
// Image codec protocol instance.
//
STATIC APPLE_IMAGE_CODEC_PROTOCOL gAppleImageCodec = {
  // Version
  0x20000,
  // FileExt
  0,
  RecognizeImageData,
  GetImageDims,
  DecodeImageData,
  GetImageDimsVersion,
  DecodeImageDataVersion
};

/**
  InitializeAppleImageCodec

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS          The entry point is executed successfully.
  @retval EFI_ALREADY_STARTED  The protocol has already been installed.
**/
EFI_STATUS
EFIAPI
InitializeAppleImageCodec (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
)
{
  EFI_STATUS                  Status;
  EFI_HANDLE                  NewHandle                 = NULL;
  APPLE_IMAGE_CODEC_PROTOCOL  *AppleImageCodecInterface = NULL;

  Status = gBS->LocateProtocol (
    &gAppleImageCodecProtocolGuid,
    NULL,
    (VOID **)&AppleImageCodecInterface
    );

  if (EFI_ERROR (Status)) {
    //
    // Install instance of Apple image codec protocol for
    // PNG files
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
      &NewHandle,
      &gAppleImageCodecProtocolGuid,
      &gAppleImageCodec,
      NULL
      );
  } else {
    Status = EFI_ALREADY_STARTED;
  }

  return Status;
}
