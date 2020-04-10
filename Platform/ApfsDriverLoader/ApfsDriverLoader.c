/** @file

APFS Driver Loader - loads apfs.efi from EfiBootRecord block

Copyright (c) 2017-2018, savvas
Copyright (c) 2018, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <AppleSupportPkgVersion.h>
#include <Uefi/UefiGpt.h>
#include <Guid/OcVariables.h>
#include <Guid/AppleApfsInfo.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/OcAppleImageVerificationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcFileLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/PartitionInfo.h>
#include <Protocol/ApplePartitionInfo.h>
#include <Protocol/ApfsEfiBootRecordInfo.h>
#include <Library/OcMiscLib.h>
#include "ApfsDriverLoader.h"
#include "EfiComponentName.h"

STATIC BOOLEAN  LegacyScan       = FALSE;
STATIC UINT64   LegacyBaseLBA    = 0;

UINT64
ApfsBlockChecksumCalculate (
  UINT32  *Data,
  UINTN   DataSize
  )
{
  UINTN         Index;
  UINT64        Sum1;
  UINT64        Check1;
  UINT64        Sum2;
  UINT64        Check2;
  UINT32        Remainder;
  CONST UINT32  ModValue = 0xFFFFFFFF;

  Sum1 = 0;
  Sum2 = 0;

  for (Index = 0; Index < DataSize / sizeof (UINT32); Index++) {
    DivU64x32Remainder (Sum1 + Data[Index], ModValue, &Remainder);
    Sum1 = Remainder;
    DivU64x32Remainder (Sum2 + Sum1, ModValue, &Remainder);
    Sum2 = Remainder;
  }

  DivU64x32Remainder (Sum1 + Sum2, ModValue, &Remainder);
  Check1 = ModValue - Remainder;
  DivU64x32Remainder (Sum1 + Check1, ModValue, &Remainder);
  Check2 = ModValue - Remainder;

  return (Check2 << 32U) | Check1;
}

//
// Function to check block checksum.
// Returns TRUE if the checksum is valid.
//
BOOLEAN
ApfsBlockChecksumVerify (
  UINT8   *Data,
  UINTN   DataSize
  )
{
  UINT64  NewChecksum;
  UINT64  *CurrChecksum = (UINT64 *) Data;

  NewChecksum = ApfsBlockChecksumCalculate (
    (UINT32 *) (Data + sizeof (UINT64)),
    DataSize - sizeof (UINT64)
    );

  return NewChecksum == *CurrChecksum;
}

EFI_STATUS
EFIAPI
StartApfsDriver (
  IN EFI_HANDLE  ControllerHandle,
  IN VOID        *EfiFileBuffer,
  IN UINTN       EfiFileSize
  )
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 ImageHandle              = NULL;
  EFI_DEVICE_PATH_PROTOCOL   *ParentDevicePath        = NULL;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedApfsDrvImage      = NULL;
  EFI_SYSTEM_TABLE           *NewSystemTable          = NULL;

  if (EfiFileBuffer == NULL
    || EfiFileSize == 0
    || EfiFileSize > (UINT32) 0xFFFFFFFFULL) {
    DEBUG ((DEBUG_VERBOSE, "Broken apfs.efi\n"));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_VERBOSE, "Loading apfs.efi from memory!\n"));

  //
  // Try to retrieve DevicePath
  //
  Status = gBS->HandleProtocol (
    ControllerHandle,
    &gEfiDevicePathProtocolGuid,
    (VOID **) &ParentDevicePath
    );

  if (EFI_ERROR (Status)) {
    ParentDevicePath = NULL;
    DEBUG ((DEBUG_VERBOSE, "ApfsDriver DevicePath not present\n"));
  }

  DEBUG ((DEBUG_VERBOSE, "ImageSize before verification: %lu\n", EfiFileSize));

  DEBUG ((DEBUG_VERBOSE, "Verifying binary signature\n"));
  Status = VerifyApplePeImageSignature (
    EfiFileBuffer,
    &EfiFileSize
    );

  DEBUG ((DEBUG_VERBOSE, "New ImageSize after verification: %lu\n", EfiFileSize));

  if (!EFI_ERROR (Status)) {
    Status = gBS->LoadImage (
      FALSE,
      gImageHandle,
      ParentDevicePath,
      EfiFileBuffer,
      EfiFileSize,
      &ImageHandle
      );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_VERBOSE, "Load image failed with Status: %r\n", Status));
        return Status;
      }
    } else {
      DEBUG ((DEBUG_ERROR, "EfiFile signature invalid\n"));
      return Status;
    }

  Status = gBS->HandleProtocol (
    ImageHandle,
    &gEfiLoadedImageProtocolGuid,
    (VOID *) &LoadedApfsDrvImage
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "Failed to Handle LoadedImage Protool with Status: %r\n", Status));
    return Status;
  }

  //
  // Patch verbose
  //
  NewSystemTable = AllocateNullTextOutSystemTable (gST);

  if (NewSystemTable == NULL) {
      return EFI_OUT_OF_RESOURCES;
  }

  LoadedApfsDrvImage->SystemTable = NewSystemTable;

  Status = gBS->StartImage (
    ImageHandle,
    NULL,
    NULL
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to start ApfsDriver with Status: %r\n", Status));

    //
    // Unload ApfsDriver image from memory
    //
    gBS->UnloadImage (ImageHandle);
    return Status;
  }

  //
  // Connect loaded apfs.efi to controller from which we retrieve it
  //
  gBS->ConnectController (ControllerHandle, NULL, NULL, TRUE);

  return EFI_SUCCESS;
}

//
// Function to parse GPT entries in legacy
//
EFI_STATUS
EFIAPI
LegacyApfsContainerScan (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN OC_DISK_CONTEXT              *DiskContext
  )
{
  EFI_STATUS                  Status;
  UINTN                       Index               = 0;
  UINT8                       *Block              = NULL;
  UINT64                      Lba                 = 0;
  UINT32                      PartitionNumber     = 0;
  UINT32                      PartitionEntrySize  = 0;
  EFI_PARTITION_TABLE_HEADER  *GptHeader          = NULL;
  EFI_PARTITION_ENTRY         *ApfsGptEntry       = NULL;

  DEBUG ((DEBUG_VERBOSE, "LegacyApfsContainerScan: Entrypoint\n"));
  DEBUG ((DEBUG_VERBOSE, "BlockSize: %lu", DiskContext->BlockSize));
  Block = AllocateZeroPool ((UINTN) DiskContext->BlockSize);
  if (Block == NULL) {
    DEBUG ((DEBUG_ERROR, "LegacyApfsContainerScan: Allocation error\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Read GPT header first.
  //
  Status = OcDiskRead (
    DiskContext,
    1,
    DiskContext->BlockSize,
    Block
    );

  if (EFI_ERROR (Status)) {
    FreePool (Block);
    DEBUG ((DEBUG_VERBOSE, "LegacyApfsContainerScan: ReadDisk1 Error %r\n", Status));
    return EFI_DEVICE_ERROR;
  }

  GptHeader = (EFI_PARTITION_TABLE_HEADER *) Block;
  PartitionEntrySize = GptHeader->SizeOfPartitionEntry;

  //
  // Check GPT Header signature.
  //
  if (GptHeader->Header.Signature == EFI_PTAB_HEADER_ID) {
    //
    // Get partitions count.
    //
    PartitionNumber = GptHeader->NumberOfPartitionEntries;
    //
    // Get partitions array start_lba.
    //
    Lba = GptHeader->PartitionEntryLBA;
    //
    // Reallocate Block size to contain all of partition entries.
    //
    FreePool (Block);
    Block = AllocateZeroPool (PartitionNumber * PartitionEntrySize);
    if (Block == NULL) {
      DEBUG ((DEBUG_ERROR, "LegacyApfsContainerScan: Allocation error %r\n", Status));
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    FreePool (Block);
    DEBUG ((DEBUG_VERBOSE, "LegacyApfsContainerScan: Block device not supported%r\n", Status));
    return EFI_UNSUPPORTED;
  }

 Status = OcDiskRead (
    DiskContext,
    Lba,
    PartitionNumber * PartitionEntrySize,
    Block
    );

  if (EFI_ERROR (Status)) {
    FreePool (Block);
    DEBUG ((DEBUG_VERBOSE, "LegacyApfsContainerScan: ReadDisk2 Error %r\n", Status));
    return EFI_DEVICE_ERROR;
  }

  //
  // Analyze partition entries.
  //
  for (Index = 0; Index < (UINTN) PartitionEntrySize * PartitionNumber; Index += PartitionEntrySize) {
    EFI_PARTITION_ENTRY *CurrentEntry = (EFI_PARTITION_ENTRY *) (Block + Index);
    if (CompareGuid (&CurrentEntry->PartitionTypeGUID, &gAppleApfsPartitionTypeGuid)) {
      ApfsGptEntry = CurrentEntry;
      break;
    }

    if (CurrentEntry->StartingLBA == 0ull && CurrentEntry->EndingLBA == 0ull) {
      break;
    }
  }

  if (ApfsGptEntry == NULL)  {
    FreePool (Block);
    DEBUG ((DEBUG_VERBOSE, "LegacyApfsContainerScan: ApfsGptEntry is null\n", Status));
    return EFI_UNSUPPORTED;
  }
  LegacyBaseLBA = ApfsGptEntry->StartingLBA;
  FreePool(Block);

  return EFI_SUCCESS;

}

STATIC
EFI_STATUS
CheckOpenCoreScanPolicy (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS  Status;
  UINT32      ScanPolicy;
  UINT32      OcScanPolicy;
  UINTN       OcScanPolicySize;

  //
  // If scan policy is missing, just ignore.
  //
  OcScanPolicy = 0;
  OcScanPolicySize = sizeof (OcScanPolicy);
  Status = gRT->GetVariable (
    OC_SCAN_POLICY_VARIABLE_NAME,
    &gOcVendorVariableGuid,
    NULL,
    &OcScanPolicySize,
    &OcScanPolicy
    );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  //
  // If filesystem limitations are set and APFS is not allowed,
  // report failure.
  //
  if ((OcScanPolicy & OC_SCAN_FILE_SYSTEM_LOCK) != 0
    && (OcScanPolicy & OC_SCAN_ALLOW_FS_APFS) == 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // If device type locking is set and this device is not allowed,
  // report failure.
  //
  if ((OcScanPolicy & OC_SCAN_DEVICE_LOCK) != 0) {
    ScanPolicy = OcGetDevicePolicyType (Handle, NULL);
    if ((ScanPolicy & OcScanPolicy) == 0) {
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}

/**

  Routine Description:

    Test to see if this driver can load ApfsDriver from Block device.
    ControllerHandle must support both Disk IO and Block IO protocols.

  Arguments:

    This                  - Protocol instance pointer.
    ControllerHandle      - Handle of device to test.
    RemainingDevicePath   - Not used.

  Returns:

    EFI_SUCCESS           - This driver supports this device.
    EFI_ALREADY_STARTED   - This driver is already running on this device.
    other                 - This driver does not support this device.

**/
EFI_STATUS
EFIAPI
ApfsDriverLoaderSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                    Status;
  APPLE_PARTITION_INFO_PROTOCOL *ApplePartitionInfo          = NULL;
  EFI_PARTITION_INFO_PROTOCOL   *Edk2PartitionInfo           = NULL;
  OC_DISK_CONTEXT               DiskContext;

  Status = CheckOpenCoreScanPolicy (ControllerHandle);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gApfsEfiBootRecordInfoProtocolGuid,
    NULL,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
    );

   if (!EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
   }

  Status = OcDiskInitializeContext (&DiskContext, ControllerHandle, TRUE, TRUE);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  if (LegacyScan) {
    return LegacyApfsContainerScan (This, ControllerHandle, &DiskContext);
  }

  //
  // We check EfiPartitionInfoProtocol and ApplePartitionInfoProtocol
  //

  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gAppleApfsPartitionTypeGuid,
    (VOID **) &Edk2PartitionInfo,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

  if (EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
      ControllerHandle,
      &gEfiPartitionInfoProtocolGuid,
      (VOID **) &Edk2PartitionInfo,
      This->DriverBindingHandle,
      ControllerHandle,
      EFI_OPEN_PROTOCOL_GET_PROTOCOL
      );

    if (EFI_ERROR (Status)) {
      Status = gBS->OpenProtocol (
        ControllerHandle,
        &gApplePartitionInfoProtocolGuid,
        (VOID **) &ApplePartitionInfo,
        This->DriverBindingHandle,
        ControllerHandle,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );

      if (EFI_ERROR (Status)) {
        ApplePartitionInfo = NULL;
        return Status;
      }

      if (ApplePartitionInfo != NULL) {
        //
        // Verify GPT entry GUID
        //
        if (CompareGuid ((EFI_GUID *) ApplePartitionInfo->PartitionType,
                         &gAppleApfsPartitionTypeGuid)) {
          return EFI_UNSUPPORTED;
        }
      }
    } else {

      //
      // Verify PartitionType
      //
      if (Edk2PartitionInfo->Type != PARTITION_TYPE_GPT) {
        return EFI_UNSUPPORTED;
      }

      //
      // Verify GPT entry GUID
      //
      if (CompareGuid (&Edk2PartitionInfo->Info.Gpt.PartitionTypeGUID,
                       &gAppleApfsPartitionTypeGuid)) {
        return EFI_UNSUPPORTED;
      }
    }
  }

  return Status;
}

