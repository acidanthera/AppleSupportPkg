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

## FwRuntimeServices
This driver provides the necessary compatibility improvements required for normal functioning of UEFI Runtime Services such as date, time, NVRAM variable storage, and others in macOS.

## VBoxHfs
This driver, based on [VBoxHfs](https://www.virtualbox.org/browser/vbox/trunk/src/VBox/Devices/EFI/FirmwareNew/VBoxPkg/VBoxFsDxe) from [VirtualBox OSE](https://www.virtualbox.org) project driver, implements HFS+ support with bless extensions. Commit history can be found in [VBoxFsDxe](https://github.com/nms42/VBoxFsDxe) repository. Note, that unlike other drivers, its source code is licensed under GPLv2.

## VerifyMsrE2

Certain firmwares fail to properly initialize 0xE2 MSR register (`MSR_BROADWELL_PKG_CST_CONFIG_CONTROL`) across all the cores. This application prints 0xE2 values of all the cores and reports 0xE2 status. The notable example of desyncrhonised 0xE2 MSR registers are several GIGABYTE UEFI firmwares for Intel 100 Series and Intel 200 Series chipsets.

CFG Lock option is available on most APTIO V firmwares, although it may be hidden from the GUI. If VerifyMsrE2 reports that your 0xE2 register is consistently locked, you may try to unlock this option directly.

1. Download [UEFITool](https://github.com/LongSoft/UEFITool/releases) and [IFR-Extractor](https://github.com/LongSoft/Universal-IFR-Extractor/releases).
2. Open your firmware image in UEFITool and find `CFG Lock` unicode string. If it is not present, your firmware does not support this and you should stop.
3. Extract the Setup.bin PE32 Image Section that UEFITool found via Extract Body.
4. Run IFR-Extractor on the extracted file (e.g. `./ifrextract Setup.bin Setup.txt`).
5. Find `CFG Lock, VarStoreInfo (VarOffset/VarName):` in `Setup.txt` and remember the offset right after it (e.g. `0x123`).
6. Download and run a [modified GRUB Shell](http://brains.by/posts/bootx64.7z), thx to [brainsucker](https://geektimes.com/post/258090/) for the binary. A more up to date version may be found in [grub-mod-setup_var](https://github.com/datasone/grub-mod-setup_var) repo.
7. Enter `setup_var 0x123 0x00` command, where `0x123` should be replaced by your actual offset and reboot.

**WARNING**: variable offsets are unique not only to each motherboard but even to its firmware version. Never ever try to use an offset without checking.

## Credits
- [Brad Conte](https://github.com/B-Con) for Sha256 implementation
- [Chromium OS project](https://github.com/chromium) for Rsa2048Sha256 signature verification implementation
- [cugu](https://github.com/cugu) for awesome research according APFS structure
- [Download-Fritz](https://github.com/Download-Fritz) for Apple EFI reverse-engineering
- [nms42](https://github.com/nms42) for advancing VBoxHfs driver
- [savvas](https://github.com/savvamitrofanov)
- [VirtualBox OSE project](https://www.virtualbox.org) for original VBoxHfs driver
- [vit9696](https://github.com/vit9696) for codereview and support in the development
