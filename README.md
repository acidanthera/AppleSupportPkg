AppleSupportPkg
==============

[![Build Status](https://travis-ci.org/acidanthera/AppleSupportPkg.svg?branch=master)](https://travis-ci.org/acidanthera/AppleSupportPkg) [![Scan Status](https://scan.coverity.com/projects/16467/badge.svg?flat=1)](https://scan.coverity.com/projects/16467)

-----

## ApfsDriverLoader
Open source apfs.efi loader based on reverse-engineered Apple's ApfsJumpStart driver. It chain loads the apfs.efi driver that is already embedded in the APFS container from this container.

- Loads apfs.efi from APFS container located on the block device.
- Apfs driver verbose logging suppressed.
- Version system: connects each apfs.efi to the device from which it was retrieved.
- It supports AppleLoadImage protocol, which provides EfiBinary signature check, what prevents possible implant injection.

## AppleImageLoader
Secure AppleEfiFat binary driver with implementation of AppleLoadImage protocol with EfiBinary signature verification.

It provides secure loading of Apple EFI binary files into memory by pre-authenticating its signature.

## AppleUiSupport
Driver which implements set of protocol for support EfiLoginUi which used for FileVault as login window. In short, it implements FileVault support and replaces AppleKeyMapAggregator. efi, AppleEvent. efi, AppleUiTheme. efi, FirmwareVolume. efi, AppleImageCodec. efi. Also, it contains hash service fixes and unicode collation for some boards. This fixes removed from AptioMemoryFix in R23.

## AppleEfiSignTool
Open source tool verifying Apple EFI binaries. It supports ApplePE and AppleFat binaries.

## AppleDxeImageVerificationLib
This library provides Apple's crypto signature algorithm for EFI binaries.

## Credits
- [cugu](https://github.com/cugu) for awesome research according APFS structure
- [CupertinoNet](https://github.com/CupertinoNet) and [Download-Fritz](https://github.com/Download-Fritz) for Apple EFI reverse-engineering
- [vit9696](https://github.com/vit9696) for codereview and support in the development
- [Chromium OS project](https://github.com/chromium) for Rsa2048Sha256 signature verification implementation
- [Brad Conte](https://github.com/B-Con) for Sha256 implementation
- [savvas](https://github.com/savvamitrofanov) 