/**
  Routine Description:

    Start this driver on ControllerHandle by opening a Block IO and Disk IO
    protocol, reading ApfsContainer if present.

  Arguments:

    This                  - Protocol instance pointer.
    ControllerHandle      - Handle of device to bind driver to.
    RemainingDevicePath   - Not used.

  Returns:

    EFI_SUCCESS           - This driver is added to DeviceHandle.
    EFI_ALREADY_STARTED   - This driver is already running on DeviceHandle.
    EFI_OUT_OF_RESOURCES  - Can not allocate the memory.
    other                 - This driver does not support this device.

**/
EFI_STATUS
EFIAPI
ApfsDriverLoaderStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                        Status;
  UINTN                             Index                        = 0;
  UINTN                             CurPos                       = 0;
  UINT32                            ApfsBlockSize                = 0;
  UINT8                             *ApfsBlock                   = NULL;
  EFI_GUID                          ContainerUuid;
  UINT64                            EfiBootRecordBlockLBA        = 0;
  UINT64                            EfiBootRecordBlockPtr        = 0;
  APFS_EFI_BOOT_RECORD              *EfiBootRecordBlock          = NULL;
  APFS_CSB                          *ContainerSuperBlock         = NULL;
  UINT64                            EfiFileCurrentExtentOffset   = 0;
  UINT8                             *EfiFileBuffer               = NULL;
  UINTN                             EfiFileCurrentExtentSize     = 0;
  APFS_DRIVER_INFO_PRIVATE_DATA     *Private                     = NULL;
  APFS_EFIBOOTRECORD_LOCATION_INFO  *EfiBootRecordLocationInfo   = NULL;
  OC_DISK_CONTEXT                   DiskContext;

  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gApfsEfiBootRecordInfoProtocolGuid,
    NULL,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
    );

   if (!EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
   }

  DEBUG ((DEBUG_VERBOSE, "Apfs Container found.\n"));

  Status = OcDiskInitializeContext (
    &DiskContext,
    ControllerHandle,
    TRUE,
    TRUE
    );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  ApfsBlock = AllocateZeroPool (2048);
  if (ApfsBlock == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Read ContainerSuperblock and get ApfsBlockSize.
  //
  Status = OcDiskRead (
    &DiskContext,
    LegacyBaseLBA,
    2048,
    ApfsBlock
    );

  if (EFI_ERROR (Status)) {
    FreePool (ApfsBlock);
    return EFI_DEVICE_ERROR;
  }

  ContainerSuperBlock = (APFS_CSB *) ApfsBlock;

  //
  // Verify ObjectOid and ObjectType
  //
  DEBUG ((DEBUG_VERBOSE, "ObjectId: %04x\n", ContainerSuperBlock->BlockHeader.ObjectOid ));
  DEBUG ((DEBUG_VERBOSE, "ObjectType: %04x\n", ContainerSuperBlock->BlockHeader.ObjectType ));
  if (ContainerSuperBlock->BlockHeader.ObjectOid != 1
      || ContainerSuperBlock->BlockHeader.ObjectType != 0x80000001) {
    return EFI_UNSUPPORTED;
  }

  //
  // Verify ContainerSuperblock magic.
  //
  DEBUG ((DEBUG_VERBOSE, "CsbMagic: %04x\n", ContainerSuperBlock->Magic));
  DEBUG ((DEBUG_VERBOSE, "Should be: %04x\n", APFS_CSB_SIGNATURE));

  if (ContainerSuperBlock->Magic != APFS_CSB_SIGNATURE) {
    FreePool (ApfsBlock);
    return EFI_UNSUPPORTED;
  }

  //
  // Get ApfsBlockSize.
  //
  ApfsBlockSize = ContainerSuperBlock->BlockSize;

  DEBUG ((
    DEBUG_VERBOSE,
    "Container Blocksize: %u bytes\n",
    ApfsBlockSize
    ));
  DEBUG ((
    DEBUG_VERBOSE,
    "ContainerSuperblock checksum: %08llx \n",
    ContainerSuperBlock->BlockHeader.Checksum
    ));

  //
  // Take pointer to EfiBootRecordBlock.
  //
  EfiBootRecordBlockPtr = ContainerSuperBlock->EfiBootRecordBlock;

  DEBUG ((
    DEBUG_INFO,
    "EfiBootRecord located at: %llu block\n",
    EfiBootRecordBlockPtr
    ));

  //
  // Free ApfsBlock and allocate one of a correct size.
  //
  FreePool (ApfsBlock);
  ApfsBlock = AllocateZeroPool (ApfsBlockSize);
  if (ApfsBlock == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Read full ContainerSuperblock with known BlockSize.
  //
  Status = OcDiskRead (
    &DiskContext,
    LegacyBaseLBA,
    ApfsBlockSize,
    ApfsBlock
    );

  if (EFI_ERROR (Status)) {
    FreePool (ApfsBlock);
    return EFI_DEVICE_ERROR;
  }

  //
  // Verify ContainerSuperblock checksum.
  //
  if (!ApfsBlockChecksumVerify ((UINT8 *) ApfsBlock, ApfsBlockSize)) {
    FreePool (ApfsBlock);
    return EFI_UNSUPPORTED;
  }

  //
  // Extract Container UUID
  //
  ContainerSuperBlock = (APFS_CSB *)ApfsBlock;
  CopyMem (&ContainerUuid, &ContainerSuperBlock->Uuid, sizeof (EFI_GUID));

  //
  // Calculate Offset of EfiBootRecordBlock
  //
  EfiBootRecordBlockLBA = DivU64x32 (
    MultU64x32 (EfiBootRecordBlockPtr, ApfsBlockSize),
    DiskContext.BlockSize
    ) + LegacyBaseLBA;

  DEBUG ((
    DEBUG_VERBOSE,
    "EfiBootRecordBlock LBA: %08llx \n",
     EfiBootRecordBlockLBA
     ));

  //
  // Read EfiBootRecordBlock.
  //
  Status = OcDiskRead (
    &DiskContext,
    EfiBootRecordBlockLBA,
    ApfsBlockSize,
    ApfsBlock
    );

  if (EFI_ERROR (Status)) {
    FreePool (ApfsBlock);
    return EFI_DEVICE_ERROR;
  }

  //
  // Verify EfiBootRecordBlock checksum.
  //
  if (!ApfsBlockChecksumVerify (ApfsBlock, ApfsBlockSize)) {
    FreePool (ApfsBlock);
    return EFI_UNSUPPORTED;
  }

  EfiBootRecordBlock = (APFS_EFI_BOOT_RECORD *) ApfsBlock;
  if (EfiBootRecordBlock->Magic != APFS_EFIBOOTRECORD_SIGNATURE) {
    FreePool (ApfsBlock);
    return EFI_UNSUPPORTED;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "EfiBootRecordBlock checksum: %08llx\n",
    EfiBootRecordBlock->BlockHeader.Checksum
    ));

  //
  // Loop over extents inside EfiBootRecord
  //        EFI embedded driver could be defragmented across whole container
  //
  DEBUG ((
    DEBUG_VERBOSE,
    "EFI embedded driver extents number %llu\n",
    EfiBootRecordBlock->NumOfExtents
    ));

  //
  // Read EFI embedded file from extents
  //
  for (Index = 0; Index < EfiBootRecordBlock->NumOfExtents; Index++) {
    DEBUG ((
        DEBUG_VERBOSE,
        "EFI embedded driver extent located at: %llu block\n with size %llu\n",
        EfiBootRecordBlock->RecordExtents[Index].StartPhysicalAddr,
        EfiBootRecordBlock->RecordExtents[Index].BlockCount
        ));

    EfiFileCurrentExtentOffset = MultU64x32 (
                                EfiBootRecordBlock->RecordExtents[Index].StartPhysicalAddr,
                                ApfsBlockSize
                                ) + LegacyBaseLBA * DiskContext.BlockSize;

    EfiFileCurrentExtentSize = (UINTN) MultU64x32 (
                                EfiBootRecordBlock->RecordExtents[Index].BlockCount,
                                ApfsBlockSize
                                );

    //
    // Adjust buffer size
    //
    EfiFileBuffer = ReallocatePool (
                      CurPos,
                      CurPos + EfiFileCurrentExtentSize,
                      EfiFileBuffer
                      );

    if (EfiFileBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Read current extent
    //
    Status = OcDiskRead (
      &DiskContext,
      DivU64x32 (EfiFileCurrentExtentOffset, DiskContext.BlockSize),
      EfiFileCurrentExtentSize,
      EfiFileBuffer + CurPos
      );

    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
    //
    // Sum size for buffer offset
    //
    CurPos += EfiFileCurrentExtentSize;
    
  }

  //
  // Drop tail
  // We do it because we read blocksize aligned data
  // Apfs driver size given in bytes
  //
  if (CurPos > EfiBootRecordBlock->EfiFileLen) {
    //
    // Zero tail
    //
    ZeroMem (
      EfiFileBuffer + EfiBootRecordBlock->EfiFileLen,
      CurPos - EfiBootRecordBlock->EfiFileLen
      );
    //
    // Reallocate buffer
    //
    EfiFileBuffer = ReallocatePool (
                      CurPos,
                      EfiBootRecordBlock->EfiFileLen,
                      EfiFileBuffer
                      );
  }

  //
  // Fill public AppleFileSystemEfiBootRecordInfo protocol interface
  //
  Private = AllocatePool (sizeof (APFS_DRIVER_INFO_PRIVATE_DATA));
  if (Private == NULL) {
    FreePool (ApfsBlock);
    return EFI_OUT_OF_RESOURCES;
  }

  Private->ControllerHandle  = ControllerHandle;
  EfiBootRecordLocationInfo = &Private->EfiBootRecordLocationInfo;
  EfiBootRecordLocationInfo->ControllerHandle = ControllerHandle;
  CopyMem (&EfiBootRecordLocationInfo->ContainerUuid, &ContainerUuid, 16);

  Status = gBS->InstallMultipleProtocolInterfaces (
    &Private->ControllerHandle,
    &gApfsEfiBootRecordInfoProtocolGuid,
    &Private->EfiBootRecordLocationInfo,
    NULL
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_VERBOSE,
      "ApfsEfiBootRecordInfoProtocol install failed with Status %r\n",
      Status
      ));
    if (EfiFileBuffer != NULL) {
      FreePool (EfiFileBuffer);
    }
    if (Private != NULL) {
      FreePool (Private);
    }
    FreePool (ApfsBlock);

    return Status;
  }

  Status = StartApfsDriver (
    ControllerHandle,
    EfiFileBuffer,
    EfiBootRecordBlock->EfiFileLen
    );

  FreePool (ApfsBlock);

  if (EFI_ERROR (Status)) {
    gBS->UninstallProtocolInterface (
      ControllerHandle,
      &gApfsEfiBootRecordInfoProtocolGuid,
      NULL
      );

    if (EfiFileBuffer != NULL) {
      FreePool (EfiFileBuffer);
    }
    if (Private != NULL) {
      FreePool (Private);
    }

    return EFI_UNSUPPORTED;
  }

  //
  // Free memory and close DiskIo protocol
  //
  if (EfiFileBuffer != NULL) {
    FreePool (EfiFileBuffer);
  }
  if (Private != NULL) {
    FreePool (Private);
  }

  return EFI_SUCCESS;
}

