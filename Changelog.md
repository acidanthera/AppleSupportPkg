ApfsSupport Changelog
==================

#### v1.1.1
- More optimization;

#### v1.1.0
- Optimization;
- Don't parse GPT by ourself, now using PartitionInfo protocols;
- Also added support for ApplePartitionInfo;
- **WARNING**: Now your system should have **Partition Driver** loaded.  Most UEFI system have it out of box. On legacy you should load **PartitionDXE** from Clover or binary from EDK2.

#### v1.0.1
- Fix byteorder in APFS Container GUID

#### v1.0
- Initial release
