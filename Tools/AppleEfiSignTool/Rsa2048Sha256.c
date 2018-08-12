/**
  Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
  Use of this source code is governed by a BSD-style license that can be
  found in the LICENSE file.
 
  Implementation of RSA signature verification which uses a pre-processed key
  for computation.
**/
#include "Rsa2048Sha256.h"

#define SHA256_DIGEST_SIZE 32

uint64_t 
Mula32 (
  uint32_t A, 
  uint32_t B, 
  uint32_t C
  )
{
  uint64_t Ret = 0;

  Ret  = A;
  Ret *= B;
  Ret += C;
  return Ret;
}

uint64_t 
Mulaa32 (
  uint32_t A, 
  uint32_t B, 
  uint32_t C, 
  uint32_t D
  )
{
  uint64_t Ret = 0;

  Ret  = A;
  Ret *= B;
  Ret += C;
  Ret += D;
  return Ret;
}

/**
  A[] -= Mod
**/
static 
void 
SubMod ( 
  RsaPublicKey  *Key, 
  uint32_t      *A
  )
{
  int64_t  B     = 0;
  uint32_t Index = 0;
  for (Index = 0; Index < RSANUMWORDS; ++Index) {
    B += (uint64_t) A[Index] - Key->N[Index];
    A[Index] = (uint32_t) B;
    B >>= 32;
  }
}

//
// Return A[] >= Mod
//
static 
int 
GeMod ( 
  RsaPublicKey  *Key,
  const uint32_t      *A
  )
{
  uint32_t Index = 0;

  for (Index = RSANUMWORDS; Index;) {
    --Index;
    if (A[Index] < Key->N[Index])
      return 0;
    if (A[Index] > Key->N[Index])
      return 1;
  }
  return 1;  
}

//
// Montgomery c[] += a * b[] / R % mod
//
static 
void 
MontMulAdd ( 
  RsaPublicKey  *Key,
  uint32_t      *C,
  uint32_t      Aa,
  uint32_t      *Bb
  )
{
  uint64_t A = 0; 
  uint32_t D0 = 0;
  uint64_t B = 0;
  uint32_t Index = 0;

  A = Mula32 (Aa, Bb[0], C[0]);
  D0 = (uint32_t) A * Key->N0Inv;
  B = Mula32 (D0, Key->N[0], (uint32_t) A);

  for (Index = 1; Index < RSANUMWORDS; ++Index) {
    A = Mulaa32 (Aa, Bb[Index], C[Index], (uint32_t) (A >> 32));
    B = Mulaa32 (D0, Key->N[Index], (uint32_t) A, (uint32_t) (B >> 32));
    C[Index - 1] = (uint32_t) B;
  }

  A = (A >> 32) + (B >> 32);
  C[Index - 1] = (uint32_t) A;

  if (A >> 32) {
    SubMod (Key, C);
  }
}

//
// Montgomery c[] = a[] * b[] / R % mod
//
static 
void 
MontMul ( 
  RsaPublicKey  *Key,
  uint32_t      *C,
  uint32_t      *A,
  uint32_t      *B
  )
{
  uint32_t Index = 0;

  for (Index = 0; Index < RSANUMWORDS; ++Index)
    C[Index] = 0;
  for (Index = 0; Index < RSANUMWORDS; ++Index)
    MontMulAdd (Key, C, A[Index], B);
}

/**
  In-place public exponentiation.
  Exponent depends on the configuration (65537 (default), or 3).
 
  @param Key        Key to use in signing
  @param InOut      Input and output big-endian byte array
  @param Workbuf32  Work buffer; caller must verify this is
                    3 x RSANUMWORDS elements long.
 **/
