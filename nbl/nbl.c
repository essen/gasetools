/*
	gasetools: a set of tools to manipulate SEGA games file formats
	Copyright (C) 2010  Loic Hoguin

	This file is part of gasetools.

	gasetools is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	gasetools is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with gasetools.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nbl.h"

/**
 * Return whether the identifier is valid for a .nbl file.
 */

int nbl_is_nmll(unsigned char* pstrBuffer)
{
	return NBL_READ_UINT(pstrBuffer, NBL_HEADER_IDENTIFIER) == NBL_ID_NMLL;
}

/**
 * Return the position of the data.
 */

int nbl_get_data_pos(unsigned char* pstrBuffer)
{
	int ret;

	ret = NBL_READ_INT(pstrBuffer, NBL_HEADER_SIZE);
	ret = ret / NBL_CHUNK_PADDING_SIZE + 1;

	return ret * NBL_CHUNK_PADDING_SIZE;
}

/**
 * Open a .nbl file and check its identifier for validity.
 * Returns a new pointer with the file contents.
 */

unsigned char* nbl_load(unsigned char* pstrFilename)
{
	FILE* pFile = NULL;
	unsigned char* pstrBuffer = NULL;
	int iSize;

	if (pstrFilename == NULL)
		return NULL;

	pFile = fopen(pstrFilename, "r");
	if (pFile == NULL)
		return NULL;

	fseek(pFile, 0, SEEK_END);
	iSize = ftell(pFile);

	if (iSize < 4)
		goto nbl_load_ret;

	pstrBuffer = malloc(iSize);
	if (pstrBuffer == NULL)
		goto nbl_load_ret;

	fseek(pFile, 0, SEEK_SET);
	fread(pstrBuffer, sizeof(unsigned char), 4, pFile);

	if (!nbl_is_nmll(pstrBuffer)) {
		free(pstrBuffer);
		pstrBuffer = NULL;
		goto nbl_load_ret;
	}

	fread(pstrBuffer + 4, sizeof(unsigned char), iSize - 4, pFile);

nbl_load_ret:
	fclose(pFile);
	return pstrBuffer;
}

/**
 * Shuffle the bits to decrypt the data.
 * When building the key the last two parameters should be (1,1).
 * Otherwise they should be (18,-1). Added those parameters compared
 * to the original algorithm to simplify greatly the code.
 * Either way I'm not sure what encryption algorithm it is exactly.
 * It's mostly xor-based though, so it should be possible to encrypt ourselves too.
 */

#define BIG_BERTHA(x, y, z, pos) \
	tmp  = x[( y        & 0xff)+0x013]; \
	tmp += x[((y >>  8) & 0xff)+0x113]; \
	tmp ^= x[((y >> 16) & 0xff)+0x213]; \
	tmp += x[((y >> 24) & 0xff)+0x313]; \
	tmp ^= x[pos]; \
	tmp ^= z; \
	z = tmp;

static void nbl_crypt_shuffle(unsigned int* puKey, unsigned int* puHigh, unsigned int* puLow, int iFrom, int iInc)
{
	unsigned int a, b, tmp;

	a = *puHigh;
	b = *puLow;

	b ^= puKey[iFrom];
	for (iFrom += iInc; iFrom > 1 && iFrom < 18; iFrom += 2 * iInc) {
		BIG_BERTHA(puKey, b, a, iFrom       );
		BIG_BERTHA(puKey, a, b, iFrom + iInc);
	}
	a ^= puKey[iFrom];

	*puHigh = b;
	*puLow  = a;
}

#undef BIG_BERTHA

/**
 * Generate the decryption key based on the common base and the seed.
 * Returns a new pointer with the decryption key.
 */

unsigned int* nbl_build_key(unsigned int uSeed)
{
	FILE* pFile = NULL;
	unsigned int* puKey;
	unsigned int a, b, i, j;

	pFile = fopen(NBL_COMMON_BASE_FILENAME, "r");
	if (pFile == NULL)
		return NULL;

	puKey = malloc(NBL_KEY_SIZE);
	if (puKey == NULL) {
		fclose(pFile);
		return NULL;
	}

	puKey[0] = NBL_COMMON_BASE_HEADER;
	fread(puKey + 1, sizeof(unsigned int), NBL_KEY_SIZE / 4 - 1, pFile); // 18 + 1024
	fclose(pFile);

	for (i = 0; i < 0x12; i++) {
		// The following code has been commented out because so far it's useless.
		// If it doesn't work anymore, it might be a good idea to see why there was
		// a modulo in use with a parameter in the original code. Probably related
		// to endianess handling.

		//~ a = 0;
		//~ a = (a & 0xffffff00) | ((*(((unsigned char*)(&uSeed)    )) & 0xff)      );
		//~ a = (a & 0xffff00ff) | ((*(((unsigned char*)(&uSeed) + 1)) & 0xff) <<  8);
		//~ a = (a & 0xff00ffff) | ((*(((unsigned char*)(&uSeed) + 2)) & 0xff) << 16);
		//~ a = (a & 0x00ffffff) | ((*(((unsigned char*)(&uSeed) + 3)) & 0xff) << 24);

		puKey[i + 1] ^= uSeed; // was ^= a
	}

	for (a = 0, b = 0, i = 0; i < 0x12; i += 2) {
		nbl_crypt_shuffle(puKey, &a, &b, 1, 1);

		puKey[i + 1] = b;
		puKey[i + 2] = a;
	}

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 0x100; j += 2) {
			nbl_crypt_shuffle(puKey, &a, &b, 1, 1);

			puKey[(i << 8) + 0x13 + j    ] = b;
			puKey[(i << 8) + 0x13 + j + 1] = a;
		}
	}

	return puKey;
}

