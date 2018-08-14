/** @file

AppleImageLoader

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef APPLE_IMAGE_LOADER_H
#define APPLE_IMAGE_LOADER_H

#include <Base.h>
#include <Uefi.h>
#include <PiDxe.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/AppleDxeImageVerificationLib.h>
#include <Library/DxeServicesLib.h>
#include <Protocol/AppleLoadImage.h>


#define APPLE_EFI_FAT_MAGIC  0x0ef1fab9
#define CPU_ARCH_ABI64 0x01000000
#define CPUYPE_X86 7
#define CPUYPE_X86_64 (CPUYPE_X86 | CPU_ARCH_ABI64)

typedef struct {
    //
    // Probably 0x07 (CPUYPE_X86) or 0x01000007 (CPUYPE_X86_64)
    //
    UINT32 CpuType;    
    //
    // Probably 3 (CPU_SUBTYPE_I386_ALL)
    //
    UINT32 CpuSubtype; 
    //
    // Offset to beginning of architecture section
    //
    UINT32 Offset;    
    //
    // Size of arch section
    //
    UINT32 Size;
    //
    // Alignment
    //
    UINT32 Align;
} APPLE_EFI_FAT_ARCH_HEADER;

typedef struct {
    //
    // Apple EFI fat binary magic number (0x0ef1fab9)
    //
    UINT32 Magic;         
    //
    // Number of architectures
    //
    UINT32 NumArchs;        
    //
    // Architecture headers
    //
    APPLE_EFI_FAT_ARCH_HEADER Archs[];
} APPLE_EFI_FAT_HEADER;

#endif //APPLE_IMAGE_LOADER_H
