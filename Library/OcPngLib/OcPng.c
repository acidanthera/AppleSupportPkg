/** @file

OcPngLib

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
#include <Library/OcCryptoLib.h>
#include <Library/OcAppleImageVerificationLib.h>
#include <Protocol/DebugSupport.h>
#include <IndustryStandard/PeImage.h>
#include <Guid/AppleEfiCertificate.h>
#include "ApplePublicKeyDb.h"

//
// Returns RAW pixel buffer and its dimensions and state
//
VOID
DecodePng (
  IN   UINT8   *FileData,
  IN   UINTN   FileDataLength,
  OUT  UINT8   *Data,
  OUT  UINT32  Width, 
  OUT  UINT32  Height
  )
{
  LodePNGState      State;
  LodePNGColorMode  *Color       = NULL;
  UINT32          Error        = 0;

  //
  // Init lodepng state
  //
  lodepng_state_init (&State);

  //
  // It should return 0 on success
  //
  Error = lodepng_decode (
    &Data,
    &Width,
    &Height,
    &State,
    FileData,
    FileDataLength
    );

  if (Error) {
    lodepng_state_cleanup (&State);
    return NULL;
  }

}



