/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "FwRuntimeServicesPrivate.h"

#include <Library/DebugLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/OcFirmwareRuntime.h>

STATIC EFI_IMAGE_START  mStoredStartImage;

STATIC UINTN            mNestedCount;

EFI_STATUS
EFIAPI
WrapStartImage (
  IN  EFI_HANDLE  ImageHandle,
  OUT UINTN       *ExitDataSize,
  OUT CHAR16      **ExitData  OPTIONAL
  )
{
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *AppleLoadedImage;

  AppleLoadedImage = OcGetAppleBootLoadedImage (ImageHandle);

  //
  // Nest count checking is used to protect from situations like
  // Start BootLoader → { Start App, App Finished, Start Kernel }
  //
  ++mNestedCount;

  if (mNestedCount == 1) {
    //
    // Request boot variable redirection if enabled.
    //
    SetBootVariableRedirect (TRUE);

    if (AppleLoadedImage) {
      //
      // Latest Windows brings Virtualization-based security and monitors
      // CR0 by launching itself under a hypevisor. Since we need WP disable
      // on macOS to let NVRAM work, and for the time being no other OS
      // requires it, here we decide to use it for macOS exclusively.
      //
      SetWriteUnprotectorMode (TRUE);
    }

    Status = mStoredStartImage (ImageHandle, ExitDataSize, ExitData);

    if (AppleLoadedImage) {
      //
      // We failed but other operating systems should be loadable.
      //
      SetWriteUnprotectorMode (FALSE);
    }

    //
    // Disable redirect on failure, this is cleaner design-wise.
    //
    SetBootVariableRedirect (FALSE);
  } else {
    Status = mStoredStartImage (ImageHandle, ExitDataSize, ExitData);
  }

  --mNestedCount;

  return Status;
}

STATIC
OC_FIRMWARE_RUNTIME_PROTOCOL
mOcFirmwareRuntimeProtocol = {
  OC_FIRMWARE_RUNTIME_REVISION,
  SetBootVariableRedirect,
  SetCustomGetVariableHandler
};

EFI_STATUS
EFIAPI
UefiEntrypoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS   Status;
  VOID         *Interface;
  EFI_HANDLE   Handle;

  Status = gBS->LocateProtocol (
    &gOcFirmwareRuntimeProtocolGuid,
    NULL,
    &Interface
    );

  if (!EFI_ERROR (Status)) {
    //
    // In case for whatever reason one tried to reload the driver.
    //
    return EFI_ALREADY_STARTED;
  }

  mStoredStartImage       = gBS->StartImage;
  gBS->StartImage         = WrapStartImage;

  gBS->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 (gBS, gBS->Hdr.HeaderSize, &gBS->Hdr.CRC32);

  RedirectRuntimeServices ();

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &Handle,
    &gOcFirmwareRuntimeProtocolGuid,
    &mOcFirmwareRuntimeProtocol,
    NULL
    );

  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
