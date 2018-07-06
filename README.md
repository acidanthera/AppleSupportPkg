ApfsSupportPkg
==============

[![Build Status](https://travis-ci.org/acidanthera/ApfsSupportPkg.svg?branch=master)](https://travis-ci.org/acidanthera/ApfsSupportPkg)

-----

## APFS Driver Loader
Open source apfs.efi loader based on reverse-engineered Apple's ApfsJumpStart driver

- Loads apfs.efi from ApfsContainer located on block device.
- Apfs driver verbose logging suppressed.

## AppleLoadImage
Implementation of AppleLoadImage protocol discoverd in ApfsJumpStart Apple driver. This protocol installs in CoreDxe Apple's firmware.

- Gives ability to use native ApfsJumpStart driver from Apple firmware

## Credits
- [cugu](https://github.com/cugu) for awesome research according APFS structure
- [CupertinoNet](https://github.com/CupertinoNet) and [Download-Fritz](https://github.com/Download-Fritz) for Apple EFI reverse-engineering
- [vit9696](https://github.com/vit9696) for codereview and support in the development
- [savvas](https://github.com/savvamitrofanov) 
