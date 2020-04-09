AppleSupport Changelog
======================
#### v2.1.7
- Added unaligned and custom-size I/O support

#### v2.1.6
- Added SSE2 support in memory intrinsics for better performance
- Added AudioDxe to the list of bundled drivers
- Switch to merged OpenCorePkg instead of OcSupportPkg

#### v2.1.5
- Fixed assertions in AppleUsbKbDxe driver
- Moved FwRuntimeServices driver to OcSupportPkg and bundle with OpenCore
- Moved VerifyMsrE2 tool to OcSupportPkg and bundle with OpenCore
- Moved AppleUsbKbDxe driver to OcSupportPkg and bundle with OpenCore

#### v2.1.4
- Added workaround for V to NV variable upgrade in FwRuntimeServices
- Added support for RequestBootVarFallback in FwRuntimeServices

#### v2.1.3
- Added VerifyMsrE2 boot screen compatibility

#### v2.1.2
- Fixed enabling RO/WO variables before OS start
- Fixed extra delay during FwRuntimeServices startup

#### v2.1.1
- Moved AppleGenericInput into OpenCore mainline

#### v2.1.0
- Moved AppleUiSupport into OpenCore mainline
- Moved CleanNvram into OpenCore mainline

#### v2.0.9
- Fixed memory corruption on select platforms (by @mjg59)
- Added AppleGenericInput (formerly AptioInputFix)
- Added CleanNvram and VerifyMsrE2 tools
- Added FwRuntimeServices OpenCore runtime compatibility layer
- Incrased VerifyMsrE2 timeout to 5s for more cores (by @mrmiller)

#### v2.0.8
- Respect OpenCore scan policy during apfs driver loading

#### v2.0.7
- Added bless-compatible VBoxHfs driver (by @nms42)
- Added compatibility with OpenCore logging protocols

#### v2.0.6
- Dynamically increase mouse polling speed to accomodate for platform specifics (thx jan4321)

#### v2.0.5
- Disable mouse polling when it does not fit the timer window
- Include AppleEvent-compatible UsbKbDxe driver in the package
- Support extracting APFS driver from the extentents as defined by the spec
- Fix Apple image signature verification failure on multiple platforms
- Temporary remove AppleImageLoader from the package (till it gains Secure Boot compat)

**WARNING**: Apple image signature verification implementation is not prone to untrusted input as of yet.

#### v2.0.4
- Implemented a complete port of AppleEvent (thx Download-Fritz for the base)
- Reduced mouse polling timer to fix booting issues on some Dell laptops

#### v2.0.3
- Embedded signature verification into ApfsDriverLoader
- Removed AppleLoadImage support from ApfsDriverLoader due to security reasons
  * AppleLoadImage can be compromised, because our implementation isn't embedded into firmware, so it is possible, that attacker can simply bypass security checks by adding dummy implementation
- Added FvOnFv2Thunk into FirmwareVolume injector to create back-compatibility for broken UEFI implementation on some boards, for example MSI

#### v2.0.2
- Fixed potential memory leaks
- Added support for AppleEfiFatBinary with signature verification (AppleImageLoader 1.5.0)
- Extended AppleLoadImage protocol with signature verification of EFI binary (AppleImageLoader 1.5.0)

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