/**
 * Decrypt the given buffer.
 */

void nbl_decrypt_buffer(unsigned char* pstrBuffer, unsigned int* puKey, int iSize)
{
	int i;

	iSize /= 8;
	for (i = 0; i < iSize; i++) {
		nbl_crypt_shuffle(puKey, (unsigned int*)(pstrBuffer + 4), (unsigned int*)pstrBuffer, 18, -1);
		pstrBuffer += 8;
	}
}

/**
 * Decrypt the headers.
 */

void nbl_decrypt_headers(unsigned char* pstrBuffer, unsigned int* puKey)
{
	int i, iNbChunks, iHeaderSize;

	iNbChunks = NBL_READ_INT(pstrBuffer, NBL_HEADER_NB_CHUNKS);

	for (i = 0; i < iNbChunks; i++) {
		iHeaderSize = NBL_READ_INT(pstrBuffer, NBL_HEADER_CHUNKS + NBL_CHUNK_HEADER_SIZE);
		nbl_decrypt_buffer(pstrBuffer + NBL_HEADER_CHUNKS + NBL_CHUNK_CRYPTED_HEADER + i * iHeaderSize, puKey, NBL_CHUNK_CRYPTED_SIZE);
	}
}

/**
 * Return whether the file is using compression.
 */

int nbl_is_compressed(unsigned char* pstrBuffer)
{
	return NBL_READ_UINT(pstrBuffer, NBL_HEADER_COMPRESSED_DATA_SIZE) != 0;
}

/**
 * Decompress the source buffer into the destination buffer.
 * Both buffers must be allocated with a correct size or a segfault will occur.
 *
 * The decompression algorithm uses a control byte followed by data which is
 * processed and saved in the destination buffer. A fixed-size circular buffer
 * is used to access the source data.
 */

typedef struct {
	unsigned int uControlByteCounter;
	unsigned char ucControlByte;

	unsigned char* pstrSrc;
	int iSrcPos;
} nbl_decompress_struct;

static unsigned char nbl_decompress_get_next_control_bit(nbl_decompress_struct* p)
{
	unsigned char ret;

	p->uControlByteCounter--;
	if (p->uControlByteCounter == 0) {
		p->ucControlByte = p->pstrSrc[p->iSrcPos++];
		p->uControlByteCounter = 8;
	}

	ret = p->ucControlByte & 0x1;
	p->ucControlByte >>= 1;

	return ret;
}

int nbl_decompress(unsigned char* pstrSrc, int iSrcSize, unsigned char* pstrDest, int iDestSize)
{
	nbl_decompress_struct p;
	int iDestPos, iTmpCount, iTmpPos;
	char a, b;

	if (pstrDest == NULL || pstrSrc == NULL || iSrcSize <= 0)
		return -1;

	pstrDest = memset(pstrDest, 0, iDestSize);
	iDestPos = 0;

	p.uControlByteCounter = 1;
	p.pstrSrc = pstrSrc;
	p.iSrcPos = 0;

	while (1) {
		/* Step 1: Write uncompressed data directly */

		while (nbl_decompress_get_next_control_bit(&p)) {
			pstrDest[iDestPos++] = p.pstrSrc[p.iSrcPos++];
		}

		/* Step 2: Calculate the two values used in step 3 */

		if (nbl_decompress_get_next_control_bit(&p)) {
			iTmpCount = p.pstrSrc[p.iSrcPos++];
			iTmpPos   = p.pstrSrc[p.iSrcPos++];

			if (iTmpCount == 0 && iTmpPos == 0)
				goto nbl_decompress_ret;

			iTmpPos = (iTmpPos << 5) + (iTmpCount >> 3) - 0x2000;
			iTmpCount &= 7;

			if (iTmpCount == 0)
				iTmpCount = p.pstrSrc[p.iSrcPos++] + 1;
			else
				iTmpCount += 2;
		} else {
			a = nbl_decompress_get_next_control_bit(&p);
			b = nbl_decompress_get_next_control_bit(&p);

			iTmpCount = b + a * 2 + 2;
			iTmpPos = p.pstrSrc[p.iSrcPos++] - 0x100;
		}

		iTmpPos += iDestPos;

		/* Step 3: Use those values to retrieve what we want from the output buffer */

		while (iTmpCount-- > 0) {
			pstrDest[iDestPos++] = pstrDest[iTmpPos++];
		}
	}

nbl_decompress_ret:
	return iDestPos;
}

/**
 * List the files from the decrypted headers.
 */

void nbl_list_files(unsigned char* pstrBuffer)
{
	int i, iNbChunks;

	iNbChunks = NBL_READ_INT(pstrBuffer, NBL_HEADER_NB_CHUNKS);

	for (i = 0; i < iNbChunks; i++)
		printf("%s\n", pstrBuffer + 0x40 + i * 0x60);
}
