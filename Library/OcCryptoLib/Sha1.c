/*********************************************************************
* Filename:   sha1.c
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Implementation of the SHA1 hashing algorithm.
              Algorithm specification can be found here:
               * http://csrc.nist.gov/publications/fips/fips180-2/fips180-2withchangenotice.pdf
              This implementation uses little endian byte order.
*********************************************************************/

#include <Library/BaseMemoryLib.h>
#include "Sha1.h"

VOID
Sha1Transform (
  Sha1Ctx      *Ctx,
  CONST UINT8  Data[]
  )
{
  UINT32 a, b, c, d, e, i, j, t, m[80];

  for (i = 0, j = 0; i < 16; ++i, j += 4) {
    m[i] = (Data[j] << 24) + (Data[j + 1] << 16) + (Data[j + 2] << 8) + (Data[j + 3]);
  }

  for ( ; i < 80; ++i) {
    m[i] = (m[i - 3] ^ m[i - 8] ^ m[i - 14] ^ m[i - 16]);
    m[i] = (m[i] << 1) | (m[i] >> 31);
  }

  a = Ctx->State[0];
  b = Ctx->State[1];
  c = Ctx->State[2];
  d = Ctx->State[3];
  e = Ctx->State[4];

  for (i = 0; i < 20; ++i) {
    t = ROTLEFT(a, 5) + ((b & c) ^ (~b & d)) + e + Ctx->k[0] + m[i];
    e = d;
    d = c;
    c = ROTLEFT(b, 30);
    b = a;
    a = t;
  }
  for ( ; i < 40; ++i) {
    t = ROTLEFT(a, 5) + (b ^ c ^ d) + e + Ctx->k[1] + m[i];
    e = d;
    d = c;
    c = ROTLEFT(b, 30);
    b = a;
    a = t;
  }
  for ( ; i < 60; ++i) {
    t = ROTLEFT(a, 5) + ((b & c) ^ (b & d) ^ (c & d))  + e + Ctx->k[2] + m[i];
    e = d;
    d = c;
    c = ROTLEFT(b, 30);
    b = a;
    a = t;
  }
  for ( ; i < 80; ++i) {
    t = ROTLEFT(a, 5) + (b ^ c ^ d) + e + Ctx->k[3] + m[i];
    e = d;
    d = c;
    c = ROTLEFT(b, 30);
    b = a;
    a = t;
  }

  Ctx->State[0] += a;
  Ctx->State[1] += b;
  Ctx->State[2] += c;
  Ctx->State[3] += d;
  Ctx->State[4] += e;
}

VOID
Sha1Init (
  Sha1Ctx  *Ctx
  )
{
  Ctx->DataLen = 0;
  Ctx->BitLen = 0;
  Ctx->State[0] = 0x67452301;
  Ctx->State[1] = 0xEFCDAB89;
  Ctx->State[2] = 0x98BADCFE;
  Ctx->State[3] = 0x10325476;
  Ctx->State[4] = 0xc3d2e1f0;
  Ctx->k[0] = 0x5a827999;
  Ctx->k[1] = 0x6ed9eba1;
  Ctx->k[2] = 0x8f1bbcdc;
  Ctx->k[3] = 0xca62c1d6;
}

VOID
Sha1Update (
  Sha1Ctx      *Ctx,
  CONST UINT8  Data[],
  UINTN        Len
  )
{
  UINTN Index = 0;

  for (Index = 0; Index < Len; ++Index) {
    Ctx->Data[Ctx->DataLen] = Data[Index];
    Ctx->DataLen++;
    if (Ctx->DataLen == 64) {
      Sha1Transform (Ctx, Ctx->Data);
      Ctx->BitLen += 512;
      Ctx->DataLen = 0;
    }
  }
}

VOID
Sha1Final (
  Sha1Ctx  *Ctx,
  UINT8    Hash[]
  )
{
  UINT32 Index = Ctx->DataLen;

  //
  // Pad whatever Data is left in the buffer.
  //
  if (Ctx->DataLen < 56) {
    Ctx->Data[Index++] = 0x80;
    ZeroMem (Ctx->Data + Index, 56-Index);
  } else {
    Ctx->Data[Index++] = 0x80;
    ZeroMem (Ctx->Data + Index, 64-Index);
    Sha1Transform (Ctx, Ctx->Data);
    ZeroMem (Ctx->Data, 56);
  }

  //
  // Append to the padding the total message's length in bits and transform.
  //
  Ctx->BitLen += Ctx->DataLen * 8;
  Ctx->Data[63] = (UINT8) (Ctx->BitLen);
  Ctx->Data[62] = (UINT8) (Ctx->BitLen >> 8);
  Ctx->Data[61] = (UINT8) (Ctx->BitLen >> 16);
  Ctx->Data[60] = (UINT8) (Ctx->BitLen >> 24);
  Ctx->Data[59] = (UINT8) (Ctx->BitLen >> 32);
  Ctx->Data[58] = (UINT8) (Ctx->BitLen >> 40);
  Ctx->Data[57] = (UINT8) (Ctx->BitLen >> 48);
  Ctx->Data[56] = (UINT8) (Ctx->BitLen >> 56);
  Sha1Transform (Ctx, Ctx->Data);

  //
  // Since this implementation uses little endian byte ordering and MD uses big endian,
  // reverse all the bytes when copying the final State to the output Hash.
  //
  for (Index = 0; Index < 4; ++Index) {
    Hash[Index]      = (UINT8) ((Ctx->State[0] >> (24 - Index * 8)) & 0x000000FF);
    Hash[Index + 4]  = (UINT8) ((Ctx->State[1] >> (24 - Index * 8)) & 0x000000FF);
    Hash[Index + 8]  = (UINT8) ((Ctx->State[2] >> (24 - Index * 8)) & 0x000000FF);
    Hash[Index + 12] = (UINT8) ((Ctx->State[3] >> (24 - Index * 8)) & 0x000000FF);
    Hash[Index + 16] = (UINT8) ((Ctx->State[4] >> (24 - Index * 8)) & 0x000000FF);
  }
}

VOID
Sha1 (
  UINT8  Hash[],
  UINT8  Data[],
  UINTN  Len
  )
{
  Sha1Ctx Ctx;

  Sha1Init (&Ctx);
  Sha1Update (&Ctx, Data, Len);
  Sha1Final (&Ctx,Hash);
}
