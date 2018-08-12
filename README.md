AppleSupportPkg
==============

[![Build Status](https://travis-ci.org/acidanthera/AppleSupportPkg.svg?branch=master)](https://travis-ci.org/acidanthera/AppleSupportPkg) [![Scan Status](https://scan.coverity.com/projects/16467/badge.svg?flat=1)](https://scan.coverity.com/projects/16467)

-----

## ApfsDriverLoader
Open source apfs.efi loader based on reverse-engineered Apple's ApfsJumpStart driver

- Loads apfs.efi from ApfsContainer located on block device.
- Apfs driver verbose logging suppressed.
- Version system: connects each apfs.efi to the device from which it was retrieved
- Supports AppleLoadImage protocol provides EfiBinary signature check
- **WARNING**: Please load AppleLoadImage.efi right before ApfsDriverLoader, or just put it inside drivers64uefi folder of your Clover bootloader

## AppleLoadImage
Implementation of AppleLoadImage protocol discoverd in ApfsJumpStart Apple driver. This protocol installs in CoreDxe Apple's firmware.

It provides safe EFI binary loading into memory by verifiyng it's signature.

- Also gives ability to use native ApfsJumpStart driver from Apple firmware
- **WARNING**: ApplePartitionDriver needed

## AppleDxeImageVerificationLib
This library provides reverse-engineered Apple's crypto signature algorithms.

## Credits
- [cugu](https://github.com/cugu) for awesome research according APFS structure
- [CupertinoNet](https://github.com/CupertinoNet) and [Download-Fritz](https://github.com/Download-Fritz) for Apple EFI reverse-engineering
- [vit9696](https://github.com/vit9696) for codereview and support in the development
- [Chromium OS project](https://github.com/chromium) for Rsa2048Sha256 signature verification implementation
- [Brad Conte](https://github.com/B-Con) for Sha256 implementation
- [savvas](https://github.com/savvamitrofanov) 
