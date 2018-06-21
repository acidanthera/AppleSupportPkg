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
#include <Protocol/AppleLoadImage.h>

STATIC EFI_HANDLE       Handle = NULL;

typedef EFI_STATUS (EFIAPI *APPLE_LOAD_IMAGE) (
  IN BOOLEAN BootPolicy,
  IN EFI_HANDLE ParentImageHandle,
  IN EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  IN VOID *SourceBuffer,
  IN UINTN SourceSize,
  IN EFI_HANDLE *ImageHandle,
  IN  UINT64     Version,
  OUT EFI_STATUS Status
);

typedef struct {
  APPLE_LOAD_IMAGE   AppleLoadImageInterface;
} APPLE_LOAD_IMAGE_PROTOCOL;


EFI_STATUS
EFIAPI
AppleLoadImage (
  BOOLEAN                  BootPolicy,
  EFI_HANDLE               ParentImageHandle,
  EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  VOID                     *SourceBuffer,
  UINTN                    SourceSize,
  EFI_HANDLE               *ImageHandle,
  UINT64                   Version,
  EFI_STATUS               Status
  )
{

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

STATIC APPLE_LOAD_IMAGE_PROTOCOL gAppleLoadImageProtocol = {
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

  Status = gBS->InstallProtocolInterface(&Handle, &gAppleLoadImageProtocolGuid, 0, &gAppleLoadImageProtocol);

  return EFI_SUCCESS;
}
