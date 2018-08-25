/** @file
Firmware volume driver that overrides the EFI_FIRMWARE_VOLUME_PROTOCOL
and injects images for boot.efi/FileVault 2.

Copyright (C) 2016 Sergey Slice.<BR>
Portions copyright (C) 2016-2018 Alex James.<BR>
Portions copyright (C) 2018 savvas.<BR>
Portions copyright (C) 2006-2014 Intel Corporation.<BR>


All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Framework/FirmwareVolumeImageFormat.h>
#include <Pi/PiFirmwareFile.h>
#include <Pi/PiFirmwareVolume.h>
#include <Protocol/FirmwareVolume.h>
#include <Protocol/FirmwareVolume2.h>
#include "FirmwareVolumeInject.h"

//
// Original functions from FirmwareVolume protocol
//
STATIC FRAMEWORK_EFI_FV_GET_ATTRIBUTES  mGetVolumeAttributes      = NULL;
STATIC FRAMEWORK_EFI_FV_SET_ATTRIBUTES  mSetVolumeAttributes      = NULL;
STATIC FRAMEWORK_EFI_FV_READ_FILE       mReadFile                 = NULL;
STATIC FRAMEWORK_EFI_FV_READ_SECTION    mReadSection              = NULL;
STATIC FRAMEWORK_EFI_FV_WRITE_FILE      mWriteFile                = NULL;
STATIC FRAMEWORK_EFI_FV_GET_NEXT_FILE   mGetNextFile              = NULL;
STATIC EFI_FIRMWARE_VOLUME2_PROTOCOL    *mFirmwareVolume2Protocol = NULL;

STATIC
FRAMEWORK_EFI_FV_ATTRIBUTES
Fv2AttributesToFvAttributes (
  IN  EFI_FV_ATTRIBUTES Fv2Attributes
  )
{
  //
  // Clear those filed that is not defined in Framework FV spec and Alignment conversion.
  //
  return (Fv2Attributes & 0x1ff) | ((UINTN) EFI_FV_ALIGNMENT_2 << RShiftU64((Fv2Attributes & EFI_FV2_ALIGNMENT), 16));
}

STATIC
EFI_STATUS
EFIAPI
GetVolumeAttributesEx (
  IN  EFI_FIRMWARE_VOLUME_PROTOCOL *This,
  OUT FRAMEWORK_EFI_FV_ATTRIBUTES  *Attributes
  )
{
  EFI_STATUS  Status;

  if (mGetVolumeAttributes != NULL) {
    Status = mGetVolumeAttributes (This, Attributes);
  } else if (mFirmwareVolume2Protocol != NULL) {
    Status = mFirmwareVolume2Protocol->GetVolumeAttributes (
      mFirmwareVolume2Protocol,
      Attributes
      );
    if (!EFI_ERROR (Status) && Attributes) {
      *Attributes = Fv2AttributesToFvAttributes (*Attributes);
    }
  } else {
    //
    // The firmware volume is configured to disallow reads.
    // UEFI PI Specification 1.6, page 90
    //    
    Status = EFI_SUCCESS;
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
SetVolumeAttributesEx (
  IN     EFI_FIRMWARE_VOLUME_PROTOCOL *This,
  IN OUT FRAMEWORK_EFI_FV_ATTRIBUTES  *Attributes
  )
{
  EFI_STATUS         Status;
  EFI_FV_ATTRIBUTES  Fv2Attributes = 0;

  if (mSetVolumeAttributes != NULL) {
    Status = mSetVolumeAttributes (This, Attributes);  
  } else if (mFirmwareVolume2Protocol != NULL) {
    if (Attributes) {
      Fv2Attributes = *Attributes & 0x1FFU;
    }

    Status = mFirmwareVolume2Protocol->SetVolumeAttributes (
      mFirmwareVolume2Protocol,
      Attributes
      );

    if (Attributes) {
      *Attributes = Fv2AttributesToFvAttributes (Fv2Attributes);
    }
  } else {
    //
    // The firmware volume is configured to disallow reads.
    // UEFI PI Specification 1.6, page 96
    //        
    Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
ReadFileEx (
  IN     EFI_FIRMWARE_VOLUME_PROTOCOL *This,
  IN     EFI_GUID                     *NameGuid,
  IN OUT VOID                         **Buffer,
  IN OUT UINTN                        *BufferSize,
  OUT    EFI_FV_FILETYPE              *FoundType,
  OUT    EFI_FV_FILE_ATTRIBUTES       *FileAttributes,
  OUT    UINT32                       *AuthenticationStatus
  )
{
  EFI_STATUS  Status;

  if (mReadFile != NULL) {
    Status = mReadFile (
      This,
      NameGuid,
      Buffer,
      BufferSize,
      FoundType,
      FileAttributes,
      AuthenticationStatus
      );        
  } else if (mFirmwareVolume2Protocol != NULL) {
    Status = mFirmwareVolume2Protocol->ReadFile (
      mFirmwareVolume2Protocol,
      NameGuid,
      Buffer,
      BufferSize,
      FoundType,
      FileAttributes,
      AuthenticationStatus
      );

    if (FileAttributes) {
      //
      // For Framework FV attrbutes, only alignment fields are valid.
      //
      *FileAttributes = *FileAttributes & EFI_FV_FILE_ATTRIB_ALIGNMENT;
    }
  } else {
    //
    // The firmware volume is configured to disallow reads.
    // UEFI PI Specification 1.6, page 98
    //  
    Status = EFI_ACCESS_DENIED;
  }


  return Status;
}

STATIC
EFI_STATUS
EFIAPI
ReadSectionEx (
  IN     EFI_FIRMWARE_VOLUME_PROTOCOL *This,
  IN     EFI_GUID                     *NameGuid,
  IN     EFI_SECTION_TYPE             SectionType,
  IN     UINTN                        SectionInstance,
  IN OUT VOID                         **Buffer,
  IN OUT UINTN                        *BufferSize,
  OUT    UINT32                       *AuthenticationStatus
  )
{
  EFI_STATUS Status;

  if (!Buffer || !BufferSize || !AuthenticationStatus) {
    return EFI_INVALID_PARAMETER;
  }

  if (CompareGuid (NameGuid, &gAppleArrowCursorImageGuid)) {
    *BufferSize = sizeof (mAppleArrowCursorImage);
    Status = gBS->AllocatePool (EfiBootServicesData, *BufferSize, (VOID **)Buffer);
    if (!EFI_ERROR (Status)) {
      gBS->CopyMem (*Buffer, &mAppleArrowCursorImage, *BufferSize);
    }

    *AuthenticationStatus = 0;
    return Status;

  } else if (CompareGuid (NameGuid, &gAppleArrowCursor2xImageGuid)) {
    *BufferSize = sizeof (mAppleArrowCursor2xImage);
    Status = gBS->AllocatePool (EfiBootServicesData, *BufferSize, (VOID **)Buffer);
    if (!EFI_ERROR (Status)) {
      gBS->CopyMem (*Buffer, &mAppleArrowCursor2xImage, *BufferSize);
    }

    *AuthenticationStatus = 0;
    return Status;

  } else if (CompareGuid (NameGuid, &gAppleImageListGuid)) {
    *BufferSize = sizeof (mAppleImageTable);
    Status = gBS->AllocatePool (EfiBootServicesData, *BufferSize, (VOID **)Buffer);
    if (!EFI_ERROR (Status)) {
      gBS->CopyMem (*Buffer, &mAppleImageTable, *BufferSize);
    }

    *AuthenticationStatus = 0;
    return Status;
  }
  if (mReadSection != NULL) {
    Status = mReadSection (
      This,
      NameGuid,
      SectionType,
      SectionInstance,
      Buffer,
      BufferSize,
      AuthenticationStatus
      );
  } else if (mFirmwareVolume2Protocol != NULL) {
    Status = mFirmwareVolume2Protocol->ReadSection (
      mFirmwareVolume2Protocol,
      NameGuid,
      SectionType,
      SectionInstance,
      Buffer,
      BufferSize,
      AuthenticationStatus
      );
  } else {
    //
    // The firmware volume is configured to disallow reads.
    // UEFI PI Specification 1.6, page 98
    //
    Status = EFI_ACCESS_DENIED;
  }

  return Status;
}

EFI_STATUS
EFIAPI
WriteFileEx (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL     *This,
  IN UINT32                           NumberOfFiles,
  IN FRAMEWORK_EFI_FV_WRITE_POLICY    WritePolicy,
  IN FRAMEWORK_EFI_FV_WRITE_FILE_DATA *FileData
  )
{
  EFI_STATUS              Status;
  EFI_FV_WRITE_FILE_DATA  *PiFileData;
  UINTN                   Index;

  if (mWriteFile != NULL) {
    Status = mWriteFile (This, NumberOfFiles, WritePolicy, FileData);
  } else if (mFirmwareVolume2Protocol != NULL && FileData != NULL) {
    PiFileData = AllocateCopyPool (sizeof (EFI_FV_WRITE_FILE_DATA), FileData);

    if (PiFileData) {
      //
      // Framework Spec assume firmware files are Memory-Mapped.
      //
      for (Index = 0; Index < NumberOfFiles; Index++) {
        PiFileData[Index].FileAttributes |= EFI_FV_FILE_ATTRIB_MEMORY_MAPPED;
      }

      Status = mFirmwareVolume2Protocol->WriteFile (
        mFirmwareVolume2Protocol,
        NumberOfFiles,
        WritePolicy,
        PiFileData
        );

      FreePool (PiFileData);
    } else {
      //
      // Choose the most sensible error code. Do not use EFI_OUT_OF_RESOURCES,
      // because we are not guaranteed to have no space on a target volume.
      //
      Status = EFI_DEVICE_ERROR;
    }
  } else if (FileData) {
    //
    // The firmware volume is configured to disallow writes.
    // According UEFI PI Specification 1.6, page 101
    //
    Status = EFI_WRITE_PROTECTED;
  } else {
    Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

EFI_STATUS
EFIAPI
GetNextFileEx (
  IN     EFI_FIRMWARE_VOLUME_PROTOCOL *This,
  IN OUT VOID                         *Key,
  IN OUT EFI_FV_FILETYPE              *FileType,
  OUT    EFI_GUID                     *NameGuid,
  OUT    EFI_FV_FILE_ATTRIBUTES       *Attributes,
  OUT    UINTN                        *Size
  )
{
  EFI_STATUS Status;

  if (mGetNextFile != NULL) {
    Status = mGetNextFile (This, Key, FileType, NameGuid, Attributes, Size);
  } else if (mFirmwareVolume2Protocol != NULL) {
    Status = mFirmwareVolume2Protocol->GetNextFile (
      mFirmwareVolume2Protocol,
      Key,
      FileType,
      NameGuid,
      Attributes,
      Size
      );

    if (Attributes) {
      //
      // For Framework FV attrbutes, only alignment fields are valid.
      //
      *Attributes = *Attributes & EFI_FV_FILE_ATTRIB_ALIGNMENT;
    }
  } else {
    //
    // The firmware volume is configured to disallow reads.
    // According UEFI PI Specification 1.6, page 101
    // 
    Status = EFI_ACCESS_DENIED;
  }

  return Status;
}

STATIC
EFI_FIRMWARE_VOLUME_PROTOCOL
mFirmwareVolume = {
  GetVolumeAttributesEx,
  SetVolumeAttributesEx,
  ReadFileEx,
  ReadSectionEx,
  WriteFileEx,
  GetNextFileEx,
  16,
  NULL
};

/**
  InitializeFirmwareVolumeInject

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS          The entry point is executed successfully.
  @retval EFI_ALREADY_STARTED  The protocol has already been installed.
**/
EFI_STATUS
EFIAPI
InitializeFirmwareVolumeInject (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_FIRMWARE_VOLUME_PROTOCOL  *FirmwareVolumeInterface  = NULL;
  EFI_HANDLE                    NewHandle                 = NULL;

  Status = gBS->LocateProtocol (
    &gEfiFirmwareVolumeProtocolGuid,
    NULL,
    (VOID **) &FirmwareVolumeInterface
    );

  if (EFI_ERROR (Status)) {
    //
    // This is a rough workaround for MSI 100 and 200 series motherboards.
    // These boards do not have Firmware Volume protocol, yet their
    // dispatcher protocol, responsible for loading BIOS UI elements,
    // prefers Firmware Volume to Firmware Volume 2.
    // For this reason we implement a subset of FvOnFv2Thunk when
    // Firmware Volume 2 is already present.
    //
    Status = gBS->LocateProtocol (
      &gEfiFirmwareVolume2ProtocolGuid,
      NULL,
      (VOID **) &mFirmwareVolume2Protocol
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Failed to locate firmware volume 2 - %r", Status));
    }

    Status = gBS->InstallMultipleProtocolInterfaces (
      &NewHandle,
      &gEfiFirmwareVolumeProtocolGuid,
      &mFirmwareVolume,
      NULL
      );
  } else {
    //
    // Remember pointers to original functions
    //
    mGetVolumeAttributes = FirmwareVolumeInterface->GetVolumeAttributes;
    mSetVolumeAttributes = FirmwareVolumeInterface->SetVolumeAttributes;
    mReadFile            = FirmwareVolumeInterface->ReadFile;
    mReadSection         = FirmwareVolumeInterface->ReadSection;
    mWriteFile           = FirmwareVolumeInterface->WriteFile;
    mGetNextFile         = FirmwareVolumeInterface->GetNextFile;
    //
    // Override with our wrappers
    //
    FirmwareVolumeInterface->GetVolumeAttributes = GetVolumeAttributesEx;
    FirmwareVolumeInterface->SetVolumeAttributes = SetVolumeAttributesEx;
    FirmwareVolumeInterface->ReadFile            = ReadFileEx;
    FirmwareVolumeInterface->ReadSection         = ReadSectionEx;
    FirmwareVolumeInterface->WriteFile           = WriteFileEx;
    FirmwareVolumeInterface->GetNextFile         = GetNextFileEx;
  }

  return Status;
}
