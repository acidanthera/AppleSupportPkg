/** @file

APFS Driver Loader - loads apfs.efi from JSDR section in container

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
#include "ApfsDriverLoaderVersion.h"

STATIC BOOLEAN      mFoundAppleFileSystemDriver;
STATIC EFI_EVENT    mLoadAppleFileSystemEvent;
STATIC VOID         *mAppleFileSystemDriverBuffer;
STATIC UINTN        mAppleFileSystemDriverSize;


STATIC
EFI_STATUS
EFIAPI 
NullTextReset (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN BOOLEAN                         ExtendedVerification
  )
{
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
NullTextOutputString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN CHAR16                          *String
  )
{
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
NullTextTestString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN CHAR16                          *String
  )
{
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
NullTextQueryMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN  UINTN                           ModeNumber,
  OUT UINTN                           *Columns,
  OUT UINTN                           *Rows
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
NullTextSetMode (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN UINTN                           ModeNumber
  )
{
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
NullTextSetAttribute (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN UINTN                           Attribute
  )
{
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
NullTextClearScreen (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
  )
{
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
NullTextSetCursorPosition (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN UINTN                           Column,
  IN UINTN                           Row
  )
{
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
NullTextEnableCursor (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN BOOLEAN                         Visible
  )
{
  return EFI_SUCCESS;
}

STATIC
EFI_SIMPLE_TEXT_OUTPUT_MODE
mNullTextOutputMode;

STATIC
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
mNullTextOutputProtocol = {
  NullTextReset,
  NullTextOutputString,
  NullTextTestString,
  NullTextQueryMode,
  NullTextSetMode,
  NullTextSetAttribute,
  NullTextClearScreen,
  NullTextSetCursorPosition,
  NullTextEnableCursor,
  &mNullTextOutputMode
};


/*
 * Function to calculate APFS block checksum.
 * According to the apple docs the Fletcher’s checksum algorithm is used.
 * Apple uses a variant of the algorithm described in a paper by John Kodis.
 * The following algorithm shows this procedure.
 * The input is the block without the first 8 byte.
 */
STATIC
UINT64
APFS_CreateChecksum (
  UINT32  *Data,
  UINTN  DataSize
  )
{
  UINTN         Index;
  UINT64        Sum1 = 0;
  UINT64        Check1 = 0;
  UINT64        Sum2 = 0;
  UINT64        Check2 = 0;
  CONST UINT64  ModValue = 0xFFFFFFFF;
  for (Index = 0; Index < DataSize / sizeof (UINT32); Index++) {
    Sum1 = ((Sum1 + (UINT64)Data[Index]) % ModValue);
    Sum2 = (Sum2 + Sum1) % ModValue;
  }

  Check1 = ModValue - ((Sum1 + Sum2) % ModValue);
  Check2 = ModValue - ((Sum1 + Check1) % ModValue);

  return (Check2 << 32) | Check1;
}

/*
 * Function to check block checksum.
 * Returns TRUE if the checksum is valid.
 */
STATIC
BOOLEAN
APFS_CheckChecksum (
  UINT8   *Data,
  UINTN  DataSize
  )
{
  UINT64  NewChecksum;
  UINT64  *CurrChecksum = (UINT64 *)Data;

  NewChecksum = APFS_CreateChecksum (
    (UINT32 *)(Data + sizeof (UINT64)),
    DataSize - sizeof (UINT64)
    );

  return NewChecksum == *CurrChecksum;
}

