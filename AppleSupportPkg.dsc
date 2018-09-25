## @file
# Copyright (c) 2017-2018, savvas
#
# All rights reserved.<BR>
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
##

[Defines]
  PLATFORM_NAME           = AppleSupportPkg
  PLATFORM_GUID           = 86972B2A-2A21-492D-88CF-2906C3E273AF
  PLATFORM_VERSION        = 1.0
  SUPPORTED_ARCHITECTURES = X64
  BUILD_TARGETS           = RELEASE|DEBUG
  SKUID_IDENTIFIER        = DEFAULT
  DSC_SPECIFICATION       = 0x00010006

[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseRngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
!if $(TARGET) == DEBUG
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
!else
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
!endif
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  OcAppleImageVerificationLib|AppleSupportPkg/Library/OcAppleImageVerificationLib/OcAppleImageVerificationLib.inf
  OcCryptoLib|AppleSupportPkg/Library/OcCryptoLib/OcCryptoLib.inf
  AppleEventLib|AppleSupportPkg/Library/AppleEventLib/AppleEventLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf

[Components]
  AppleSupportPkg/Library/OcCryptoLib/OcCryptoLib.inf
  AppleSupportPkg/Library/OcAppleImageVerificationLib/OcAppleImageVerificationLib.inf
  AppleSupportPkg/Platform/AppleImageLoader/AppleImageLoader.inf
  AppleSupportPkg/Platform/AppleUiSupport/AppleUiSupport.inf
  AppleSupportPkg/Platform/ApfsDriverLoader/ApfsDriverLoader.inf

[PcdsFixedAtBuild]
!if $(TARGET) == DEBUG
  gEfiMdePkgTokenSpaceGuid.PcdMaximumAsciiStringLength|0
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0xC04A054F
!endif

[BuildOptions]
  # While there are no PCDs as of now, there at least are some custom macros.
  DEFINE APPLESUPPORTPKG_BUILD_OPTIONS_GEN = -D DISABLE_NEW_DEPRECATED_INTERFACES $(APPLESUPPORTPKG_BUILD_OPTIONS)

  INTEL:DEBUG_*_*_CC_FLAGS   = $(APPLESUPPORTPKG_BUILD_OPTIONS_GEN)
  INTEL:RELEASE_*_*_CC_FLAGS = /D MDEPKG_NDEBUG $(APPLESUPPORTPKG_BUILD_OPTIONS_GEN)
  GCC:DEBUG_*_*_CC_FLAGS     = $(APPLESUPPORTPKG_BUILD_OPTIONS_GEN)
  GCC:RELEASE_*_*_CC_FLAGS   = -D MDEPKG_NDEBUG $(APPLESUPPORTPKG_BUILD_OPTIONS_GEN)
  MSFT:DEBUG_*_*_CC_FLAGS    = $(APPLESUPPORTPKG_BUILD_OPTIONS_GEN)
  MSFT:RELEASE_*_*_CC_FLAGS  = /D MDEPKG_NDEBUG $(APPLESUPPORTPKG_BUILD_OPTIONS_GEN)
  XCODE:RELEASE_*_*_CC_FLAGS = -Wno-varargs -flto -D MDEPKG_NDEBUG $(APPLESUPPORTPKG_BUILD_OPTIONS_GEN)
  XCODE:DEBUG_*_*_CC_FLAGS   = -Wno-varargs $(APPLESUPPORTPKG_BUILD_OPTIONS_GEN)
