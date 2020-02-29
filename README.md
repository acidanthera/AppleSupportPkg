AppleSupportPkg
==============

[![Build Status](https://travis-ci.com/acidanthera/AppleSupportPkg.svg?branch=master)](https://travis-ci.com/acidanthera/AppleSupportPkg) [![Scan Status](https://scan.coverity.com/projects/16467/badge.svg?flat=1)](https://scan.coverity.com/projects/16467)

-----

## ApfsDriverLoader
Open source apfs.efi loader based on reverse-engineered Apple's ApfsJumpStart driver. It chain loads the apfs.efi driver that is already embedded in the APFS container from this container.

- Loads apfs.efi from APFS container located on the block device.
- Apfs driver verbose logging suppressed.
- Version system: connects each apfs.efi to the device from which it was retrieved.
- Embedded signature verification of chainloaded apfs.efi driver, what prevents possible implant injection.

## VBoxHfs
This driver, based on [VBoxHfs](https://www.virtualbox.org/browser/vbox/trunk/src/VBox/Devices/EFI/FirmwareNew/VBoxPkg/VBoxFsDxe) from [VirtualBox OSE](https://www.virtualbox.org) project driver, implements HFS+ support with bless extensions. Commit history can be found in [VBoxFsDxe](https://github.com/nms42/VBoxFsDxe) repository. Note, that unlike other drivers, its source code is licensed under GPLv2.

## AudioDxe
Improved audio driver (currently only Intel HD audio).  
HDMI or other digital outputs don't work.

## Credits
- [Brad Conte](https://github.com/B-Con) for Sha256 implementation
- [Chromium OS project](https://github.com/chromium) for Rsa2048Sha256 signature verification implementation
- [cugu](https://github.com/cugu) for awesome research according APFS structure
- [Download-Fritz](https://github.com/Download-Fritz) for Apple EFI reverse-engineering
- [nms42](https://github.com/nms42) for advancing VBoxHfs driver
- [savvas](https://github.com/savvamitrofanov)
- [VirtualBox OSE project](https://www.virtualbox.org) for original VBoxHfs driver
- [vit9696](https://github.com/vit9696) for codereview and support in the development
- [Goldfish64](https://github.com/Goldfish64) for AudioDxe