STATIC
EFI_STATUS
ReadDisk (
  IN EFI_DISK_IO_PROTOCOL   *DiskIo,
  IN EFI_DISK_IO2_PROTOCOL  *DiskIo2,
  IN UINT32                 MediaId,
  IN UINT64                 Offset,
  IN UINTN                  BufferSize,
  OUT VOID                  *Buffer
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

/**
 * Driver Binding EFI protocol, Supported function. This function is called by EFI
 * to test if this driver can handle a certain device. Our implementation only checks
 * if the device is a disk (i.e. that it supports the Block I/O and Disk I/O protocols)
 * and implicitly checks if the disk is already in use by another driver.
 * (taken from vbox fs drivers "fsw")
 */

EFI_STATUS
EFIAPI
ApfsDriverLoaderSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS Status;

  if (mFoundAppleFileSystemDriver) {
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

  return Status;
}

/**
 * Driver Binding EFI protocol, Start function. This function is called by EFI
 * to start driving the given device. It is still possible at this point to
 * return EFI_UNSUPPORTED, and in fact we will do so if the file system driver
 * cannot find the superblock signature (or equivalent) that it expects.
 *
 * This function allocates memory for a per-volume structure, opens the
 * required protocols (just Disk I/O in our case, Block I/O is only looked
 * at to get the MediaId field), and lets the FSW core mount the file system.
 * If successful, an EFI Simple File System protocol is exported on the
 * device handle.
 */

EFI_STATUS
EFIAPI
ApfsDriverLoaderStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                  Status;
  EFI_BLOCK_IO_PROTOCOL       *BlockIo                     = NULL;
  EFI_BLOCK_IO2_PROTOCOL      *BlockIo2                    = NULL;
  EFI_DISK_IO_PROTOCOL        *DiskIo                      = NULL;
  EFI_DISK_IO2_PROTOCOL       *DiskIo2                     = NULL;
  UINT32                      BlockSize                    = 0;
  UINT32                      ApfsBlockSize                = 0;
  UINT32                      MediaId                      = 0;
  EFI_LBA                     LastBlock                    = 0;
  EFI_PARTITION_TABLE_HEADER  *GptHeader                   = NULL;
  UINT32                      PartitionNumber              = 0;
  UINT32                      PartitionEntrySize           = 0;
  EFI_BLOCK_IO_MEDIA          *Media                       = NULL;
  UINT32                      Lba                          = 0;
  VOID                        *Block                       = NULL;
  EFI_PARTITION_ENTRY         *ApfsGptEntry                = NULL;
  VOID                        *ApfsBlock                   = NULL;
  UINT64                      JsdrOffset                   = 0;
  UINT64                      JsdrPtr                      = 0;
  APFS_EFI_BOOT_RECORD        *JsdrBlock                   = NULL;
  APFS_NXSB                   *NextSuperBlock              = NULL;
  UINT64                      ApfsDriverBootReccordOffset  = 0;
  UINTN                       Index                        = 0;

  if (mFoundAppleFileSystemDriver) {
    return EFI_UNSUPPORTED;
  }

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
    Media         = BlockIo2->Media;
    BlockSize     = BlockIo2->Media->BlockSize;
    LastBlock     = BlockIo2->Media->LastBlock;
    MediaId       = BlockIo2->Media->MediaId;
  } else {
    Media         = BlockIo->Media;
    BlockSize     = BlockIo->Media->BlockSize;
    LastBlock     = BlockIo->Media->LastBlock;
    MediaId       = BlockIo->Media->MediaId;
  }

  //
  // Skip strange blockio devices on VmWare
  //
  if (LastBlock == 0) {
    return EFI_UNSUPPORTED;
  }

  Block = AllocateZeroPool ((UINTN)BlockSize);
  if (Block == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Read GPT header first.
  //
  Lba = 1;
  Status = ReadDisk (
    DiskIo,
    DiskIo2,
    MediaId,
    MultU64x32 (Lba, BlockSize),
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

    if (CompareMem(&CurrentEntry->PartitionTypeGUID, &mApfsContainerGuid, sizeof (EFI_GUID)) == 0) {
      ApfsGptEntry = CurrentEntry;
      break;
    }

    //
    // Speedhack. Remove?
    //
    if (CurrentEntry->StartingLBA == 0ull && CurrentEntry->EndingLBA == 0ull) {
      break;
    }
  }

  if (ApfsGptEntry == NULL)  {
    FreePool (Block);
    return EFI_UNSUPPORTED;
  }

  ApfsBlock = AllocateZeroPool (2048);
  if (ApfsBlock == NULL) {
    FreePool (Block);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Read NXSB and get ApfsBlockSize.
  //
  Status = ReadDisk (
    DiskIo,
    DiskIo2,
    MediaId,
    MultU64x32 (ApfsGptEntry->StartingLBA, BlockSize),
    2048,
    ApfsBlock
    );

  if (EFI_ERROR (Status)) {
    FreePool (Block);
    FreePool (ApfsBlock);
    return EFI_DEVICE_ERROR;
  }
  
  //
  // Get NXSB header and verify magic number.
  //
  NextSuperBlock = (APFS_NXSB *)ApfsBlock;
  if (NextSuperBlock ->MagicNumber != NXSB_MN) {
    FreePool (Block);
    FreePool (ApfsBlock);
    return EFI_UNSUPPORTED;
  }

  //
  // Get ApfsBlockSize.
  //
  ApfsBlockSize = NextSuperBlock->BlockSize;

  //
  // Free ApfsBlock and allocate one of a correct size.
  //
  FreePool (ApfsBlock);
  ApfsBlock = AllocateZeroPool (ApfsBlockSize);
  if (ApfsBlock == NULL) {
    FreePool (Block);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Read full NXSB with known BlockSize.
  //

  Status = ReadDisk (
    DiskIo,
    DiskIo2,
    MediaId,
    MultU64x32 (ApfsGptEntry->StartingLBA, BlockSize),
    ApfsBlockSize,
    ApfsBlock
    );
  
  if (EFI_ERROR (Status)) {
    FreePool (Block);
    FreePool (ApfsBlock);
    return EFI_DEVICE_ERROR;
  }
  
  //
  // Verify NXSB checksum.
  //
  if (!APFS_CheckChecksum((UINT8 *)ApfsBlock, ApfsBlockSize)) {
    FreePool (Block);
    FreePool (ApfsBlock);
    return EFI_UNSUPPORTED;
  } 

  //
  // Take pointer to JSDR.
  //
  JsdrPtr = *(UINT64 *) (ApfsBlock + 0x4F8);

  //
  // Calculate Offset...
  //
  JsdrOffset = MultU64x32 (JsdrPtr, ApfsBlockSize) + MultU64x32 (ApfsGptEntry->StartingLBA, BlockSize);

  //
  // Read JSDR block.
  //

  Status = ReadDisk (
    DiskIo,
    DiskIo2,
    MediaId,
    JsdrOffset,
    ApfsBlockSize,
    ApfsBlock
    );

  if (EFI_ERROR (Status)) {
    FreePool (Block);
    FreePool (ApfsBlock);
    return EFI_DEVICE_ERROR;
  }

  //
  // Verify JSDR checksum.
  //
  if (!APFS_CheckChecksum(ApfsBlock, ApfsBlockSize)) {
    FreePool (Block);
    FreePool (ApfsBlock);
    return EFI_UNSUPPORTED;
  }

  
  JsdrBlock = (APFS_EFI_BOOT_RECORD *) ApfsBlock;
  if (JsdrBlock ->MagicNumber != JSDR_MN) {
    FreePool(Block);
    FreePool(ApfsBlock);
    return EFI_UNSUPPORTED;
  }

  ApfsDriverBootReccordOffset = 
    MultU64x32 (JsdrBlock->BootRecordLBA, ApfsBlockSize) + 
    MultU64x32 (ApfsGptEntry->StartingLBA, BlockSize);
  mAppleFileSystemDriverSize = MultU64x32 (JsdrBlock->BootRecordSize, ApfsBlockSize);

  FreePool (Block);
  FreePool (ApfsBlock);

  mAppleFileSystemDriverBuffer = AllocateZeroPool (mAppleFileSystemDriverSize);

  if (mAppleFileSystemDriverBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = ReadDisk (
    DiskIo,
    DiskIo2,
    MediaId,
    ApfsDriverBootReccordOffset,
    mAppleFileSystemDriverSize,
    mAppleFileSystemDriverBuffer
    );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  mFoundAppleFileSystemDriver = TRUE;
  Status = gBS->SignalEvent (mLoadAppleFileSystemEvent);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
ApfsDriverLoaderStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS EFIAPI
NullConOutOutputString(IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, IN CHAR16 *String) {
  return EFI_SUCCESS;
}


VOID
EFIAPI
LoadAppleFileSystemDriverNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS   Status;
  EFI_HANDLE   ImageHandle = NULL;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedApfsDrvImage = NULL;
  EFI_SYSTEM_TABLE           *NewSystemTable;

  if (mAppleFileSystemDriverBuffer == NULL) {
    DEBUG ((DEBUG_WARN, "Second attempt to load apfs.efi, aborting...\n"));
    return;
  }

  DEBUG ((DEBUG_VERBOSE, "Loading apfs.efi from memory!\n"));

  Status = gBS->LoadImage (
    FALSE,
    gImageHandle,
    NULL,
    mAppleFileSystemDriverBuffer, 
    mAppleFileSystemDriverSize,
    &ImageHandle
    );

  FreePool (mAppleFileSystemDriverBuffer);
  mAppleFileSystemDriverBuffer = NULL;
  Status = gBS->HandleProtocol (
    ImageHandle,
    &gEfiLoadedImageProtocolGuid,
    (VOID *) &LoadedApfsDrvImage
    ); 

  //
  // Patch verbose
  //
  NewSystemTable = (EFI_SYSTEM_TABLE *) AllocateZeroPool (gST->Hdr.HeaderSize);
  if (NewSystemTable != NULL) {
    CopyMem ((VOID *) NewSystemTable, gST, gST->Hdr.HeaderSize);
    NewSystemTable->ConOut = &mNullTextOutputProtocol;
    NewSystemTable->Hdr.CRC32 = 0;
    gBS->CalculateCrc32 (NewSystemTable, NewSystemTable->Hdr.HeaderSize, &NewSystemTable->Hdr.CRC32);
    LoadedApfsDrvImage->SystemTable = NewSystemTable;
  }

  gBS->StartImage (ImageHandle, NULL, NULL);
}

/**
 * Interface structure for the EFI Driver Binding protocol.
 * According to UEFI Spec 2.6 , we should define Supported,Start,Stop function for
 * DriverBinding
 */
STATIC
EFI_DRIVER_BINDING_PROTOCOL
mApfsDriverLoaderBinding = {
  ApfsDriverLoaderSupported,
  ApfsDriverLoaderStart,
  ApfsDriverLoaderStop,
  0xf,
  NULL,
  NULL,
};

/**
 * Image entry point. Installs the Driver Binding and Component Name protocols
 * on the image's handle.
 */
EFI_STATUS
EFIAPI
ApfsDriverLoaderInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
)
{
  EFI_STATUS  Status;
  
  Status = gBS->CreateEvent (
    EVT_NOTIFY_SIGNAL,
    TPL_CALLBACK,
    LoadAppleFileSystemDriverNotify,
    NULL,
    &mLoadAppleFileSystemEvent
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EfiLibInstallDriverBindingComponentName2 (
    ImageHandle,
    SystemTable,
    &mApfsDriverLoaderBinding,
    ImageHandle,
    NULL,
    NULL
    );

  return Status;
}
