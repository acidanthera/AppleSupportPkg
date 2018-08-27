/** @file

AppleEvent

Copyright (c) 2018, savvas.<BR>
Portions copyright (c) 2018, CupertinoNet.<BR>

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <IndustryStandard/AppleHid.h>
#include <Protocol/AppleEvent.h>
#include <Protocol/LoadedImage.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC APPLE_EVENT_PROTOCOL mAppleEventProtocol = {
  1,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

/**
  InitializeAppleEvent

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS          The entry point is executed successfully.
  @retval EFI_ALREADY_STARTED  The protocol has already been installed.
**/
EFI_STATUS
EFIAPI
InitializeAppleEvent (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_HANDLE            NewHandle            = NULL;
  APPLE_EVENT_PROTOCOL  *AppleEventInterface = NULL;

  //
  // Check existence of AppleEventProtocol
  //
  Status = gBS->LocateProtocol (
    &gAppleEventProtocolGuid,
    NULL,
    (VOID **)&AppleEventInterface
    );

  if (EFI_ERROR (Status)) {
    Status = gBS->InstallMultipleProtocolInterfaces (
      &NewHandle,
      &gAppleEventProtocolGuid,
      &mAppleEventProtocol,
      NULL
      );
  } else {
    Status = EFI_ALREADY_STARTED;
  }

  return Status;
}
