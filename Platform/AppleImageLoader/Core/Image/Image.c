/** @file
  Core image handling services to load and unload PeImage.
  
Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Base.h>
#include <Uefi.h>
#include <PiDxe.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PeCoffLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PeCoffExtraActionLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/LoadPe32Image.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/Ebc.h>
#include <Protocol/Runtime.h>
#include <Protocol/HiiPackageList.h>
#include <Protocol/TcgService.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/FirmwareFileSystem3.h>
#include "ImagePrivate.h"

#include "CoreMain.h"

//
// Disable performance lib
//
#define PERF_LOAD_IMAGE_BEGIN(ModuleHandle) do { } while (0)
#define PERF_LOAD_IMAGE_END(ModuleHandle) do { } while (0)
#define PERF_START_IMAGE_BEGIN(ModuleHandle) do { } while (0)
#define PERF_START_IMAGE_END(ModuleHandle) do { } while (0)

//
// Module Globals
//
LOADED_IMAGE_PRIVATE_DATA  *mCurrentImage = NULL;

//
// Map CoreDxe function into platform ones
//
STATIC
EFI_STATUS
CoreLocateHandleBuffer (
  IN  EFI_LOCATE_SEARCH_TYPE   SearchType,
    IN  EFI_GUID                 *Protocol OPTIONAL,
    IN  VOID                     *SearchKey OPTIONAL,
    IN  OUT UINTN                *NoHandles,
    OUT EFI_HANDLE               **Buffer
    ) 
{
  return gBS->LocateHandleBuffer (
               SearchType,
               Protocol,
               SearchKey,
               NoHandles,
               Buffer
               );
}

STATIC
EFI_STATUS
CoreProtocolsPerHandle (
    IN EFI_HANDLE               Handle,
    OUT EFI_GUID                ***ProtocolBuffer,
    OUT UINTN                   *ProtocolBufferCount
    )
{
  return gBS->ProtocolsPerHandle (
    Handle,
    ProtocolBuffer,
    ProtocolBufferCount
    );
}

STATIC
EFI_STATUS
CoreOpenProtocolInformation (
  IN EFI_HANDLE               Handle,
  IN EFI_GUID                 *Protocol,
  OUT EFI_OPEN_PROTOCOL_INFORMATION_ENTRY **EntryBuffer,
  OUT UINTN                   *EntryCount
  )
{
  return gBS->OpenProtocolInformation (
    Handle,
    Protocol,
    EntryBuffer,
    EntryCount
    );
}

STATIC
EFI_STATUS
CoreCloseProtocol (
  IN EFI_HANDLE               Handle,
  IN EFI_GUID                 *Protocol,
  IN EFI_HANDLE               AgentHandle,
  IN EFI_HANDLE               ControllerHandle  
  )
{
  return gBS->CloseProtocol (
    Handle,
    Protocol,
    AgentHandle,
    ControllerHandle
    );
}

STATIC
EFI_STATUS
CoreUninstallProtocolInterface (
  IN EFI_HANDLE               Handle,
  IN EFI_GUID                 *Protocol,
  IN VOID                     *Interface
  )
{
  return gBS->UninstallProtocolInterface (
    Handle,
    Protocol,
    Interface
    );
}

STATIC
VOID
CoreFreePool (
  IN VOID                         *Buffer
  ) 
{
  return FreePool (Buffer);
}

STATIC
EFI_STATUS
CoreFreePages (
  IN EFI_PHYSICAL_ADDRESS         Memory,
  IN UINTN                        NoPages
  )
{
  return gBS->FreePages (Memory, NoPages);
}

STATIC
EFI_STATUS
CoreAllocatePages (
  IN EFI_ALLOCATE_TYPE            Type,
  IN EFI_MEMORY_TYPE              MemoryType,
  IN UINTN                        NoPages,
  OUT EFI_PHYSICAL_ADDRESS        *Memory
  )
{
  return gBS->AllocatePages (
    Type,
    MemoryType,
    NoPages,
    Memory
    );
}

STATIC
EFI_STATUS
CoreLocateDevicePath (
  IN EFI_GUID                 *Protocol,
  IN OUT EFI_DEVICE_PATH      **DevicePath,
  OUT EFI_HANDLE              *Device
  )
{
  return gBS->LocateDevicePath (
    Protocol,
    DevicePath,
    Device
    );
}

STATIC
EFI_STATUS
CoreHandleProtocol (
  IN EFI_HANDLE               Handle,
  IN EFI_GUID                 *Protocol,
  OUT VOID                    **Interface
  )
{
  return gBS->HandleProtocol (
    Handle,
    Protocol,
    Interface
    );
}

STATIC
EFI_STATUS
CoreLocateProtocol (
  IN EFI_GUID                 *Protocol,
  IN VOID                     *Registration OPTIONAL,
  OUT VOID                    **Interface
  )
{
  return gBS->LocateProtocol (
    Protocol,
    Registration,
    Interface
    );
}

STATIC
EFI_STATUS
CoreInstallProtocolInterface (
  IN OUT EFI_HANDLE           *Handle,
  IN EFI_GUID                 *Protocol,
  IN EFI_INTERFACE_TYPE       InterfaceType,
  IN VOID                     *Interface
  )
{
  return gBS->InstallProtocolInterface (
    Handle,
    Protocol,
    InterfaceType,
    Interface
    );
}

STATIC
EFI_STATUS
CoreExit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_STATUS                   ExitStatus,
  IN UINTN                        ExitDataSize,
  IN CHAR16                       *ExitData OPTIONAL
  )
{
  return gBS->Exit (
    ImageHandle,
    ExitStatus,
    ExitDataSize,
    ExitData
    );
}

STATIC
VOID
CoreRestoreTpl (
  IN EFI_TPL      OldTpl
  )
{
  return gBS->RestoreTPL (OldTpl);
}

STATIC
EFI_TPL
CoreRaiseTPL (
  IN EFI_TPL      NewTpl
  )
{
  return gBS->RaiseTPL (NewTpl);
}

typedef struct {
  UINT16  MachineType;
  CHAR16  *MachineTypeName;
} MACHINE_TYPE_INFO;

GLOBAL_REMOVE_IF_UNREFERENCED MACHINE_TYPE_INFO  mMachineTypeInfo[] = {
  {EFI_IMAGE_MACHINE_IA32,           L"IA32"},
  {EFI_IMAGE_MACHINE_IA64,           L"IA64"},
  {EFI_IMAGE_MACHINE_X64,            L"X64"},
  {EFI_IMAGE_MACHINE_ARMTHUMB_MIXED, L"ARM"},
  {EFI_IMAGE_MACHINE_AARCH64,        L"AARCH64"}
};

UINT16 mDxeCoreImageMachineType = 0;

/**
 Return machine type name.
 @param MachineType The machine type
 @return machine type name
**/
CHAR16 *
GetMachineTypeName (
  UINT16 MachineType
  )
{
  UINTN  Index;

  for (Index = 0; Index < sizeof(mMachineTypeInfo)/sizeof(mMachineTypeInfo[0]); Index++) {
    if (mMachineTypeInfo[Index].MachineType == MachineType) {
      return mMachineTypeInfo[Index].MachineTypeName;
    }
  }

  return L"<Unknown>";
}

