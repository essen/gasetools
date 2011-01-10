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

int nbl_is_nmll(char* pstrBuffer)
{
	return NBL_READ_UINT(pstrBuffer, NBL_HEADER_IDENTIFIER) == NBL_ID_NMLL;
}

/**
 * Return whether the file includes a TMLL section.
 */

int nbl_has_tmll(char* pstrBuffer)
{
	return NBL_READ_UINT(pstrBuffer, NBL_HEADER_TMLL_HEADER_SIZE) != 0;
}

/**
 * Return the position of the data.
 */

int nbl_get_data_pos(char* pstrBuffer)
{
	int ret;

	ret = NBL_READ_INT(pstrBuffer, NBL_HEADER_SIZE);
	while (NBL_READ_INT(pstrBuffer, ret) == 0)
		ret += 16;
	return ret;
}

/**
 * Return the position of the TMLL chunk.
 * Currently use brute-force to find it since the header values are still unknown.
 * Do NOT use this function if you don't know whether there is a TMLL chunk,
 * check first using the nbl_has_tmll function.
 */

int nbl_get_tmll_pos(char* pstrBuffer)
{
	int ret = 0;

	while (NBL_READ_INT(pstrBuffer, ret) != NBL_ID_TMLL)
		ret += 16;

	return ret;
}

/**
 * Open a .nbl file and check its identifier for validity.
 * Returns a new pointer with the file contents.
 */

char* nbl_load(char* pstrFilename)
{
	FILE* pFile = NULL;
	char* pstrBuffer = NULL;
	int iRead, iSize;

	if (pstrFilename == NULL)
		return NULL;

	pFile = fopen(pstrFilename, "rb");
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
	iRead = fread(pstrBuffer, sizeof(char), 4, pFile);

	if (iRead != 4) {
		free(pstrBuffer);
		pstrBuffer = NULL;
		goto nbl_load_ret;
	}

	if (!nbl_is_nmll(pstrBuffer)) {
		free(pstrBuffer);
		pstrBuffer = NULL;
		goto nbl_load_ret;
	}

	iRead = fread(pstrBuffer + 4, sizeof(char), iSize - 4, pFile);

	if (iRead != iSize - 4) {
		free(pstrBuffer);
		pstrBuffer = NULL;
		/* goto nbl_load_ret; */
	}

nbl_load_ret:
	fclose(pFile);
	return pstrBuffer;
}

/**
 * Decrypt the given buffer.
 * @todo Guess the buffer should be unsigned char* after all.
 */

void nbl_decrypt_buffer(struct bf_ctx *pCtx, char* pstrBuffer, int iSize)
{
	int i;

	iSize /= 8;
	for (i = 0; i < iSize; i++) {
		bf_decrypt(pCtx, (unsigned char*)pstrBuffer, (unsigned char*)pstrBuffer);
		pstrBuffer += 8;
	}
}

/**
 * Decrypt the headers.
 */

void nbl_decrypt_headers(struct bf_ctx *pCtx, char* pstrBuffer, int iHeaderChunksPos)
{
	int i, iNbChunks;

	iNbChunks = NBL_READ_INT(pstrBuffer, NBL_HEADER_NB_CHUNKS);

	for (i = 0; i < iNbChunks; i++)
		nbl_decrypt_buffer(pCtx, pstrBuffer + iHeaderChunksPos + NBL_CHUNK_CRYPTED_HEADER + i * 96, NBL_CHUNK_CRYPTED_SIZE);
}

/**
 * Return whether the file is using compression.
 */

int nbl_is_compressed(char* pstrBuffer)
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

int nbl_decompress(char* pstrSrc, int iSrcSize, char* pstrDest, int iDestSize)
{
	nbl_decompress_struct p;
	int iDestPos, iTmpCount, iTmpPos;
	char a, b;

	if (pstrSrc == NULL || iSrcSize <= 0 || pstrDest == NULL || iDestSize <= 0)
		return -1;

	pstrDest = memset(pstrDest, 0, iDestSize);
	iDestPos = 0;

	p.uControlByteCounter = 1;
	p.ucControlByte = 0;
	p.pstrSrc = (unsigned char*)pstrSrc;
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

void nbl_list_files(char* pstrBuffer, int iHeaderChunksPos)
{
	int i, iNbChunks;

	iNbChunks = NBL_READ_INT(pstrBuffer, NBL_HEADER_NB_CHUNKS);

	for (i = 0; i < iNbChunks; i++)
		printf("%s\n", pstrBuffer + iHeaderChunksPos + NBL_CHUNK_FILENAME + i * NBL_CHUNK_SIZE);
}

/**
 * Extract all the files from the data.
 */

void nbl_extract_all(char* pstrBuffer, char* pstrData, char* pstrDestPath)
{
	int i, iNbChunks, iLen;
	FILE* pFile;
	char* pstrFilename;

	if (pstrDestPath == NULL) {
		iLen = 0;
		pstrFilename = malloc(iLen + NBL_CHUNK_FILENAME_SIZE + 1);
	} else {
		iLen = strlen(pstrDestPath);
		pstrFilename = malloc(iLen + NBL_CHUNK_FILENAME_SIZE + 1);
		/* TODO: check return value */
		strcpy(pstrFilename, pstrDestPath);
		if (pstrDestPath[iLen - 1] != '/' && pstrDestPath[iLen - 1] != '\\')
			pstrFilename[iLen++] = '/';
	}

	iNbChunks = NBL_READ_INT(pstrBuffer, NBL_HEADER_NB_CHUNKS);

	for (i = 0; i < iNbChunks; i++) {
		strncpy(pstrFilename + iLen, pstrBuffer + 0x40 + i * NBL_CHUNK_SIZE, NBL_CHUNK_FILENAME_SIZE);

		pFile = fopen(pstrFilename, "wb");
		if (pFile) {
			fwrite(pstrData + NBL_READ_UINT(pstrBuffer, 0x60 + i * NBL_CHUNK_SIZE), 1, NBL_READ_UINT(pstrBuffer, 0x64 + i * NBL_CHUNK_SIZE), pFile);
			fclose(pFile);
		}
	}

	free(pstrFilename);
}
