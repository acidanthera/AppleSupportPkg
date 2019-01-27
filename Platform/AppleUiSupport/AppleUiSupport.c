/** @file

AppleUiSupport

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include "AppleUiSupport.h"
#include <AppleSupportPkgVersion.h>

//
// Driver's entry point
//
EFI_STATUS
EFIAPI
AppleUiSupportEntrypoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
)
{
  EFI_STATUS            Status;

  DEBUG ((
    DEBUG_INFO,
    "Starting AppleUiSupport ver. %s\n",
    APPLE_SUPPORT_VERSION
    ));

  Status = InitializeAppleImageConversion (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "AppleUiSupport: AppleImageConversion install failure, Status = %r\n", Status));
  }

  Status = InitializeUserInterfaceTheme (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "AppleUiSupport: AppleUserInterfaceTheme install failure - %r\n", Status));
  }

  Status = InitializeUnicodeCollationEng (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "AppleUiSupport: UnicodeCollation install failure - %r\n", Status));
  }

  Status = InitializeHashServices (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "AppleUiSupport: HashServices install failure - %r\n", Status));
  }

  Status = InitializeAppleKeyMapAggregator (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "AppleUiSupport: AppleKeyMapAggregator install failure - %r\n", Status));
  }

  Status = InitializeAppleEvent (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "AppleUiSupport: AppleEvent install failure - %r\n", Status));
  }

  Status = InitializeFirmwareVolumeInject (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "AppleUiSupport: AppleFirmwareVolume install failure - %r\n", Status));
  }

  return Status;
}