VOID *
EFIAPI
InvalidateInstructionCacheRange (
  IN      VOID                      *Address,
  IN      UINTN                     Length
  )
{
  if (Length == 0) {
    return Address;
  }

  ASSERT ((Length - 1) <= (MAX_ADDRESS - (UINTN)Address));
  return Address;
}

/**
  Read image file (specified by UserHandle) into user specified buffer with specified offset
  and length.
  @param  UserHandle             Image file handle
  @param  Offset                 Offset to the source file
  @param  ReadSize               For input, pointer of size to read; For output,
                                 pointer of size actually read.
  @param  Buffer                 Buffer to write into
  @retval EFI_SUCCESS            Successfully read the specified part of file
                                 into buffer.
**/
EFI_STATUS
EFIAPI
CoreReadImageFile (
  IN     VOID    *UserHandle,
  IN     UINTN   Offset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  )
{
  UINTN               EndPosition;
  IMAGE_FILE_HANDLE  *FHand;

  if (UserHandle == NULL || ReadSize == NULL || Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (MAX_ADDRESS - Offset < *ReadSize) {
    return EFI_INVALID_PARAMETER;
  }

  FHand = (IMAGE_FILE_HANDLE  *)UserHandle;
  ASSERT (FHand->Signature == IMAGE_FILE_HANDLE_SIGNATURE);

  //
  // Move data from our local copy of the file
  //
  EndPosition = Offset + *ReadSize;
  if (EndPosition > FHand->SourceSize) {
    *ReadSize = (UINT32)(FHand->SourceSize - Offset);
  }
  if (Offset >= FHand->SourceSize) {
      *ReadSize = 0;
  }

  CopyMem (Buffer, (CHAR8 *)FHand->Source + Offset, *ReadSize);
  return EFI_SUCCESS;
}

VOID
CoreUnloadAndCloseImage (
  IN LOADED_IMAGE_PRIVATE_DATA  *Image,
  IN BOOLEAN                    FreePage
  )
{
  EFI_STATUS                          Status;
  UINTN                               HandleCount;
  EFI_HANDLE                          *HandleBuffer;
  UINTN                               HandleIndex;
  EFI_GUID                            **ProtocolGuidArray;
  UINTN                               ArrayCount;
  UINTN                               ProtocolIndex;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *OpenInfo;
  UINTN                               OpenInfoCount;
  UINTN                               OpenInfoIndex;

  HandleBuffer = NULL;
  ProtocolGuidArray = NULL;

  if (Image->Started) {
    UnregisterMemoryProfileImage (Image);
  } 

  UnprotectUefiImage (&Image->Info, Image->LoadedImageDevicePath);

  //
  // Unload image, free Image->ImageContext->ModHandle
  //
  PeCoffLoaderUnloadImage (&Image->ImageContext);

  //
  // Free our references to the image handle
  //
  if (Image->Handle != NULL) {

    Status = CoreLocateHandleBuffer (
               AllHandles,
               NULL,
               NULL,
               &HandleCount,
               &HandleBuffer
               );
    if (!EFI_ERROR (Status)) {
      for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
        Status = CoreProtocolsPerHandle (
                   HandleBuffer[HandleIndex],
                   &ProtocolGuidArray,
                   &ArrayCount
                   );
        if (!EFI_ERROR (Status)) {
          for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++) {
            Status = CoreOpenProtocolInformation (
                       HandleBuffer[HandleIndex],
                       ProtocolGuidArray[ProtocolIndex],
                       &OpenInfo,
                       &OpenInfoCount
                       );
            if (!EFI_ERROR (Status)) {
              for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
                if (OpenInfo[OpenInfoIndex].AgentHandle == Image->Handle) {
                  Status = CoreCloseProtocol (
                             HandleBuffer[HandleIndex],
                             ProtocolGuidArray[ProtocolIndex],
                             Image->Handle,
                             OpenInfo[OpenInfoIndex].ControllerHandle
                             );
                }
              }
              if (OpenInfo != NULL) {
                FreePool(OpenInfo);
              }
            }
          }
          if (ProtocolGuidArray != NULL) {
            FreePool(ProtocolGuidArray);
          }
        }
      }
      if (HandleBuffer != NULL) {
        FreePool (HandleBuffer);
      }
    }

    //
    // Here we remove debug image info entry
    //

    Status = CoreUninstallProtocolInterface (
               Image->Handle,
               &gEfiLoadedImageDevicePathProtocolGuid,
               Image->LoadedImageDevicePath
               );

    Status = CoreUninstallProtocolInterface (
               Image->Handle,
               &gEfiLoadedImageProtocolGuid,
               &Image->Info
               );

    if (Image->ImageContext.HiiResourceData != 0) {
      Status = CoreUninstallProtocolInterface (
                 Image->Handle,
                 &gEfiHiiPackageListProtocolGuid,
                 (VOID *) (UINTN) Image->ImageContext.HiiResourceData
                 );
    }

  }

  if (Image->RuntimeData != NULL) {
    if (Image->RuntimeData->Link.ForwardLink != NULL) {
      //
      // Remove the Image from the Runtime Image list as we are about to Free it!
      //
      RemoveEntryList (&Image->RuntimeData->Link);
      
      //
      // CHECKME: ImageRecord used on UEFI 2.5 specification
      // 
      //RemoveImageRecord (Image->RuntimeData);
    }
    CoreFreePool (Image->RuntimeData);
  }

  //
  // Free the Image from memory
  //
  if ((Image->ImageBasePage != 0) && FreePage) {
    CoreFreePages (Image->ImageBasePage, Image->NumberOfPages);
  }

  //
  // Done with the Image structure
  //
  if (Image->Info.FilePath != NULL) {
    CoreFreePool (Image->Info.FilePath);
  }

  if (Image->LoadedImageDevicePath != NULL) {
    CoreFreePool (Image->LoadedImageDevicePath);
  }

  if (Image->FixupData != NULL) {
    CoreFreePool (Image->FixupData);
  }

  CoreFreePool (Image);
}