static 
void 
ModPow ( 
  RsaPublicKey  *Key, 
  uint8_t       *InOut, 
  uint32_t      *Workbuf32
  )
{
  uint32_t *A     = NULL;
  uint32_t *Ar    = NULL;
  uint32_t *Aar   = NULL;
  uint32_t *Aaa   = NULL;
  int32_t  Index  = 0;
  uint32_t Tmp    = 0;

  A = Workbuf32;
  Ar = A + RSANUMWORDS;
  Aar = Ar + RSANUMWORDS;

  //
  // Re-use location
  //
  Aaa = Aar;  

  //
  //Convert from big endian byte array to little endian word array
  //
  for (Index = 0; Index < (int)RSANUMWORDS; ++Index) {
    Tmp =
      (InOut[((RSANUMWORDS - 1 - Index) * 4) + 0] << 24) |
      (InOut[((RSANUMWORDS - 1 - Index) * 4) + 1] << 16) |
      (InOut[((RSANUMWORDS - 1 - Index) * 4) + 2] << 8) |
      (InOut[((RSANUMWORDS - 1 - Index) * 4) + 3] << 0);
    A[Index] = Tmp;
  }

  MontMul (Key, Ar, A, Key->Rr);  
  //
  // Exponent 65537 
  //
  for (Index = 0; Index < 16; Index += 2) {
    MontMul (Key, Aar, Ar, Ar); 
    MontMul (Key, Ar, Aar, Aar);
  }
  MontMul (Key, Aaa, Ar, A);  

  if (GeMod (Key, Aaa)){
    SubMod (Key, Aaa);
  }

  //
  // Convert to bigendian byte array
  //
  for (Index = (int) RSANUMWORDS - 1; Index >= 0; --Index) {
    Tmp = Aaa[Index];
  
    *InOut++ = (uint8_t) (Tmp >> 24);
    *InOut++ = (uint8_t) (Tmp >> 16);
    *InOut++ = (uint8_t) (Tmp >>  8);
    *InOut++ = (uint8_t) (Tmp >>  0);
  }
}

/**
  PKCS#1 padding (from the RSA PKCS#1 v2.1 standard)
 
  The DER-encoded padding is defined as follows :
  0x00 || 0x01 || PS || 0x00 || T
 
  T: DER Encoded DigestInfo value which depends on the hash function used,
  for SHA-256:
  (0x)30 31 30 0d 06 09 60 86 48 01 65 03 04 02 01 05 00 04 20 || H.
 
  Length(T) = 51 octets for SHA-256
 
  PS: octet string consisting of {Length(RSA Key) - Length(T) - 3} 0xFF
 **/
static  uint8_t Sha256Tail[] = {
  0x00, 0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60,
  0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01,
  0x05, 0x00, 0x04, 0x20
};

#define PKCS_PAD_SIZE (RSANUMBYTES - SHA256_DIGEST_SIZE)

/**
 * Check PKCS#1 padding bytes
 *
 * @param sig  Signature to verify
 * @return 0 if the padding is correct.
 */
static 
int 
CheckPadding ( 
  uint8_t  *Sig
  )
{
  uint8_t   *Ptr   = NULL;
  int       Result = 0;
  uint32_t  Index  = 0;

  Ptr = Sig;
  //
  // First 2 bytes are always 0x00 0x01
  //
  Result |= *Ptr++ ^ 0x00;
  Result |= *Ptr++ ^ 0x01;
  //
  // Then 0xff bytes until the tail
  //
  for (Index = 0; Index < PKCS_PAD_SIZE - sizeof (Sha256Tail) - 2; Index++)
    Result |= *Ptr++ ^ 0xff;
  //
  // Check the tail
  //
  Result |= memcmp (Ptr, Sha256Tail, sizeof (Sha256Tail));
  return Result != 0;
}

/**
  Verify a SHA256WithRSA PKCS#1 v1.5 signature against an expected
  SHA256 hash.
 
  @param Key  RSA public key
  @param Signature   RSA signature
  @param Sha  SHA-256 digest of the content to verify
  @param Workbuf32   Work buffer; caller must verify this is
        3 x RSANUMWORDS elements long.
  @return 0 on failure, 1 on success.
 **/
int 
RsaVerify ( 
  RsaPublicKey  *Key,  
  uint8_t       *Signature,
  uint8_t       *Sha, 
  uint32_t      *Workbuf32
  )
{
  uint8_t Buf[RSANUMBYTES];
  
  //
  // Copy input to local workspace
  //
  memcpy (Buf, Signature, RSANUMBYTES);

  //
  // In-place exponentiation
  //
  ModPow (Key, Buf, Workbuf32); 

  //
  // Check the PKCS#1 padding 
  //
  if (CheckPadding (Buf) != 0) {
    return 0;
  }

  //
  // Check the digest
  //
  if (memcmp (Buf + PKCS_PAD_SIZE, Sha, SHA256_DIGEST_SIZE) != 0) {
    return 0;
  }

  //
  // All checked out OK
  //
  return 1;  
}
