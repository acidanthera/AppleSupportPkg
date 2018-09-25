/** @file

OcCryptoLib

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// Default to 2048-bit key length
//
#define CONFIG_RSA_KEY_SIZE 2048
#define RSANUMBYTES ((CONFIG_RSA_KEY_SIZE) / 8)
#define RSANUMWORDS (RSANUMBYTES / sizeof (UINT32))

typedef struct _RsaPublicKey {
    UINT32 Size;
    UINT32 N0Inv;
    UINT32 N[RSANUMWORDS];
    UINT32 Rr[RSANUMWORDS];
} RsaPublicKey;

typedef struct {
    UINT8  Data[64];
    UINT32 DataLen;
    UINT64 BitLen;
    UINT32 State[8];
} Sha256Context;

//
// Functions prototypes
//
void
Sha256Init (
	Sha256Context  *Context
	);

void
Sha256Update (
	Sha256Context  *Context,
	const UINT8  Data[],
	UINT64       Len
	);

void Sha256Final(
	Sha256Context  *Context,
	UINT8        HashDigest[]
	);

int RsaVerify(
	RsaPublicKey  *Key,
	UINT8         *Signature,
	UINT8         *Sha,
	UINT32        *Workbuf32
	);