/**
  Get the image's private data from its handle.
  @param  ImageHandle             The image handle
  @return Return the image private data associated with ImageHandle.
**/
LOADED_IMAGE_PRIVATE_DATA *
CoreLoadedImageInfo (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                 Status;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  LOADED_IMAGE_PRIVATE_DATA  *Image;

  Status = CoreHandleProtocol (
             ImageHandle,
             &gEfiLoadedImageProtocolGuid,
             (VOID **)&LoadedImage
             );
  if (!EFI_ERROR (Status)) {
    Image = LOADED_IMAGE_PRIVATE_DATA_FROM_THIS (LoadedImage);
  } else {
    DEBUG ((DEBUG_LOAD, "CoreLoadedImageInfo: Not an ImageHandle %p\n", ImageHandle));
    Image = NULL;
  }

  return Image;
}

EFI_STATUS
CoreLoadPeImage (
  IN BOOLEAN                     BootPolicy,
  IN VOID                        *Pe32Handle,
  IN LOADED_IMAGE_PRIVATE_DATA   *Image,
  IN EFI_PHYSICAL_ADDRESS        DstBuffer    OPTIONAL,
  OUT EFI_PHYSICAL_ADDRESS       *EntryPoint  OPTIONAL,
  IN  UINT32                     Attribute
  )
{
  EFI_STATUS                Status;
  BOOLEAN                   DstBufAlocated;
  UINTN                     Size;
  EFI_RUNTIME_ARCH_PROTOCOL *gRuntime = NULL;

  //
  // CHECKME: 
  // Old platforms doesn't have gRuntime
  //
  Status = CoreLocateProtocol (
    &gEfiRuntimeArchProtocolGuid,
    NULL,
    (VOID **) &gRuntime
  );

  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_WARN, "Runtime protocol not found"));
    return EFI_UNSUPPORTED;
  }

  ZeroMem (&Image->ImageContext, sizeof (Image->ImageContext));

  Image->ImageContext.Handle    = Pe32Handle;
  Image->ImageContext.ImageRead = (PE_COFF_LOADER_READ_FILE) CoreReadImageFile;

  //
  // Get information about the image being loaded
  //
  Status = PeCoffLoaderGetImageInfo (&Image->ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!EFI_IMAGE_MACHINE_TYPE_SUPPORTED (Image->ImageContext.Machine)) {
    if (!EFI_IMAGE_MACHINE_CROSS_TYPE_SUPPORTED (Image->ImageContext.Machine)) {
      //
      // The PE/COFF loader can support loading image types that can be executed.
      // If we loaded an image type that we can not execute return EFI_UNSUPORTED.
      //
      DEBUG ((EFI_D_ERROR, "Image type %s can't be loaded ", GetMachineTypeName(Image->ImageContext.Machine)));
      DEBUG ((EFI_D_ERROR, "on %s UEFI system.\n", GetMachineTypeName(mDxeCoreImageMachineType)));
      return EFI_UNSUPPORTED;
    }
  }

  //
  // Set EFI memory type based on ImageType
  //
  switch (Image->ImageContext.ImageType) {
  case EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION:
    Image->ImageContext.ImageCodeMemoryType = EfiLoaderCode;
    Image->ImageContext.ImageDataMemoryType = EfiLoaderData;
    break;
  case EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
    Image->ImageContext.ImageCodeMemoryType = EfiBootServicesCode;
    Image->ImageContext.ImageDataMemoryType = EfiBootServicesData;
    break;
  case EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
  case EFI_IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER:
    Image->ImageContext.ImageCodeMemoryType = EfiRuntimeServicesCode;
    Image->ImageContext.ImageDataMemoryType = EfiRuntimeServicesData;
    break;
  default:
    Image->ImageContext.ImageError = IMAGE_ERROR_INVALID_SUBSYSTEM;
    return EFI_UNSUPPORTED;
  }

  //
  // Allocate memory of the correct memory type aligned on the required image boundary
  //
  DstBufAlocated = FALSE;
  if (DstBuffer == 0) {
    //
    // Allocate Destination Buffer as caller did not pass it in
    //

    if (Image->ImageContext.SectionAlignment > EFI_PAGE_SIZE) {
      Size = (UINTN)Image->ImageContext.ImageSize + Image->ImageContext.SectionAlignment;
    } else {
      Size = (UINTN)Image->ImageContext.ImageSize;
    }

    Image->NumberOfPages = EFI_SIZE_TO_PAGES (Size);

    //
    // If the image relocations have not been stripped, then load at any address.
    // Otherwise load at the address at which it was linked.
    //
    // Memory below 1MB should be treated reserved for CSM and there should be
    // no modules whose preferred load addresses are below 1MB.
    //
    Status = EFI_OUT_OF_RESOURCES;

    if (Image->ImageContext.ImageAddress >= 0x100000 || Image->ImageContext.RelocationsStripped) {
      Status = CoreAllocatePages (
                 AllocateAddress,
                 (EFI_MEMORY_TYPE) (Image->ImageContext.ImageCodeMemoryType),
                 Image->NumberOfPages,
                 &Image->ImageContext.ImageAddress
                 );
    }
    if (EFI_ERROR (Status) && !Image->ImageContext.RelocationsStripped) {
      Status = CoreAllocatePages (
                 AllocateAnyPages,
                 (EFI_MEMORY_TYPE) (Image->ImageContext.ImageCodeMemoryType),
                 Image->NumberOfPages,
                 &Image->ImageContext.ImageAddress
                 );
    }
    
    if (EFI_ERROR (Status)) {
      return Status;
    }
    DstBufAlocated = TRUE;
  } else {
    //
    // Caller provided the destination buffer
    //

    if (Image->ImageContext.RelocationsStripped && (Image->ImageContext.ImageAddress != DstBuffer)) {
      //
      // If the image relocations were stripped, and the caller provided a
      // destination buffer address that does not match the address that the
      // image is linked at, then the image cannot be loaded.
      //
      return EFI_INVALID_PARAMETER;
    }

    if (Image->NumberOfPages != 0 &&
        Image->NumberOfPages <
        (EFI_SIZE_TO_PAGES ((UINTN)Image->ImageContext.ImageSize + Image->ImageContext.SectionAlignment))) {
      Image->NumberOfPages = EFI_SIZE_TO_PAGES ((UINTN)Image->ImageContext.ImageSize + Image->ImageContext.SectionAlignment);
      return EFI_BUFFER_TOO_SMALL;
    }

    Image->NumberOfPages = EFI_SIZE_TO_PAGES ((UINTN)Image->ImageContext.ImageSize + Image->ImageContext.SectionAlignment);
    Image->ImageContext.ImageAddress = DstBuffer;
  }

  Image->ImageBasePage = Image->ImageContext.ImageAddress;
  if (!Image->ImageContext.IsTeImage) {
    Image->ImageContext.ImageAddress =
        (Image->ImageContext.ImageAddress + Image->ImageContext.SectionAlignment - 1) &
        ~((UINTN)Image->ImageContext.SectionAlignment - 1);
  }

  //
  // Load the image from the file into the allocated memory
  //
  Status = PeCoffLoaderLoadImage (&Image->ImageContext);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // If this is a Runtime Driver, then allocate memory for the FixupData that
  // is used to relocate the image when SetVirtualAddressMap() is called. The
  // relocation is done by the Runtime AP.
  //
  if ((Attribute & EFI_LOAD_PE_IMAGE_ATTRIBUTE_RUNTIME_REGISTRATION) != 0) {
    if (Image->ImageContext.ImageType == EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER) {
      Image->ImageContext.FixupData = AllocateRuntimePool ((UINTN)(Image->ImageContext.FixupDataSize));
      if (Image->ImageContext.FixupData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
    }
  }

  //
  // Relocate the image in memory
  //
  Status = PeCoffLoaderRelocateImage (&Image->ImageContext);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Flush the Instruction Cache
  //
  InvalidateInstructionCacheRange ((VOID *)(UINTN)Image->ImageContext.ImageAddress, (UINTN)Image->ImageContext.ImageSize);

  //
  // Copy the machine type from the context to the image private data. This
  // is needed during image unload to know if we should call an EBC protocol
  // to unload the image.
  //
  Image->Machine = Image->ImageContext.Machine;

  //
  // Get the image entry point. If it's an EBC image, then call into the
  // interpreter to create a thunk for the entry point and use the returned
  // value for the entry point.
  //
  Image->EntryPoint   = (EFI_IMAGE_ENTRY_POINT)(UINTN)Image->ImageContext.EntryPoint;

  //
  // Fill in the image information for the Loaded Image Protocol
  //
  Image->Type               = Image->ImageContext.ImageType;
  Image->Info.ImageBase     = (VOID *)(UINTN)Image->ImageContext.ImageAddress;
  Image->Info.ImageSize     = Image->ImageContext.ImageSize;
  Image->Info.ImageCodeType = (EFI_MEMORY_TYPE) (Image->ImageContext.ImageCodeMemoryType);
  Image->Info.ImageDataType = (EFI_MEMORY_TYPE) (Image->ImageContext.ImageDataMemoryType);
  if ((Attribute & EFI_LOAD_PE_IMAGE_ATTRIBUTE_RUNTIME_REGISTRATION) != 0) {
    if (Image->ImageContext.ImageType == EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER) {
      //
      // Make a list off all the RT images so we can let the RT AP know about them.
      //
      Image->RuntimeData = AllocateRuntimePool (sizeof(EFI_RUNTIME_IMAGE_ENTRY));
      if (Image->RuntimeData == NULL) {
        goto Done;
      }
      Image->RuntimeData->ImageBase      = Image->Info.ImageBase;
      Image->RuntimeData->ImageSize      = (UINT64) (Image->Info.ImageSize);
      Image->RuntimeData->RelocationData = Image->ImageContext.FixupData;
      Image->RuntimeData->Handle         = Image->Handle;
      
      //
      // CHECKME: 
      // Old platforms doesn't have gRuntime
      //
      InsertTailList (&gRuntime->ImageHead, &Image->RuntimeData->Link);
      
      //
      // CHECKME: ImageRecord used on UEFI 2.5 specification
      //
      //InsertImageRecord (Image->RuntimeData);
    }
  }

  //
  // Fill in the entry point of the image if it is available
  //
  if (EntryPoint != NULL) {
    *EntryPoint = Image->ImageContext.EntryPoint;
  }

  //
  // Print the load address and the PDB file name if it is available
  //

  DEBUG_CODE_BEGIN ();

    UINTN Index;
    UINTN StartIndex;
    CHAR8 EfiFileName[256];


    DEBUG ((DEBUG_INFO | DEBUG_LOAD,
           "Loading driver at 0x%11p EntryPoint=0x%11p ",
           (VOID *)(UINTN) Image->ImageContext.ImageAddress,
           FUNCTION_ENTRY_POINT (Image->ImageContext.EntryPoint)));


    //
    // Print Module Name by Pdb file path.
    // Windows and Unix style file path are all trimmed correctly.
    //
    if (Image->ImageContext.PdbPointer != NULL) {
      StartIndex = 0;
      for (Index = 0; Image->ImageContext.PdbPointer[Index] != 0; Index++) {
        if ((Image->ImageContext.PdbPointer[Index] == '\\') || (Image->ImageContext.PdbPointer[Index] == '/')) {
          StartIndex = Index + 1;
        }
      }
      //
      // Copy the PDB file name to our temporary string, and replace .pdb with .efi
      // The PDB file name is limited in the range of 0~255.
      // If the length is bigger than 255, trim the redudant characters to avoid overflow in array boundary.
      //
      for (Index = 0; Index < sizeof (EfiFileName) - 4; Index++) {
        EfiFileName[Index] = Image->ImageContext.PdbPointer[Index + StartIndex];
        if (EfiFileName[Index] == 0) {
          EfiFileName[Index] = '.';
        }
        if (EfiFileName[Index] == '.') {
          EfiFileName[Index + 1] = 'e';
          EfiFileName[Index + 2] = 'f';
          EfiFileName[Index + 3] = 'i';
          EfiFileName[Index + 4] = 0;
          break;
        }
      }

      if (Index == sizeof (EfiFileName) - 4) {
        EfiFileName[Index] = 0;
      }
      DEBUG ((DEBUG_INFO | DEBUG_LOAD, "%a", EfiFileName)); // &Image->ImageContext.PdbPointer[StartIndex]));
    }
    DEBUG ((DEBUG_INFO | DEBUG_LOAD, "\n"));

  DEBUG_CODE_END ();

  return EFI_SUCCESS;

Done:

  //
  // Free memory.
  //

  if (DstBufAlocated) {
    CoreFreePages (Image->ImageContext.ImageAddress, Image->NumberOfPages);
    Image->ImageContext.ImageAddress = 0;
    Image->ImageBasePage = 0;
  }

  if (Image->ImageContext.FixupData != NULL) {
    CoreFreePool (Image->ImageContext.FixupData);
  }

  return Status;
}

