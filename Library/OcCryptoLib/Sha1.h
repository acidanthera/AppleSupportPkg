/*********************************************************************
* Filename:   sha1.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding SHA1 implementation.
*********************************************************************/

#ifndef SHA1_H
#define SHA1_H

#define ROTLEFT(a, b) ((a << b) | (a >> (32 - b)))

//
// SHA1 outputs a 20 byte digest
//
#define SHA1_BLOCK_SIZE 20

typedef struct {
	UINT8  Data[64];
	UINT32 DataLen;
	UINT64 BitLen;
	UINT32 State[5];
	UINT32 k[4];
} Sha1Ctx;


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

#endif   // SHA1_H
