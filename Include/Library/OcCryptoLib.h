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
} Sha256Ctx;

typedef struct {
	UINT8  Data[64];
	UINT32 DataLen;
	UINT64 BitLen;
	UINT32 State[5];
	UINT32 k[4];
} Sha1Ctx;

typedef struct {
   UINT8   Data[64];
   UINT32  DataLen;
   UINT64  BitLen;
   UINT32  State[4];
} Md5Ctx;

//
// Sha256
//
VOID
Sha256Init (
	Sha256Ctx  *Context
	);

VOID
Sha256Update (
	Sha256Ctx      *Context,
	CONST UINT8    Data[],
	UINT64         Len
	);

VOID
Sha256Final (
	Sha256Ctx  *Context,
	UINT8      HashDigest[]
	);

VOID
Sha256 (
    UINT8 Hash[],
    UINT8 Data[],
    UINTN Len
    );

//
// Sha1
//
VOID
Sha1Init (
	Sha1Ctx  *Ctx
	);

VOID
Sha1Update (
	Sha1Ctx      *Ctx,
	CONST UINT8  Data[],
	UINTN        Len
	);

VOID
Sha1Final (
	Sha1Ctx  *Ctx,
	UINT8    Hash[]
	);

VOID
Sha1 (
	UINT8  Hash[],
	UINT8  Data[],
	UINTN  Len
	);

//
// Md5
//
VOID
Md5Init (
  Md5Ctx  *Ctx
  );

VOID
Md5Update (
  Md5Ctx       *Ctx,
  CONST UINT8  Data[],
  UINTN        Len
  );

VOID
Md5Final (
  Md5Ctx  *Ctx,
  UINT8   Hash[]
  );

VOID
Md5 (
	UINT8  Hash[],
	UINT8  Data[],
	UINTN  Len
	);

int RsaVerify(
	RsaPublicKey  *Key,
	UINT8         *Signature,
	UINT8         *Sha,
	UINT32        *Workbuf32
	);
