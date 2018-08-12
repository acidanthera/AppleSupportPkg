#ifndef APPLE_DXE_IMAGE_VERIFICATION_H
#define APPLE_DXE_IMAGE_VERIFICATION_H

//
// Function prototypes
//
UINT16
GetPeHeaderMagicValue (
  EFI_IMAGE_OPTIONAL_HEADER_UNION *Hdr
  );

EFI_STATUS
GetPeHeader (
  VOID                                *Image,
  UINT32                              ImageSize,
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
  UINT32                              ImageSize,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  UINT8                               *CalcucatedHash
  );

EFI_STATUS
VerifyApplePeImageSignature (
  VOID     *PeImage,
  UINT32   ImageSize
  );

#endif //APPLE_DXE_IMAGE_VERIFICATION_H
