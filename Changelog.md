AppleSupport Changelog
==================

## ApfsDriverLoader
#### v2.0.2
- Fixed potential memory leaks

#### v2.0.1 
- Implemented support for AppleLoadImage protocol
	
	* Now for security reasons you should load AppleLoadImage.efi right before ApfsDriverLoader.efi. If you use Clover bootloader, you can just put AppleLoadImage.efi and ApfsDriverLoader.efi into your drivers64uefi folder.

#### v1.3.2
- Cleanup and code style

#### v1.3.0
- Version system: connects each apfs.efi to the device from which it was retrieved

#### v1.2.0
- Discovered ApfsJumpStart driver protocol

#### v1.1.9 
- Add legacy partition entry scan back for incompatible PartitionDriver's;

#### v1.1.1
- More optimization;

#### v1.1.0
- Optimization;
- Don't parse GPT by ourself, now using PartitionInfo protocols;
- Also added support for ApplePartitionInfo;

#### v1.0.1
- Fix byteorder in APFS Container GUID

#### v1.0
- Initial release

## AppleImageLoader
### v1.5.0
- Added support for AppleEfiFatBinary with signature verification
- Extended AppleLoadImage protocol with signature verification of EFI binary

### v1.0.0
- Initial release

## AppleUiSupport
### v1.0.0
- Initial release

## AppleEfiSignTool
### v1.0
- Initial release