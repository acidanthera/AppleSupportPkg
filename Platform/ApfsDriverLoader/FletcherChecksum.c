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
  )
{
  UINTN         Index;
  UINT64        Sum1 = 0;
  UINT64        Check1 = 0;
  UINT64        Sum2 = 0;
  UINT64        Check2 = 0;
  CONST UINT64  ModValue = 0xFFFFFFFF;
  for (Index = 0; Index < DataSize / sizeof (UINT32); Index++) {
    Sum1 = ((Sum1 + (UINT64)Data[Index]) % ModValue);
    Sum2 = (Sum2 + Sum1) % ModValue;
  }

  Check1 = ModValue - ((Sum1 + Sum2) % ModValue);
  Check2 = ModValue - ((Sum1 + Check1) % ModValue);

  return (Check2 << 32) | Check1;
}

//
// Function to check block checksum.
// Returns TRUE if the checksum is valid.
//
BOOLEAN
ApfsBlockChecksumVerify (
  UINT8   *Data,
  UINTN  DataSize
  )
{
  UINT64  NewChecksum;
  UINT64  *CurrChecksum = (UINT64 *)Data;

  NewChecksum = ApfsBlockChecksumCalculate (
    (UINT32 *)(Data + sizeof (UINT64)),
    DataSize - sizeof (UINT64)
    );

  return NewChecksum == *CurrChecksum;
}
