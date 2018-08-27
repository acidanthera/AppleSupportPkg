/** @file

APFS Driver Loader - loads apfs.efi from EfiBootRecord block

Copyright (c) 2017-2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef FLETCHER_CHECKSUM_H_
#define FLETCHER_CHECKSUM_H_

//
// Function to calculate APFS block checksum.
// According to the apple docs the Fletcher’s checksum algorithm is used.
// Apple uses a variant of the algorithm described in a paper by John Kodis.
// The following algorithm shows this procedure.
// The input is the block without the first 8 byte.
//

UINT64
ApfsBlockChecksumCalculate (
  UINT32  *Data,
  UINTN  DataSize
  );

BOOLEAN
ApfsBlockChecksumVerify (
  UINT8   *Data,
  UINTN  DataSize
  );

#endif // FLETCHER_CHECKSUM_H_