EFI_STATUS
CoreLoadImageCommon (
  IN  BOOLEAN                          BootPolicy,
  IN  EFI_HANDLE                       ParentImageHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL         *FilePath,
  IN  VOID                             *SourceBuffer       OPTIONAL,
  IN  UINTN                            SourceSize,
  IN  EFI_PHYSICAL_ADDRESS             DstBuffer           OPTIONAL,
  IN OUT UINTN                         *NumberOfPages      OPTIONAL,
  OUT EFI_HANDLE                       *ImageHandle,
  OUT EFI_PHYSICAL_ADDRESS             *EntryPoint         OPTIONAL,
  IN  UINT32                           Attribute
  )
{
  LOADED_IMAGE_PRIVATE_DATA  *Image;
  LOADED_IMAGE_PRIVATE_DATA  *ParentImage;
  IMAGE_FILE_HANDLE          FHand;
  EFI_STATUS                 Status;
  EFI_STATUS                 SecurityStatus;
  EFI_HANDLE                 DeviceHandle;
  UINT32                     AuthenticationStatus;
  EFI_DEVICE_PATH_PROTOCOL   *OriginalFilePath;
  EFI_DEVICE_PATH_PROTOCOL   *HandleFilePath;
  EFI_DEVICE_PATH_PROTOCOL   *InputFilePath;
  EFI_DEVICE_PATH_PROTOCOL   *Node;
  UINTN                      FilePathSize;
  BOOLEAN                    ImageIsFromFv;
  BOOLEAN                    ImageIsFromLoadFile;
  EFI_TPL                    gEfiCurrentTpl;

  SecurityStatus = EFI_SUCCESS;

  //
  // Get current Tpl
  //
  gEfiCurrentTpl = CoreRaiseTPL (TPL_APPLICATION);
  //
  // Restore back
  //
  CoreRaiseTPL (gEfiCurrentTpl);

  ASSERT (gEfiCurrentTpl < TPL_NOTIFY);
  ParentImage = NULL;

  //
  // The caller must pass in a valid ParentImageHandle
  //
  if (ImageHandle == NULL || ParentImageHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ParentImage = CoreLoadedImageInfo (ParentImageHandle);
  if (ParentImage == NULL) {
    DEBUG((DEBUG_LOAD|DEBUG_ERROR, "LoadImageEx: Parent handle not an image handle\n"));
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&FHand, sizeof (IMAGE_FILE_HANDLE));
  FHand.Signature  = IMAGE_FILE_HANDLE_SIGNATURE;
  OriginalFilePath = FilePath;
  InputFilePath    = FilePath;
  HandleFilePath   = FilePath;
  DeviceHandle     = NULL;
  Status           = EFI_SUCCESS;
  AuthenticationStatus = 0;
  ImageIsFromFv        = FALSE;
  ImageIsFromLoadFile  = FALSE;

  //
  // If the caller passed a copy of the file, then just use it
  //
  if (SourceBuffer != NULL) {
    FHand.Source     = SourceBuffer;
    FHand.SourceSize = SourceSize;
    Status = CoreLocateDevicePath (
      &gEfiDevicePathProtocolGuid,
      &HandleFilePath,
      &DeviceHandle
      );
    if (EFI_ERROR (Status)) {
      DeviceHandle = NULL;
    }
    if (SourceSize > 0) {
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_LOAD_ERROR;
    }
  } else {
    if (FilePath == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Try to get the image device handle by checking the match protocol.
    //
    Node   = NULL;
    Status = CoreLocateDevicePath (&gEfiFirmwareVolume2ProtocolGuid, &HandleFilePath, &DeviceHandle);
    if (!EFI_ERROR (Status)) {
      ImageIsFromFv = TRUE;
    } else {
      HandleFilePath = FilePath;
      Status = CoreLocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &HandleFilePath, &DeviceHandle);
      if (EFI_ERROR (Status)) {
        if (!BootPolicy) {
          HandleFilePath = FilePath;
          Status = CoreLocateDevicePath (&gEfiLoadFile2ProtocolGuid, &HandleFilePath, &DeviceHandle);
        }
        if (EFI_ERROR (Status)) {
          HandleFilePath = FilePath;
          Status = CoreLocateDevicePath (&gEfiLoadFileProtocolGuid, &HandleFilePath, &DeviceHandle);
          if (!EFI_ERROR (Status)) {
            ImageIsFromLoadFile = TRUE;
            Node = HandleFilePath;
          }
        }
      }
    }

    //
    // Get the source file buffer by its device path.
    //
    FHand.Source = GetFileBufferByFilePath (
                      BootPolicy,
                      FilePath,
                      &FHand.SourceSize,
                      &AuthenticationStatus
                      );
    if (FHand.Source == NULL) {
      Status = EFI_NOT_FOUND;
    } else {
      FHand.FreeBuffer = TRUE;
      if (ImageIsFromLoadFile) {
        //
        // LoadFile () may cause the device path of the Handle be updated.
        //
        OriginalFilePath = AppendDevicePath (DevicePathFromHandle (DeviceHandle), Node);
      }
    }
  }

  if (EFI_ERROR (Status)) {
    Image = NULL;
    goto Done;
  }


  //
  // The original implementation performed authenticode validation here, which we removed,
  // as we only verify Apple Signature
  //

  //
  // Check Security Status.
  //
  if (EFI_ERROR (SecurityStatus) && SecurityStatus != EFI_SECURITY_VIOLATION) {
    if (SecurityStatus == EFI_ACCESS_DENIED) {
      //
      // Image was not loaded because the platform policy prohibits the image from being loaded.
      // It's the only place we could meet EFI_ACCESS_DENIED.
      //
      *ImageHandle = NULL;
    }
    Status = SecurityStatus;
    Image = NULL;
    goto Done;
  }

  //
  // Allocate a new image structure
  //
  Image = AllocateZeroPool (sizeof (LOADED_IMAGE_PRIVATE_DATA));
  if (Image == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Pull out just the file portion of the DevicePath for the LoadedImage FilePath
  //
  FilePath = OriginalFilePath;
  if (DeviceHandle != NULL) {
    Status = CoreHandleProtocol (DeviceHandle, &gEfiDevicePathProtocolGuid, (VOID **)&HandleFilePath);
    if (!EFI_ERROR (Status)) {
      FilePathSize = GetDevicePathSize (HandleFilePath) - sizeof(EFI_DEVICE_PATH_PROTOCOL);
      FilePath = (EFI_DEVICE_PATH_PROTOCOL *) (((UINT8 *)FilePath) + FilePathSize );
    }
  }
  //
  // Initialize the fields for an internal driver
  //
  Image->Signature         = LOADED_IMAGE_PRIVATE_DATA_SIGNATURE;
  Image->Info.SystemTable  = gST;
  Image->Info.DeviceHandle = DeviceHandle;
  Image->Info.Revision     = EFI_LOADED_IMAGE_PROTOCOL_REVISION;
  Image->Info.FilePath     = DuplicateDevicePath (FilePath);
  Image->Info.ParentHandle = ParentImageHandle;

  if (NumberOfPages != NULL) {
    Image->NumberOfPages = *NumberOfPages ;
  } else {
    Image->NumberOfPages = 0 ;
  }

  //
  // WARN: The original implementation calls private CoreInstallProtocolInterfaceNotify, 
  // which does not invoke notification event.
  //

  //
  // Load the image.  If EntryPoint is Null, it will not be set.
  //
  Status = CoreLoadPeImage (BootPolicy, &FHand, Image, DstBuffer, EntryPoint, Attribute);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_BUFFER_TOO_SMALL) || (Status == EFI_OUT_OF_RESOURCES)) {
      if (NumberOfPages != NULL) {
        *NumberOfPages = Image->NumberOfPages;
      }
    }
    goto Done;
  }

  if (NumberOfPages != NULL) {
    *NumberOfPages = Image->NumberOfPages;
  }

  //
  // Here we add debug image info entry
  //

  //
  // WARN: Original function reinstalls loaded image protocol to fire notifications
  //
  //Install loaded image protocol
  Status = CoreInstallProtocolInterface (
             &Image->Handle,
             &gEfiLoadedImageProtocolGuid,
             EFI_NATIVE_INTERFACE,
             &Image->Info
             );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // If DevicePath parameter to the LoadImage() is not NULL, then make a copy of DevicePath,
  // otherwise Loaded Image Device Path Protocol is installed with a NULL interface pointer.
  //
  if (OriginalFilePath != NULL) {
    Image->LoadedImageDevicePath = DuplicateDevicePath (OriginalFilePath);
  }

  //
  // Install Loaded Image Device Path Protocol onto the image handle of a PE/COFE image
  //
  Status = CoreInstallProtocolInterface (
            &Image->Handle,
            &gEfiLoadedImageDevicePathProtocolGuid,
            EFI_NATIVE_INTERFACE,
            Image->LoadedImageDevicePath
            );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Install HII Package List Protocol onto the image handle
  //
  if (Image->ImageContext.HiiResourceData != 0) {
    Status = CoreInstallProtocolInterface (
               &Image->Handle,
               &gEfiHiiPackageListProtocolGuid,
               EFI_NATIVE_INTERFACE,
               (VOID *) (UINTN) Image->ImageContext.HiiResourceData
               );
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }
  ProtectUefiImage (&Image->Info, Image->LoadedImageDevicePath);

  //
  // Success.  Return the image handle
  //
  *ImageHandle = Image->Handle;

