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

#include "ApfsDriverLoader.h"
#include "Version.h"


STATIC BOOLEAN                     LegacyScan                     = FALSE;
STATIC UINT64                      LegacyBaseOffset               = 0;


EFI_STATUS
EFIAPI
StartApfsDriver (
  IN EFI_HANDLE ControllerHandle,
  IN VOID       *AppleFileSystemDriverBuffer,
  IN UINTN      AppleFileSystemDriverSize
  )
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 ImageHandle              = NULL;
  EFI_DEVICE_PATH_PROTOCOL   *ParentDevicePath        = NULL;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedApfsDrvImage      = NULL;
  EFI_SYSTEM_TABLE           *NewSystemTable          = NULL;
  APPLE_LOAD_IMAGE_PROTOCOL  *AppleLoadImageInterface = NULL;

  if (AppleFileSystemDriverBuffer == NULL || AppleFileSystemDriverSize == 0) {
    DEBUG ((DEBUG_WARN, "Broken apfs.efi\n"));
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
      DEBUG ((DEBUG_WARN, "ApfsDriver DevicePath not present\n"));
  }

  Status = gBS->LocateProtocol (
    &gAppleLoadImageProtocolGuid,
    NULL,
    (VOID **) &AppleLoadImageInterface
    );

  if (!EFI_ERROR(Status)) {
    Status = AppleLoadImageInterface->LoadImage (
      FALSE,
      gImageHandle,
      ParentDevicePath,
      AppleFileSystemDriverBuffer, 
      AppleFileSystemDriverSize,
      &ImageHandle,
      0x01
      );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "Apple Load image failed with Status: %r\n", Status));
        return Status;
      }    
  } else {
    DEBUG ((DEBUG_WARN, "SECURITY VIOLATION!!! Loading image without signature check!"));
    Status = gBS->LoadImage (
      FALSE,
      gImageHandle,
      ParentDevicePath,
      AppleFileSystemDriverBuffer, 
      AppleFileSystemDriverSize,
      &ImageHandle
      );    
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "Load image failed with Status: %r\n", Status));
        return Status;
      }
  }

  Status = gBS->HandleProtocol (
    ImageHandle,
    &gEfiLoadedImageProtocolGuid,
    (VOID *) &LoadedApfsDrvImage
    ); 

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Failed to Handle LoadedImage Protool with Status: %r\n", Status));
    return Status;
  }

  //
  // Patch verbose
  //
  NewSystemTable = (EFI_SYSTEM_TABLE *) AllocateZeroPool (gST->Hdr.HeaderSize);

  if (NewSystemTable == NULL) {
      return EFI_OUT_OF_RESOURCES;
  }  

  CopyMem ((VOID *) NewSystemTable, gST, gST->Hdr.HeaderSize);
  NewSystemTable->ConOut = &mNullTextOutputProtocol;
  NewSystemTable->Hdr.CRC32 = 0;
  
  Status = gBS->CalculateCrc32 (
    NewSystemTable, 
    NewSystemTable->Hdr.HeaderSize, 
    &NewSystemTable->Hdr.CRC32
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Failed to calculated new system table CRC32 with Status: %r\n", Status));
    return Status;
  }

  LoadedApfsDrvImage->SystemTable = NewSystemTable;

  Status = gBS->StartImage (
    ImageHandle, 
    NULL, 
    NULL
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Failed to start ApfsDriver with Status: %r\n", Status));

    //
    // Unload ApfsDriver image from memory
    //
    gBS->UnloadImage (ImageHandle);
    return Status;
  }
  
  //
  // Connect loaded apfs.efi to controller from which we retrieve it
  //
  gBS->ConnectController(ControllerHandle, NULL, NULL, TRUE);
 
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ReadDisk (
  IN EFI_DISK_IO_PROTOCOL   *DiskIo,
  IN EFI_DISK_IO2_PROTOCOL  *DiskIo2,
  IN UINT32                 MediaId,
  IN UINT64                 Offset,
  IN UINTN                  BufferSize,
  OUT UINT8                 *Buffer
  )
{
  EFI_STATUS  Status;

  if (DiskIo2 != NULL) {
    Status = DiskIo2->ReadDiskEx (
      DiskIo2,
      MediaId,
      Offset,
      NULL,
      BufferSize,
      Buffer
      );
  } else {
    Status = DiskIo->ReadDisk (
      DiskIo,
      MediaId,
      Offset,
      BufferSize,
      Buffer
      );
  }

  return Status;
}

