/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <memory.h>
#include <stdint.h>

//
// Default to 2048-bit key length
//
#define CONFIG_RSA_KEY_SIZE 2048
#define RSANUMBYTES ((CONFIG_RSA_KEY_SIZE) / 8)
#define RSANUMWORDS (RSANUMBYTES / sizeof(uint32_t))

typedef struct _RsaPublicKey {
    uint32_t Size;
    uint32_t N0Inv;
    uint32_t N[RSANUMWORDS];
    uint32_t Rr[RSANUMWORDS];
} RsaPublicKey;

int RsaVerify(
	RsaPublicKey  *Key,
	uint8_t       *Signature,
	uint8_t       *Sha,
	uint32_t      *Workbuf32
	);