Done:
  //
  // All done accessing the source file
  // If we allocated the Source buffer, free it
  //
  if (FHand.FreeBuffer) {
    CoreFreePool (FHand.Source);
  }
  if (OriginalFilePath != InputFilePath) {
    CoreFreePool (OriginalFilePath);
  }

  //
  // There was an error.  If there's an Image structure, free it
  //
  if (EFI_ERROR (Status)) {
    if (Image != NULL) {
      CoreUnloadAndCloseImage (Image, (BOOLEAN)(DstBuffer == 0));
      Image = NULL;
    }
  } else if (EFI_ERROR (SecurityStatus)) {
    Status = SecurityStatus;
  }

  //
  // Track the return status from LoadImage.
  //
  if (Image != NULL) {
    Image->LoadImageStatus = Status;
  }

  return Status;
}

EFI_STATUS
EFIAPI
CoreLoadImage (
  IN BOOLEAN                    BootPolicy,
  IN EFI_HANDLE                 ParentImageHandle,
  IN EFI_DEVICE_PATH_PROTOCOL   *FilePath,
  IN VOID                       *SourceBuffer   OPTIONAL,
  IN UINTN                      SourceSize,
  OUT EFI_HANDLE                *ImageHandle
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    Handle;

  PERF_LOAD_IMAGE_BEGIN (NULL);

  Status = CoreLoadImageCommon (
             BootPolicy,
             ParentImageHandle,
             FilePath,
             SourceBuffer,
             SourceSize,
             (EFI_PHYSICAL_ADDRESS) (UINTN) NULL,
             NULL,
             ImageHandle,
             NULL,
             EFI_LOAD_PE_IMAGE_ATTRIBUTE_RUNTIME_REGISTRATION | EFI_LOAD_PE_IMAGE_ATTRIBUTE_DEBUG_IMAGE_INFO_TABLE_REGISTRATION
             );

  Handle = NULL;
  if (!EFI_ERROR (Status)) {
    //
    // ImageHandle will be valid only Status is success.
    //
    Handle = *ImageHandle;
  }

  PERF_LOAD_IMAGE_END (Handle);

  return Status;
}

