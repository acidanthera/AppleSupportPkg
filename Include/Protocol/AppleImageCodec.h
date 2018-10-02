
//********************************************************************
//  created:  28:8:2012   20:54
//  filename:   AppleImageCodec.h
//  author:    tiamo
//  purpose:  image code
//********************************************************************
// dmazar: changed ImageWidth and ImageHeight in GET_IMAGE_DIMS
//         to UINT32 from UINTN to get it working in 64 bit
//********************************************************************
// savvas: fixed RecognizeImageData structure and its function.
//         code refactor and cleanup. use lodepng instead of picopng
//********************************************************************

#ifndef APPLE_IMAGE_CODEC_H_
#define APPLE_IMAGE_CODEC_H_

#define APPLE_IMAGE_CODEC_PROTOCOL_GUID               \
  { 0x0DFCE9F6, 0xC4E3, 0x45EE,                       \
    {0xA0, 0x6A, 0xA8, 0x61, 0x3B, 0x98, 0xA5, 0x07 } }

typedef
EFI_STATUS
(EFIAPI* RECOGNIZE_IMAGE_DATA) (
  VOID   *ImageBuffer,
  UINTN  ImageSize
  );

typedef
EFI_STATUS 
(EFIAPI* GET_IMAGE_DIMS) (
  VOID    *ImageBuffer,
  UINTN   ImageSize,
  UINT32  *ImageWidth,
  UINT32  *ImageHeight
  );

typedef
EFI_STATUS
(EFIAPI* DECODE_IMAGE_DATA) (
  VOID           *ImageBuffer,
  UINTN          ImageSize,
  EFI_UGA_PIXEL  **RawImageData,
  UINTN          *RawImageDataSize
  );

typedef 
EFI_STATUS 
(EFIAPI* GET_IMAGE_DIMS_VER) (
  VOID    *ImageBuffer,
  UINTN   ImageSize,
  UINTN   Version,
  UINT32  *ImageWidth,
  UINT32  *ImageHeight
  );

typedef
EFI_STATUS
(EFIAPI* DECODE_IMAGE_DATA_VER) (
  VOID           *ImageBuffer,
  UINTN          ImageSize,
  UINTN          Version,
  EFI_UGA_PIXEL  **RawImageData,
  UINTN          *RawImageDataSize
  );

typedef struct APPLE_IMAGE_CODEC_PROTOCOL_
{
  UINT64                 Version;
  UINTN                  FileExt;
  RECOGNIZE_IMAGE_DATA   RecognizeImageData;
  GET_IMAGE_DIMS         GetImageDims;
  DECODE_IMAGE_DATA      DecodeImageData;
  GET_IMAGE_DIMS_VER     GetImageDimsVer;
  DECODE_IMAGE_DATA_VER  DecodeImageDataVer;
} APPLE_IMAGE_CODEC_PROTOCOL;

extern EFI_GUID gAppleImageCodecProtocolGuid;

#endif //APPLE_IMAGE_CODEC_H_
