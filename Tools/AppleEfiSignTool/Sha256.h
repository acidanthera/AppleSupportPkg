/**
* Filename:   sha256.h
* Author:  Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details: Defines the API for the corresponding SHA1 implementation.
**/

#ifndef SHA256_H
#define SHA256_H

#include <stdlib.h>
#include <memory.h>

//
// SHA256 outputs a 32 uint8_t digest
//
#define SHA256_BLOCK_SIZE 32
#define ROTLEFT(a, b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32-(b))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)  (ROTRIGHT(x, 2)  ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))
#define EP1(x)  (ROTRIGHT(x, 6)  ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))
#define SIG0(x) (ROTRIGHT(x, 7)  ^ ROTRIGHT(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ ((x) >> 10))

typedef struct {
    uint8_t  Data[64];
    uint32_t DataLen;
    uint64_t BitLen;
    uint32_t State[8];
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
	const uint8_t  Data[],
	uint64_t       Len
	);

void Sha256Final(
	Sha256Context  *Context,
	uint8_t        HashDigest[]
	);


#endif   // SHA256_H
