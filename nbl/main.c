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

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "nbl.h"

/**
 * Options masks.
 */

#define OPTION_LIST		0x1
#define OPTION_DEBUG	0x2
#define OPTION_VERBOSE	0x4

/**
 * Save the given buffer in a file. Used for debugging purpose only.
 */

void debug_save_buffer(char* pstrFilename, char* pstrBuffer, int iSize)
{
	FILE* pFile;

	pFile = fopen(pstrFilename, "wb");
	if (pFile) {
		fwrite(pstrBuffer, 1, iSize, pFile);
		fclose(pFile);
	}
}

/**
 * Extract the files from the nbl archive.
 */

int extract(unsigned int uOptions, char* pstrBuffer, struct bf_ctx* pCtx, char* pstrDestPath)
{
	char* pstrData;
	int iIsCompressed = 0;
	int iDataPos;
	int iTMLLPos;

	if (pCtx) {
		nbl_decrypt_headers(pCtx, pstrBuffer, NBL_HEADER_CHUNKS);

		if (uOptions & OPTION_DEBUG)
			debug_save_buffer("decrypt-headers.dbg", pstrBuffer, NBL_READ_UINT(pstrBuffer, NBL_HEADER_SIZE));
	}

	iDataPos = nbl_get_data_pos(pstrBuffer);
	iIsCompressed = nbl_is_compressed(pstrBuffer);

	if (uOptions & OPTION_VERBOSE)
		printf("data=%x, compressed=%x, encrypted=%x\n", iDataPos, iIsCompressed, pCtx != NULL);

	if (iIsCompressed) {
		if (pCtx)
			nbl_decrypt_buffer(pCtx, pstrBuffer + iDataPos, NBL_READ_UINT(pstrBuffer, NBL_HEADER_COMPRESSED_DATA_SIZE));

		if (uOptions & OPTION_DEBUG)
			debug_save_buffer("comp-decrypt.dbg", pstrBuffer + iDataPos, NBL_READ_UINT(pstrBuffer, NBL_HEADER_COMPRESSED_DATA_SIZE));

		pstrData = malloc(NBL_READ_UINT(pstrBuffer, NBL_HEADER_DATA_SIZE));
		if (pstrData == NULL)
			return -3;

		nbl_decompress(
			pstrBuffer + iDataPos,
			NBL_READ_UINT(pstrBuffer, NBL_HEADER_COMPRESSED_DATA_SIZE),
			pstrData,
			NBL_READ_UINT(pstrBuffer, NBL_HEADER_DATA_SIZE)
		);
		/* TODO: check the return value */
	} else {
		if (pCtx)
			nbl_decrypt_buffer(pCtx, pstrBuffer + iDataPos, NBL_READ_UINT(pstrBuffer, NBL_HEADER_DATA_SIZE));

		pstrData = pstrBuffer + iDataPos;
	}

	if (uOptions & OPTION_DEBUG)
		debug_save_buffer("decomp-decrypt.dbg", pstrData, NBL_READ_UINT(pstrBuffer, NBL_HEADER_DATA_SIZE));

	nbl_extract_all(pstrBuffer, pstrData, pstrDestPath);

	if (iIsCompressed)
		free(pstrData);

	/* TMLL part (incomplete) */

	if (!nbl_has_tmll(pstrBuffer))
		return 0;

	iTMLLPos = nbl_get_tmll_pos(pstrBuffer);
	if (uOptions & OPTION_VERBOSE)
		printf("TMLL section found at position 0x%x!\n", iTMLLPos);

	if (pCtx)
		nbl_decrypt_headers(pCtx, pstrBuffer + iTMLLPos, NBL_TMLL_HEADER_CHUNKS);

	iDataPos = nbl_get_data_pos(pstrBuffer + iTMLLPos);
	iIsCompressed = nbl_is_compressed(pstrBuffer + iTMLLPos);

	if (uOptions & OPTION_VERBOSE)
		printf("data=%x, compressed=%x, encrypted=%x\n", iDataPos, iIsCompressed, pCtx != NULL);

	if (iIsCompressed) {
		if (uOptions & OPTION_DEBUG)
			debug_save_buffer("tmll-comp-crypt.dbg", pstrBuffer + iTMLLPos, NBL_READ_UINT(pstrBuffer + iTMLLPos, NBL_HEADER_COMPRESSED_DATA_SIZE));

		if (pCtx)
			nbl_decrypt_buffer(pCtx, pstrBuffer + iTMLLPos + iDataPos, NBL_READ_UINT(pstrBuffer + iTMLLPos, NBL_HEADER_COMPRESSED_DATA_SIZE));

		if (uOptions & OPTION_DEBUG)
			debug_save_buffer("tmll-comp-decrypt.dbg", pstrBuffer + iTMLLPos, NBL_READ_UINT(pstrBuffer + iTMLLPos, NBL_HEADER_COMPRESSED_DATA_SIZE));

		/* TODO: find out the correct decompress algorithm for the TMLL chunk; disabled meanwhile */
		return 0;

		pstrData = malloc(NBL_READ_UINT(pstrBuffer + iTMLLPos, NBL_HEADER_DATA_SIZE));
		if (pstrData == NULL) {
			free(pstrBuffer);
			return -3;
		}

		nbl_decompress(
			pstrBuffer + iTMLLPos + iDataPos,
			NBL_READ_UINT(pstrBuffer + iTMLLPos, NBL_HEADER_COMPRESSED_DATA_SIZE),
			pstrData,
			NBL_READ_UINT(pstrBuffer + iTMLLPos, NBL_HEADER_DATA_SIZE)
		);
		/* TODO: check the return value */
	} else {
		if (pCtx)
			nbl_decrypt_buffer(pCtx, pstrBuffer + iTMLLPos + iDataPos, NBL_READ_UINT(pstrBuffer + iTMLLPos, NBL_HEADER_DATA_SIZE));

		pstrData = pstrBuffer + iTMLLPos + iDataPos;
	}

	if (uOptions & OPTION_DEBUG)
		debug_save_buffer("tmll-decomp-decrypt.dbg", pstrData, NBL_READ_UINT(pstrBuffer + iTMLLPos, NBL_HEADER_DATA_SIZE));

	nbl_extract_all(pstrBuffer + iTMLLPos, pstrData, pstrDestPath);

	if (iIsCompressed)
		free(pstrData);

	return 0;
}

