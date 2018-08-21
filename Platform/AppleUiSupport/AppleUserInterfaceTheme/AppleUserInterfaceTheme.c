/** @file

AppleUserInterfaceTheme

Copyright (c) 2016-2018, vit9696.<BR>
Portions copyright (c) 2018, savvas.<BR>


All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Protocol/UserInterfaceTheme.h>

STATIC UINT32           mCurrentColor = 0;

STATIC
EFI_STATUS
EFIAPI
UserInterfaceThemeGetColor (
  UINT32    *Color
  )
{
  if (Color == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Color = mCurrentColor;
  return EFI_SUCCESS;
}

STATIC EFI_USER_INTERFACE_THEME_PROTOCOL mAppleUserInterfaceThemeProtocol = {
  USER_THEME_INTERFACE_PROTOCOL_REVISION,
  UserInterfaceThemeGetColor
};

/**
  InitializeUserInterfaceTheme

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS          The entry point is executed successfully.
  @retval EFI_ALREADY_STARTED  The protocol has already been installed.
**/
EFI_STATUS
EFIAPI
InitializeUserInterfaceTheme (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS             Status;
  UINTN                  DataSize                   = 0;
  UINT32                 Color                      = 0;
  EFI_BOOT_SERVICES*     gBS                        = NULL;
  EFI_RUNTIME_SERVICES*  gRT                        = NULL;
  EFI_HANDLE             NewHandle                  = NULL;
  EFI_USER_INTERFACE_THEME_PROTOCOL *EfiUiInterface = NULL;

  gRT = SystemTable->RuntimeServices;      
  gBS = SystemTable->BootServices;
  
  //
  // Default color is black
  //
  mCurrentColor = 0x000000;

  Status = gBS->LocateProtocol (
    &gEfiUserInterfaceThemeProtocolGuid,
    NULL,
    (VOID **)&EfiUiInterface
    );

  if (EFI_ERROR (Status)) {
    Status = gRT->GetVariable (
      L"DefaultBackgroundColor", 
      &gAppleVendorVariableGuid, 
      0, 
      &DataSize, 
      &Color
      );
    if (!EFI_ERROR (Status)) {
      mCurrentColor = Color;
    }

    Status = gBS->InstallMultipleProtocolInterfaces (
      &NewHandle, 
      &gEfiUserInterfaceThemeProtocolGuid, 
      &mAppleUserInterfaceThemeProtocol,
      NULL
      );
  } else {
    Status = EFI_ALREADY_STARTED;
  }

  return Status;
}
