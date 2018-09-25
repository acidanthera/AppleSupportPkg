/**
* Filename:   sha256.c
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Implementation of the SHA-256 hashing algorithm.
              SHA-256 is one of the three algorithms in the SHA2
              specification. The others, SHA-384 and SHA-512, are not
              offered in this implementation.
              Algorithm specification can be found here:
               * http://csrc.nist.gov/publications/fips/fips180-2/fips180-2withchangenotice.pdf
              This implementation uses little endian byte order.
**/

#include <Library/OcCryptoLib/OcCryptoLib.h>

static const UINT32 K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

void
Sha256Transform (
	Sha256Context *Context,
	const UINT8 Data[]
	)
{
    UINT32 A      =     0;
    UINT32 B      =     0;
    UINT32 C      =     0;
    UINT32 D      =     0;
    UINT32 E      =     0;
    UINT32 F      =     0;
    UINT32 G      =     0;
    UINT32 H      =     0;
    UINT32 Index1 =     0;
    UINT32 Index2 =     0;
    UINT32 T1     =     0;
    UINT32 T2     =     0;
    UINT32 M[64];

    for (Index1 = 0, Index2 = 0; Index1 < 16; Index1++, Index2 += 4)
        M[Index1] = (Data[Index2] << 24) | (Data[Index2 + 1] << 16) | (Data[Index2 + 2] << 8) | (Data[Index2 + 3]);
    for ( ; Index1 < 64; Index1++)
        M[Index1] = SIG1(M[Index1 - 2]) + M[Index1 - 7] + SIG0(M[Index1 - 15]) + M[Index1 - 16];

    A = Context->State[0];
    B = Context->State[1];
    C = Context->State[2];
    D = Context->State[3];
    E = Context->State[4];
    F = Context->State[5];
    G = Context->State[6];
    H = Context->State[7];

    for (Index1 = 0; Index1 < 64; Index1++) {
        T1 = H + EP1(E) + CH(E, F, G) + K[Index1] + M[Index1];
        T2 = EP0(A) + MAJ(A, B, C);
        H = G;
        G = F;
        F = E;
        E = D + T1;
        D = C;
        C = B;
        B = A;
        A = T1 + T2;
    }

    Context->State[0] += A;
    Context->State[1] += B;
    Context->State[2] += C;
    Context->State[3] += D;
    Context->State[4] += E;
    Context->State[5] += F;
    Context->State[6] += G;
    Context->State[7] += H;
}

void Sha256Init (
	Sha256Context *Context
	)
{
    Context->DataLen = 0;
    Context->BitLen = 0;
    Context->State[0] = 0x6a09e667;
    Context->State[1] = 0xbb67ae85;
    Context->State[2] = 0x3c6ef372;
    Context->State[3] = 0xa54ff53a;
    Context->State[4] = 0x510e527f;
    Context->State[5] = 0x9b05688c;
    Context->State[6] = 0x1f83d9ab;
    Context->State[7] = 0x5be0cd19;
}

void Sha256Update (
	Sha256Context *Context,
	const UINT8 Data[],
	UINT64 Len
	)
{
    UINT32 Index;

    for (Index = 0; Index < Len; Index++) {
        Context->Data[Context->DataLen] = Data[Index];
        Context->DataLen++;
        if (Context->DataLen == 64) {
            Sha256Transform (Context, Context->Data);
            Context->BitLen += 512;
            Context->DataLen = 0;
        }
    }
}

void Sha256Final (
	Sha256Context *Context,
	UINT8 HashDigest[]
	)
{
    UINT32 Index  = 0;

    Index = Context->DataLen;

    //
    // Pad whatever data is left in the buffer.
    //
    if (Context->DataLen < 56) {
        Context->Data[Index++] = 0x80;
        ZeroMem (Context->Data + Index, 56-Index);
    } else {
        Context->Data[Index++] = 0x80;
        ZeroMem (Context->Data + Index, 64-Index);
        Sha256Transform (Context, Context->Data);
        ZeroMem (Context->Data, 56);
    }

    //
    // Append to the padding the total message's length in bits and transform.
    //
    Context->BitLen  += Context->DataLen * 8;
    Context->Data[63] = (UINT8) Context->BitLen;
    Context->Data[62] = (UINT8) (Context->BitLen >> 8);
    Context->Data[61] = (UINT8) (Context->BitLen >> 16);
    Context->Data[60] = (UINT8) (Context->BitLen >> 24);
    Context->Data[59] = (UINT8) (Context->BitLen >> 32);
    Context->Data[58] = (UINT8) (Context->BitLen >> 40);
    Context->Data[57] = (UINT8) (Context->BitLen >> 48);
    Context->Data[56] = (UINT8) (Context->BitLen >> 56);
    Sha256Transform (Context, Context->Data);

    //
    // Since this implementation uses little endian byte ordering and SHA uses big endian,
    // reverse all the bytes when copying the final State to the output hash.
    //
    for (Index = 0; Index < 4; Index++) {
        HashDigest[Index]      = (UINT8) ((Context->State[0] >> (24 - Index * 8)) & 0x000000ff);
        HashDigest[Index + 4]  = (UINT8) ((Context->State[1] >> (24 - Index * 8)) & 0x000000ff);
        HashDigest[Index + 8]  = (UINT8) ((Context->State[2] >> (24 - Index * 8)) & 0x000000ff);
        HashDigest[Index + 12] = (UINT8) ((Context->State[3] >> (24 - Index * 8)) & 0x000000ff);
        HashDigest[Index + 16] = (UINT8) ((Context->State[4] >> (24 - Index * 8)) & 0x000000ff);
        HashDigest[Index + 20] = (UINT8) ((Context->State[5] >> (24 - Index * 8)) & 0x000000ff);
        HashDigest[Index + 24] = (UINT8) ((Context->State[6] >> (24 - Index * 8)) & 0x000000ff);
        HashDigest[Index + 28] = (UINT8) ((Context->State[7] >> (24 - Index * 8)) & 0x000000ff);
    }
}
