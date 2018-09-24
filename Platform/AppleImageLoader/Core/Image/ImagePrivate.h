/** @file
  Data structure to load and unload PeImage.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef _IMAGE_PRIVATE_H_
#define _IMAGE_PRIVATE_H_

#define LOAD_PE32_IMAGE_PRIVATE_DATA_SIGNATURE  SIGNATURE_32('l','p','e','i')

#define LOAD_PE32_IMAGE_PRIVATE_DATA_FROM_THIS(a) \
          CR(a, LOAD_PE32_IMAGE_PRIVATE_DATA, Pe32Image, LOAD_PE32_IMAGE_PRIVATE_DATA_SIGNATURE)

//
// Private Data Types
//
#define IMAGE_FILE_HANDLE_SIGNATURE       SIGNATURE_32('i','m','g','f')
typedef struct {
  UINTN               Signature;
  BOOLEAN             FreeBuffer;
  VOID                *Source;
  UINTN               SourceSize;
} IMAGE_FILE_HANDLE;


typedef struct {
  UINTN                       Signature;
  /// Image handle
  EFI_HANDLE                  Handle;
  EFI_PE32_IMAGE_PROTOCOL     Pe32Image;
} LOAD_PE32_IMAGE_PRIVATE_DATA;

#define LOADED_IMAGE_PRIVATE_DATA_SIGNATURE   SIGNATURE_32('l','d','r','i')

typedef struct {
  UINTN                       Signature;
  /// Image handle
  EFI_HANDLE                  Handle;
  /// Image type
  UINTN                       Type;
  /// If entrypoint has been called
  BOOLEAN                     Started;
  /// The image's entry point
  EFI_IMAGE_ENTRY_POINT       EntryPoint;
  /// loaded image protocol
  EFI_LOADED_IMAGE_PROTOCOL   Info;
  /// Location in memory
  EFI_PHYSICAL_ADDRESS        ImageBasePage;
  /// Number of pages
  UINTN                       NumberOfPages;
  /// Original fixup data
  CHAR8                       *FixupData;
  /// Tpl of started image
  EFI_TPL                     Tpl;
  /// Status returned by started image
  EFI_STATUS                  Status;
  /// Size of ExitData from started image
  UINTN                       ExitDataSize;
  /// Pointer to exit data from started image
  VOID                        *ExitData;
  /// Pointer to pool allocation for context save/restore
  VOID                        *JumpBuffer;
  /// Pointer to buffer for context save/restore
  BASE_LIBRARY_JUMP_BUFFER    *JumpContext;
  /// Machine type from PE image
  UINT16                      Machine;
  /// EBC Protocol pointer
  EFI_EBC_PROTOCOL            *Ebc;
  /// Runtime image list
  EFI_RUNTIME_IMAGE_ENTRY     *RuntimeData;
  /// Pointer to Loaded Image Device Path Protocol
  EFI_DEVICE_PATH_PROTOCOL    *LoadedImageDevicePath;
  /// PeCoffLoader ImageContext
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  /// Status returned by LoadImage() service.
  EFI_STATUS                  LoadImageStatus;
} LOADED_IMAGE_PRIVATE_DATA;

#define LOADED_IMAGE_PRIVATE_DATA_FROM_THIS(a) \
          CR(a, LOADED_IMAGE_PRIVATE_DATA, Info, LOADED_IMAGE_PRIVATE_DATA_SIGNATURE)

#endif
