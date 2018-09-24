/** @file
  UEFI PropertiesTable support
  
Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Guid/PropertiesTable.h>

#define PREVIOUS_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) - (Size)))

#define IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE SIGNATURE_32 ('I','P','R','C')

typedef struct {
  UINT32                 Signature;
  LIST_ENTRY             Link;
  EFI_PHYSICAL_ADDRESS   CodeSegmentBase;
  UINT64                 CodeSegmentSize;
} IMAGE_PROPERTIES_RECORD_CODE_SECTION;

#define IMAGE_PROPERTIES_RECORD_SIGNATURE SIGNATURE_32 ('I','P','R','D')

typedef struct {
  UINT32                 Signature;
  LIST_ENTRY             Link;
  EFI_PHYSICAL_ADDRESS   ImageBase;
  UINT64                 ImageSize;
  UINTN                  CodeSegmentCount;
  LIST_ENTRY             CodeSegmentList;
} IMAGE_PROPERTIES_RECORD;

#define IMAGE_PROPERTIES_PRIVATE_DATA_SIGNATURE SIGNATURE_32 ('I','P','P','D')