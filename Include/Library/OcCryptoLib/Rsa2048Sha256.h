/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <Library/BaseMemoryLib.h>

//
// Default to 2048-bit key length
//
#define CONFIG_RSA_KEY_SIZE 2048
#define RSANUMBYTES ((CONFIG_RSA_KEY_SIZE) / 8)
#define RSANUMWORDS (RSANUMBYTES / sizeof(UINT32))

typedef struct _RsaPublicKey {
    UINT32 Size;
    UINT32 N0Inv;
    UINT32 N[RSANUMWORDS];
    UINT32 Rr[RSANUMWORDS];
} RsaPublicKey;

int RsaVerify(
	RsaPublicKey  *Key,
	UINT8         *Signature,
	UINT8         *Sha,
	UINT32        *Workbuf32
	);
