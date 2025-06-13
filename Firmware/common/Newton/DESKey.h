//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_NEWTON_DES_KEY_H
#define ND_NEWTON_DES_KEY_H

#include <cstdint>

typedef unsigned short UniChar;

typedef struct
{
	uint32_t hi;
	uint32_t lo;
} SNewtNonce;

void DESCharToKey(UniChar * inString, SNewtNonce * outKey);
void DESKeySched(SNewtNonce * inKey, SNewtNonce * outKeys);
void DESPermute(const unsigned char * inPermuteTable, uint32_t inHi, uint32_t inLo, SNewtNonce * outKey);

void DESEncode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr);
void DESEncodeNonce(SNewtNonce * initVect, SNewtNonce * outNonce);

void DESDecode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr);
void DESCBCDecode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr, SNewtNonce * a4);
void DESDecodeNonce(SNewtNonce * initVect, SNewtNonce * outNonce);

#endif // ND_NEWTON_DES_KEY_H