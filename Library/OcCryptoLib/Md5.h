/*********************************************************************
* Filename:   md5.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding MD5 implementation.
*********************************************************************/
#ifndef MD5_H
#define MD5_H

//
// MD5 outputs a 16 byte digest
//
#define MD5_BLOCK_SIZE 16

typedef struct {
   UINT8   Data[64];
   UINT32  DataLen;
   UINT64  BitLen;
   UINT32  State[4];
} Md5Ctx;

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

#endif   // MD5_H