//
// Function to parse GPT entries in legacy 
//
EFI_STATUS
EFIAPI
LegacyApfsContainerScan (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle
  )
{
  EFI_STATUS                  Status;
  UINTN                       Index                        = 0;
  UINT8                       *Block                       = NULL;
  UINTN                       Lba                          = 0;
  UINT32                      PartitionNumber              = 0;
  UINT32                      PartitionEntrySize           = 0;
  EFI_PARTITION_TABLE_HEADER  *GptHeader                   = NULL;
  UINT32                      MediaId                      = 0;
  UINT32                      BlockSize                    = 0;
  EFI_BLOCK_IO_PROTOCOL       *BlockIo                     = NULL; 
  EFI_BLOCK_IO2_PROTOCOL      *BlockIo2                    = NULL;
  EFI_DISK_IO_PROTOCOL        *DiskIo                      = NULL;
  EFI_DISK_IO2_PROTOCOL       *DiskIo2                     = NULL;
  EFI_PARTITION_ENTRY         *ApfsGptEntry                = NULL;

  //
  // Open I/O protocols
  //
  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gEfiBlockIo2ProtocolGuid,
    (VOID **) &BlockIo2,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

  if (EFI_ERROR (Status)) {
    BlockIo2 = NULL;

    Status = gBS->OpenProtocol (
      ControllerHandle,
      &gEfiBlockIoProtocolGuid,
      (VOID **) &BlockIo,
      This->DriverBindingHandle,
      ControllerHandle,
      EFI_OPEN_PROTOCOL_GET_PROTOCOL
      );

    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  }  

  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gEfiDiskIo2ProtocolGuid,
    (VOID **) &DiskIo2,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

  if (EFI_ERROR (Status)) {
    DiskIo2 = NULL;
    Status = gBS->OpenProtocol (
      ControllerHandle,
      &gEfiDiskIoProtocolGuid,
      (VOID **) &DiskIo,
      This->DriverBindingHandle,
      ControllerHandle,
      EFI_OPEN_PROTOCOL_GET_PROTOCOL
      );

    if (EFI_ERROR (Status)){
      return EFI_UNSUPPORTED;
    }
  }

  if (BlockIo2 != NULL) {
    BlockSize     = BlockIo2->Media->BlockSize;
    MediaId       = BlockIo2->Media->MediaId;
  } else {
    BlockSize     = BlockIo->Media->BlockSize;
    MediaId       = BlockIo->Media->MediaId;
  }

  Block = AllocateZeroPool ((UINTN)BlockSize);
  if (Block == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Read GPT header first.
  //
  Status = ReadDisk (
    DiskIo,
    DiskIo2,
    MediaId,
    BlockSize,
    BlockSize,
    Block
    );

  if (EFI_ERROR (Status)) {
    FreePool (Block);
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
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    FreePool (Block);
    return EFI_UNSUPPORTED;
  }

 Status = ReadDisk (
    DiskIo,
    DiskIo2,
    MediaId,
    MultU64x32 (Lba, BlockSize),
    PartitionNumber * PartitionEntrySize,
    Block
    );

  if (EFI_ERROR (Status)) {
    FreePool (Block);
    return EFI_DEVICE_ERROR;
  }

  //
  // Analyze partition entries.
  //
  for (Index = 0; Index < PartitionEntrySize * PartitionNumber; Index += PartitionEntrySize) {
    EFI_PARTITION_ENTRY *CurrentEntry = (EFI_PARTITION_ENTRY *) (Block + Index);

    if (CompareMem(&CurrentEntry->PartitionTypeGUID, &gApfsContainerPartitionTypeGuid, sizeof (EFI_GUID)) == 0) {
      ApfsGptEntry = CurrentEntry;
      break;
    }

    if (CurrentEntry->StartingLBA == 0ull && CurrentEntry->EndingLBA == 0ull) {
      break;
    }
  }

  if (ApfsGptEntry == NULL)  {
    FreePool (Block);
    return EFI_UNSUPPORTED;
  }  
  LegacyBaseOffset = MultU64x32 (ApfsGptEntry->StartingLBA, BlockSize);
  FreePool(Block);

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

  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gAppleFileSystemEfiBootRecordInfoProtocolGuid,
    NULL,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
    );
   
   if (!EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
   }

  //
  // We check for both DiskIO and BlockIO protocols.
  // Both V1 and V2.
  //

  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gEfiDiskIo2ProtocolGuid,
    NULL,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
    );

  if (EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
      ControllerHandle,
      &gEfiDiskIoProtocolGuid,
      NULL,
      This->DriverBindingHandle,
      ControllerHandle,
      EFI_OPEN_PROTOCOL_TEST_PROTOCOL
      );
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gEfiBlockIo2ProtocolGuid,
    NULL,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
    );

  if (EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol(
      ControllerHandle,
      &gEfiBlockIoProtocolGuid,
      NULL,
      This->DriverBindingHandle,
      ControllerHandle,
      EFI_OPEN_PROTOCOL_TEST_PROTOCOL
      );
  }

  if (EFI_ERROR(Status)) {
    return Status;
  }

  if (LegacyScan) {
    return LegacyApfsContainerScan(This, ControllerHandle);
  }

  //
  // We check EfiPartitionInfoProtocol and ApplePartitionInfoProtocol
  //
  
  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gApfsContainerPartitionTypeGuid,
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
        if (CompareMem((EFI_GUID *)(ApplePartitionInfo->PartitionType), 
                       &gApfsContainerPartitionTypeGuid, sizeof (EFI_GUID)) != 0) {
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
      if (CompareMem(&Edk2PartitionInfo->Info.Gpt.PartitionTypeGUID, &gApfsContainerPartitionTypeGuid, sizeof (EFI_GUID)) != 0) {
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
  EFI_STATUS                                   Status;
  EFI_BLOCK_IO_PROTOCOL                        *BlockIo                     = NULL;
  EFI_BLOCK_IO2_PROTOCOL                       *BlockIo2                    = NULL;
  EFI_DISK_IO_PROTOCOL                         *DiskIo                      = NULL;
  EFI_DISK_IO2_PROTOCOL                        *DiskIo2                     = NULL;
  UINT32                                       BlockSize                    = 0;
  UINT32                                       ApfsBlockSize                = 0;
  UINT32                                       MediaId                      = 0;
  UINT8                                        *ApfsBlock                   = NULL;
  EFI_GUID                                     ContainerUuid;
  UINT64                                       EfiBootRecordBlockOffset     = 0;
  UINT64                                       EfiBootRecordBlockPtr        = 0;
  APFS_EFI_BOOT_RECORD                         *EfiBootRecordBlock          = NULL;
  APFS_NXSB                                    *ContainerSuperBlock         = NULL;
  UINT64                                       ApfsDriverBootRecordOffset   = 0;
  VOID                                         *AppleFileSystemDriverBuffer = NULL;
  UINTN                                        AppleFileSystemDriverSize    = 0;
  APPLE_FILESYSTEM_DRIVER_INFO_PRIVATE_DATA    *Private                     = NULL;
  APPLE_FILESYSTEM_EFIBOOTRECORD_LOCATION_INFO *EfiBootRecordLocationInfo   = NULL;

  
  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gAppleFileSystemEfiBootRecordInfoProtocolGuid,
    NULL,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
    );
   
   if (!EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
   }  

  DEBUG ((DEBUG_VERBOSE, "Apfs Container found.\n"));

  //
  // Open I/O protocols
  //
  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gEfiBlockIo2ProtocolGuid,
    (VOID **) &BlockIo2,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

  if (EFI_ERROR (Status)) {
    BlockIo2 = NULL;

    Status = gBS->OpenProtocol (
      ControllerHandle,
      &gEfiBlockIoProtocolGuid,
      (VOID **) &BlockIo,
      This->DriverBindingHandle,
      ControllerHandle,
      EFI_OPEN_PROTOCOL_GET_PROTOCOL
      );

    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  }  

  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gEfiDiskIo2ProtocolGuid,
    (VOID **) &DiskIo2,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

  if (EFI_ERROR (Status)) {
    DiskIo2 = NULL;
    Status = gBS->OpenProtocol (
      ControllerHandle,
      &gEfiDiskIoProtocolGuid,
      (VOID **) &DiskIo,
      This->DriverBindingHandle,
      ControllerHandle,
      EFI_OPEN_PROTOCOL_GET_PROTOCOL
      );

    if (EFI_ERROR (Status)){
      return EFI_UNSUPPORTED;
    }
  }

  if (BlockIo2 != NULL) {
    MediaId       = BlockIo2->Media->MediaId;
    BlockSize     = BlockIo2->Media->BlockSize;
  } else {
    MediaId       = BlockIo->Media->MediaId;
    BlockSize     = BlockIo->Media->BlockSize;
  }

  ApfsBlock = AllocateZeroPool (2048);
  if (ApfsBlock == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Read ContainerSuperblock and get ApfsBlockSize.
  //
  Status = ReadDisk (
    DiskIo,
    DiskIo2,
    MediaId,
    LegacyBaseOffset,
    2048,
    ApfsBlock
    );

  if (EFI_ERROR (Status)) {
    FreePool (ApfsBlock);
    return EFI_DEVICE_ERROR;
  }
 
  ContainerSuperBlock = (APFS_NXSB *)ApfsBlock;

  //
  // Verify NodeId and NodeType
  //
  if (ContainerSuperBlock->BlockHeader.NodeType != 0x80000001
      || ContainerSuperBlock->BlockHeader.NodeId != 1) {
    return EFI_UNSUPPORTED;
  }
  
  //
  // Verify ContainerSuperblock magic.
  //
  DEBUG ((DEBUG_VERBOSE, "CsbMagic: %04x\n", ContainerSuperBlock->MagicNumber));
  DEBUG ((DEBUG_VERBOSE, "Should be: %04x\n", CsbMagic));

  if (ContainerSuperBlock->MagicNumber != CsbMagic) {
    FreePool (ApfsBlock);
    return EFI_UNSUPPORTED;
  }

  //
  // Get ApfsBlockSize.
  //
  ApfsBlockSize = ContainerSuperBlock->BlockSize;

  DEBUG ((DEBUG_VERBOSE, "Container Blocksize: %u bytes\n", ApfsBlockSize));
  DEBUG ((DEBUG_VERBOSE, "ContainerSuperblock checksum: %08llx \n", ContainerSuperBlock->BlockHeader.Checksum));

  //
  // Take pointer to EfiBootRecordBlock.
  //
  EfiBootRecordBlockPtr = ContainerSuperBlock->EfiBootRecordBlock;

  DEBUG ((DEBUG_VERBOSE, "EfiBootRecord located at: %llu block\n", EfiBootRecordBlockPtr));

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
  Status = ReadDisk (
    DiskIo,
    DiskIo2,
    MediaId,
    LegacyBaseOffset,
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
  if (!ApfsBlockChecksumVerify((UINT8 *)ApfsBlock, ApfsBlockSize)) {
    FreePool (ApfsBlock);
    return EFI_UNSUPPORTED;
  } 

  //
  // Extract Container UUID
  //
  //ContainerUuid = AllocateZeroPool(sizeof(EFI_GUID));
  ContainerSuperBlock = (APFS_NXSB *)ApfsBlock;
  CopyMem(&ContainerUuid, &ContainerSuperBlock->Uuid, 16);

  //
  // Calculate Offset of EfiBootRecordBlock...
  //
  EfiBootRecordBlockOffset = MultU64x32 (EfiBootRecordBlockPtr, ApfsBlockSize) + LegacyBaseOffset; 
  
  DEBUG ((DEBUG_VERBOSE, "EfiBootRecordBlock offset: %08llx \n", EfiBootRecordBlockOffset));
  
  //
  // Read EfiBootRecordBlock.
  //
  Status = ReadDisk (
    DiskIo,
    DiskIo2,
    MediaId,
    EfiBootRecordBlockOffset,
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
  if (!ApfsBlockChecksumVerify(ApfsBlock, ApfsBlockSize)) {
    FreePool (ApfsBlock);
    return EFI_UNSUPPORTED;
  }
  
  EfiBootRecordBlock = (APFS_EFI_BOOT_RECORD *) ApfsBlock;
  if (EfiBootRecordBlock->MagicNumber != EfiBootRecordMagic) {
    FreePool(ApfsBlock);
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_VERBOSE, "EfiBootRecordBlock checksum: %08llx\n", EfiBootRecordBlock->BlockHeader.Checksum));
  DEBUG ((DEBUG_VERBOSE, "ApfsDriver located at: %llu block\n", EfiBootRecordBlock->BootRecordLBA));

  ApfsDriverBootRecordOffset = MultU64x32 (EfiBootRecordBlock->BootRecordLBA, ApfsBlockSize) + LegacyBaseOffset;
  AppleFileSystemDriverSize = MultU64x32 (EfiBootRecordBlock->BootRecordSize, ApfsBlockSize);

  DEBUG ((DEBUG_VERBOSE, "ApfsDriver offset: %08llx \n", ApfsDriverBootRecordOffset));
  DEBUG ((DEBUG_VERBOSE, "ApfsDriver size: %llu bytes\n", AppleFileSystemDriverSize));

  FreePool (ApfsBlock);

  AppleFileSystemDriverBuffer = AllocateZeroPool (AppleFileSystemDriverSize);

  if (AppleFileSystemDriverBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = ReadDisk (
    DiskIo,
    DiskIo2,
    MediaId,
    ApfsDriverBootRecordOffset,
    AppleFileSystemDriverSize,
    AppleFileSystemDriverBuffer
    );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  
  //
  // Fill public AppleFileSystemEfiBootRecordInfo protocol interface
  //
  Private = AllocatePool (sizeof(APPLE_FILESYSTEM_DRIVER_INFO_PRIVATE_DATA));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->ControllerHandle  = ControllerHandle;
  EfiBootRecordLocationInfo = &Private->EfiBootRecordLocationInfo;
  EfiBootRecordLocationInfo->ControllerHandle = ControllerHandle;
  CopyMem(&EfiBootRecordLocationInfo->ContainerUuid, &ContainerUuid, 16);

  Status = gBS->InstallMultipleProtocolInterfaces (
    &Private->ControllerHandle,
    &gAppleFileSystemEfiBootRecordInfoProtocolGuid,
    &Private->EfiBootRecordLocationInfo,
    NULL
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "AppleFileSystemEfiBootRecordInfoProtocol install failed with Status %r\n", Status));
    return Status;
  }

  Status = StartApfsDriver(
    ControllerHandle,
    AppleFileSystemDriverBuffer,
    AppleFileSystemDriverSize
    );

  if (EFI_ERROR (Status)) {
    gBS->UninstallProtocolInterface (
      ControllerHandle,
      &gAppleFileSystemEfiBootRecordInfoProtocolGuid,
      NULL
      );
    return EFI_UNSUPPORTED;
  }

  //
  // Free memory and close DiskIo protocol
  //
  if (AppleFileSystemDriverBuffer != NULL) { 
    FreePool (AppleFileSystemDriverBuffer);
  }
  if (Private != NULL) {
    FreePool(Private);
  }
  if (DiskIo2 != NULL) {
    gBS->CloseProtocol(
      ControllerHandle, 
      &gEfiDiskIo2ProtocolGuid, 
      This->DriverBindingHandle, 
      ControllerHandle
      );
  } else {
    gBS->CloseProtocol(
      ControllerHandle, 
      &gEfiDiskIoProtocolGuid, 
      This->DriverBindingHandle, 
      ControllerHandle
      );
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
  EFI_STATUS                                   Status;
  APPLE_FILESYSTEM_EFIBOOTRECORD_LOCATION_INFO *EfiBootRecordLocationInfo   = NULL;
  
  Status = gBS->OpenProtocol(
    ControllerHandle,
    &gAppleFileSystemEfiBootRecordInfoProtocolGuid,
    (VOID **) &EfiBootRecordLocationInfo,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

  if (EFI_ERROR (Status)) {
    Status = gBS->CloseProtocol(
      ControllerHandle,
      &gEfiDiskIoProtocolGuid,
      This->DriverBindingHandle,
      ControllerHandle
      );
    Status = gBS->CloseProtocol(
      ControllerHandle, 
      &gEfiDiskIo2ProtocolGuid, 
      This->DriverBindingHandle, 
      ControllerHandle
      );
  } else { 
    Status = gBS->UninstallMultipleProtocolInterfaces(
      EfiBootRecordLocationInfo->ControllerHandle,
      &gAppleFileSystemEfiBootRecordInfoProtocolGuid,
      EfiBootRecordLocationInfo
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
  VOID                                *PartitionInfoInterface = NULL;
  
  DEBUG ((DEBUG_VERBOSE, "Starting ApfdDriverLoader ver. %s\n", APFSDRIVERLOADER_VERSION));

  //
  // Check that PartitionInfo protocol present
  // If not present use Legacy scan
  //
  Status = gBS->LocateProtocol(
    &gApfsContainerPartitionTypeGuid,
    NULL,
    (VOID **) &PartitionInfoInterface
    );
  if (Status == EFI_NOT_FOUND) {
    Status = gBS->LocateProtocol(
      &gEfiPartitionInfoProtocolGuid,
      NULL,
      (VOID **) &PartitionInfoInterface
      );
    if (Status == EFI_NOT_FOUND) {
      Status = gBS->LocateProtocol(
        &gApplePartitionInfoProtocolGuid,
        NULL,
        (VOID **) &PartitionInfoInterface
      );
    }
  }

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_VERBOSE, "No partition info protocol, using Legacy scan\n"));
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
