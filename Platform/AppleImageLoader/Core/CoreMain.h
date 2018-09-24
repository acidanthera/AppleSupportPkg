/** @file
  The internal header file includes the common header files, defines
  internal structure and functions used by DxeCore module.
Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef _CORE_MAIN_H_
#define _CORE_MAIN_H_

#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>

VOID
UnprotectUefiImage (
  IN EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage,
  IN EFI_DEVICE_PATH_PROTOCOL    *LoadedImageDevicePath
  );

VOID
ProtectUefiImage (
  IN EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage,
  IN EFI_DEVICE_PATH_PROTOCOL    *LoadedImageDevicePath
  );

EFI_STATUS
EFIAPI
CoreLoadImage (
  IN  BOOLEAN                    BootPolicy,
  IN  EFI_HANDLE                 ParentImageHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL   *FilePath,
  IN  VOID                       *SourceBuffer   OPTIONAL,
  IN  UINTN                      SourceSize,
  OUT EFI_HANDLE                 *ImageHandle
  );

EFI_STATUS
EFIAPI
CoreStartImage (
  IN  EFI_HANDLE  ImageHandle,
  OUT UINTN       *ExitDataSize,
  OUT CHAR16      **ExitData   OPTIONAL
  );

EFI_STATUS
EFIAPI
CoreExit (
  IN EFI_HANDLE  ImageHandle,
  IN EFI_STATUS  Status,
  IN UINTN       ExitDataSize,
  IN CHAR16      *ExitData   OPTIONAL
  );

#endif