/**
 * List the files inside the nbl archive.
 */

void list(unsigned int uOptions, char* pstrBuffer, struct bf_ctx* pCtx)
{
	int iTMLLPos;

	if (pCtx)
		nbl_decrypt_headers(pCtx, pstrBuffer, NBL_HEADER_CHUNKS);
	nbl_list_files(pstrBuffer, NBL_HEADER_CHUNKS);

	if (!nbl_has_tmll(pstrBuffer))
		return;

	iTMLLPos = nbl_get_tmll_pos(pstrBuffer);
	if (uOptions & OPTION_VERBOSE)
		printf("TMLL section found at position 0x%x!\n", iTMLLPos);

	if (pCtx)
		nbl_decrypt_headers(pCtx, pstrBuffer + iTMLLPos, NBL_TMLL_HEADER_CHUNKS);
	nbl_list_files(pstrBuffer + iTMLLPos, NBL_TMLL_HEADER_CHUNKS);
}

/**
 * Entry point.
 */

int main(int argc, char** argv)
{
	char* pstrBuffer = NULL;
	char* pstrDestPath = NULL;
	struct bf_ctx ctx;
	struct bf_ctx* pCtx = NULL;
	unsigned char key[4];
	unsigned int uOptions = 0;
	int i;
	int ret = 0;

	opterr = 0;
	while ((i = getopt(argc, argv, "do:tv")) != -1) {
		switch (i) {
			case 'd':
				uOptions |= OPTION_DEBUG;
				break;

			case 'o':
				pstrDestPath = optarg;
				break;

			case 't':
				uOptions |= OPTION_LIST;
				break;

			case 'v':
				uOptions |= OPTION_VERBOSE;
				break;

			case '?':
				if (optopt == 'o')
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint(optopt))
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
				return 1;

			default:
				abort();
		}
	}

	i = optind;

	if (i + 1 != argc) {
		fprintf(stderr, "Usage: %s [-d] [-v] [-t] [-o destpath] file.nbl\n", argv[0]);
		return 1;
	}

	pstrBuffer = nbl_load(argv[i]);
	if (pstrBuffer == NULL) {
		fprintf(stderr, "Error opening file %s\n", argv[i]);
		return -1;
	}

	if (NBL_READ_UINT(pstrBuffer, NBL_HEADER_KEY_SEED) == 0)
		pCtx = NULL;
	else {
		pCtx = &ctx;
		key[0] = *(unsigned char*)(pstrBuffer + NBL_HEADER_KEY_SEED + 3);
		key[1] = *(unsigned char*)(pstrBuffer + NBL_HEADER_KEY_SEED + 2);
		key[2] = *(unsigned char*)(pstrBuffer + NBL_HEADER_KEY_SEED + 1);
		key[3] = *(unsigned char*)(pstrBuffer + NBL_HEADER_KEY_SEED);
		bf_setkey(pCtx, key, 4);
	}

	if (uOptions & OPTION_LIST)
		list(uOptions, pstrBuffer, pCtx);
	else
		ret = extract(uOptions, pstrBuffer, pCtx, pstrDestPath);

	if (pstrBuffer)
		free(pstrBuffer);

	return ret;
}