EFI_STATUS
EFIAPI
CoreStartImage (
  IN EFI_HANDLE  ImageHandle,
  OUT UINTN      *ExitDataSize,
  OUT CHAR16     **ExitData  OPTIONAL
  )
{
  EFI_STATUS                    Status;
  LOADED_IMAGE_PRIVATE_DATA     *Image;
  LOADED_IMAGE_PRIVATE_DATA     *LastImage;
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         Index;
  UINTN                         SetJumpFlag;
  EFI_HANDLE                    Handle;
  EFI_TPL                       gEfiCurrentTpl;

  Handle = ImageHandle;

  Image = CoreLoadedImageInfo (ImageHandle);
  if (Image == NULL  ||  Image->Started) {
    return EFI_INVALID_PARAMETER;
  }
  if (EFI_ERROR (Image->LoadImageStatus)) {
    return Image->LoadImageStatus;
  }

  //
  // The image to be started must have the machine type supported by DxeCore.
  //
  if (!EFI_IMAGE_MACHINE_TYPE_SUPPORTED (Image->Machine)) {
    //
    // Do not ASSERT here, because image might be loaded via EFI_IMAGE_MACHINE_CROSS_TYPE_SUPPORTED
    // But it can not be started.
    //
    DEBUG ((EFI_D_ERROR, "Image type %s can't be started ", GetMachineTypeName(Image->Machine)));
    DEBUG ((EFI_D_ERROR, "on %s UEFI system.\n", GetMachineTypeName(mDxeCoreImageMachineType)));
    return EFI_UNSUPPORTED;
  }

  PERF_START_IMAGE_BEGIN (Handle);


  //
  // Push the current start image context, and
  // link the current image to the head.   This is the
  // only image that can call Exit()
  //

  //
  // Dirty connect all handles instead of maintaining database
  // Original code assign HandleDatabaseKey to  CoreGetHandleDatabaseKey ()
  //

  //
  // Get current Tpl
  //
  gEfiCurrentTpl = CoreRaiseTPL (TPL_APPLICATION);
  //
  // Restore back
  //
  CoreRaiseTPL (gEfiCurrentTpl);

  LastImage         = mCurrentImage;
  mCurrentImage     = Image;
  Image->Tpl        = gEfiCurrentTpl;

  //
  // Set long jump for Exit() support
  // JumpContext must be aligned on a CPU specific boundary.
  // Overallocate the buffer and force the required alignment
  //
  Image->JumpBuffer = AllocatePool (sizeof (BASE_LIBRARY_JUMP_BUFFER) + BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT);
  if (Image->JumpBuffer == NULL) {
    //
    // Image may be unloaded after return with failure,
    // then ImageHandle may be invalid, so use NULL handle to record perf log.
    //
    PERF_START_IMAGE_END (NULL);

    //
    // Pop the current start image context
    //
    mCurrentImage = LastImage;

    return EFI_OUT_OF_RESOURCES;
  }
  Image->JumpContext = ALIGN_POINTER (Image->JumpBuffer, BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT);

  SetJumpFlag = SetJump (Image->JumpContext);
  //
  // The initial call to SetJump() must always return 0.
  // Subsequent calls to LongJump() cause a non-zero value to be returned by SetJump().
  //
  if (SetJumpFlag == 0) {
    RegisterMemoryProfileImage (Image, (Image->ImageContext.ImageType == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION ? EFI_FV_FILETYPE_APPLICATION : EFI_FV_FILETYPE_DRIVER));
    //
    // Call the image's entry point
    //
    Image->Started = TRUE;
    Image->Status = Image->EntryPoint (ImageHandle, Image->Info.SystemTable);

    //
    // Add some debug information if the image returned with error.
    // This make the user aware and check if the driver image have already released
    // all the resource in this situation.
    //
    DEBUG_CODE_BEGIN ();
      if (EFI_ERROR (Image->Status)) {
        DEBUG ((DEBUG_ERROR, "Error: Image at %11p start failed: %r\n", Image->Info.ImageBase, Image->Status));
      }
    DEBUG_CODE_END ();

    //
    // If the image returns, exit it through Exit()
    //
    CoreExit (ImageHandle, Image->Status, 0, NULL);
  }

  //
  // Image has completed.  Verify the tpl is the same
  //
  
  ASSERT (Image->Tpl == gEfiCurrentTpl);
  CoreRestoreTpl (Image->Tpl);

  CoreFreePool (Image->JumpBuffer);

  //
  // Pop the current start image context
  //
  mCurrentImage = LastImage;

  //
  // UEFI Specification - StartImage() - EFI 1.10 Extension
  // To maintain compatibility with UEFI drivers that are written to the EFI
  // 1.02 Specification, StartImage() must monitor the handle database before
  // and after each image is started. If any handles are created or modified
  // when an image is started, then EFI_BOOT_SERVICES.ConnectController() must
  // be called with the Recursive parameter set to TRUE for each of the newly
  // created or modified handles before StartImage() returns.
  //
  if (Image->Type != EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION) {
    //
    // Dirty connect all handles instead of maintaining database
    // Original function passes HandleDatabaseKey to CoreConnectHandlesByKey
    //
    Status = CoreLocateHandleBuffer (
           AllHandles,
           NULL,
           NULL,
           &HandleCount,
           &HandleBuffer
           );
    if (!EFI_ERROR (Status)) {
      for (Index = 0; Index < HandleCount; Index++) {
        gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
      }

      if (HandleBuffer != NULL) {
        CoreFreePool (HandleBuffer);
      }
    }
  }

  //
  // Handle the image's returned ExitData
  //
  DEBUG_CODE_BEGIN ();
    if (Image->ExitDataSize != 0 || Image->ExitData != NULL) {

      DEBUG ((DEBUG_LOAD, "StartImage: ExitDataSize %d, ExitData %p", (UINT32)Image->ExitDataSize, Image->ExitData));
      if (Image->ExitData != NULL) {
        DEBUG ((DEBUG_LOAD, " (%hs)", Image->ExitData));
      }
      DEBUG ((DEBUG_LOAD, "\n"));
    }
  DEBUG_CODE_END ();

  //
  //  Return the exit data to the caller
  //
  if (ExitData != NULL && ExitDataSize != NULL) {
    *ExitDataSize = Image->ExitDataSize;
    *ExitData     = Image->ExitData;
  } else {
    //
    // Caller doesn't want the exit data, free it
    //
    CoreFreePool (Image->ExitData);
    Image->ExitData = NULL;
  }

  //
  // Save the Status because Image will get destroyed if it is unloaded.
  //
  Status = Image->Status;

  //
  // If the image returned an error, or if the image is an application
  // unload it
  //
  if (EFI_ERROR (Image->Status) || Image->Type == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION) {
    CoreUnloadAndCloseImage (Image, TRUE);
    //
    // ImageHandle may be invalid after the image is unloaded, so use NULL handle to record perf log.
    //
    Handle = NULL;
  }

  //
  // Done
  //
  PERF_START_IMAGE_END (Handle);
  return Status;
}