/**

  Routine Description:
    Stop this driver on ControllerHandle.

  Arguments:
    This                  - Protocol instance pointer.
    ControllerHandle      - Handle of device to stop driver on.
    NumberOfChildren      - Not used.
    ChildHandleBuffer     - Not used.

  Returns:
    EFI_SUCCESS           - This driver is removed DeviceHandle.
    other                 - This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
ApfsDriverLoaderStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                        Status;
  APFS_EFIBOOTRECORD_LOCATION_INFO  *EfiBootRecordLocationInfo = NULL;

  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gApfsEfiBootRecordInfoProtocolGuid,
    (VOID **) &EfiBootRecordLocationInfo,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

  if (!EFI_ERROR (Status)) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
      EfiBootRecordLocationInfo->ControllerHandle,
      &gApfsEfiBootRecordInfoProtocolGuid,
      EfiBootRecordLocationInfo,
      NULL
      );
  }

  return Status;
}

//
// Interface structure for the EFI Driver Binding protocol.
// According to UEFI Spec 2.6 , we should define Supported, Start, Stop function for
// DriverBinding
//
EFI_DRIVER_BINDING_PROTOCOL gApfsDriverLoaderDriverBinding = {
  ApfsDriverLoaderSupported,
  ApfsDriverLoaderStart,
  ApfsDriverLoaderStop,
  0x10,
  NULL,
  NULL,
};

/**

  Routine Description:

    Register Driver Binding protocol for this driver.

  Arguments:

    ImageHandle           - Handle for the image of this driver.
    SystemTable           - Pointer to the EFI System Table.

  Returns:

    EFI_SUCCESS           - Driver loaded.
    other                 - Driver not loaded.

**/
EFI_STATUS
EFIAPI
ApfsDriverLoaderInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                          Status;
  VOID                                *PartitionInfoInterface;

  DEBUG ((
    DEBUG_INFO,
    "Starting ApfsDriverLoader ver. %s\n",
    APPLE_SUPPORT_VERSION
    ));

  //
  // Check that PartitionInfo protocol present
  // If not present use Legacy scan
  //
  Status = gBS->LocateProtocol (
    &gAppleApfsPartitionTypeGuid,
    NULL,
    (VOID **) &PartitionInfoInterface
    );
  if (Status == EFI_NOT_FOUND) {
    Status = gBS->LocateProtocol (
      &gEfiPartitionInfoProtocolGuid,
      NULL,
      (VOID **) &PartitionInfoInterface
      );
    if (Status == EFI_NOT_FOUND) {
      Status = gBS->LocateProtocol (
        &gApplePartitionInfoProtocolGuid,
        NULL,
        (VOID **) &PartitionInfoInterface
      );
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_VERBOSE,
      "No partition info protocol, using Legacy scan\n"
      ));
    LegacyScan = TRUE;
  }

  //
  // Install Driver Binding Instance
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
    ImageHandle,
    SystemTable,
    &gApfsDriverLoaderDriverBinding,
    ImageHandle,
    &gApfsDriverLoaderComponentName,
    &gApfsDriverLoaderComponentName2
    );

  return Status;
}
