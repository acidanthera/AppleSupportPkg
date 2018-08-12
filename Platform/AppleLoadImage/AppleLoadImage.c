/** @file

Apple LoadImage protocol. 

Copyright (c) 2018, savvas

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
#include <Library/UefiBootServicesTableLib.h>
#include <Library/AppleDxeImageVerificationLib.h>
#include <Protocol/AppleLoadImage.h>


STATIC EFI_HANDLE       Handle = NULL;

EFI_STATUS
EFIAPI
AppleLoadImage (
  BOOLEAN                  BootPolicy,
  EFI_HANDLE               ParentImageHandle,
  EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  VOID                     *SourceBuffer,
  UINTN                    SourceSize,
  EFI_HANDLE               *ImageHandle,
  UINT64                   Version
  )
{
  EFI_STATUS  Status;
  //
  // @TODO: Add read from devicepath
  // 

  // Verify ApplePeImage signature  
  if (SourceBuffer != NULL && SourceSize != 0) {
    Status = VerifyApplePeImageSignature (SourceBuffer, SourceSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = gBS->LoadImage (
    BootPolicy,
    ParentImageHandle,
    DevicePath,
    SourceBuffer, 
    SourceSize,
    ImageHandle
    );

  return Status;
}

STATIC APPLE_LOAD_IMAGE_PROTOCOL mAppleLoadImageProtocol = {
  AppleLoadImage
};

EFI_STATUS
EFIAPI
AppleLoadImageEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_SERVICES             *gBS;
   
  gBS = SystemTable->BootServices;

  Status = gBS->InstallProtocolInterface(&Handle, &gAppleLoadImageProtocolGuid, 0, &mAppleLoadImageProtocol);

  return EFI_SUCCESS;
}
