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

#define OPTION_PACK		0x1
#define OPTION_LIST		0x2
#define OPTION_DEBUG	0x4
#define OPTION_VERBOSE	0x8

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

int extract(unsigned int uOptions, char* pstrBuffer, unsigned int* puKey, char* pstrDestPath)
{
	char* pstrData;
	int iIsCompressed = 0;
	int iDataPos;
	int iTMLLPos;

	if (puKey)
		nbl_decrypt_headers(pstrBuffer, puKey, NBL_HEADER_CHUNKS);

	iDataPos = nbl_get_data_pos(pstrBuffer);
	iIsCompressed = nbl_is_compressed(pstrBuffer);

	if (uOptions & OPTION_VERBOSE)
		printf("data=%x, compressed=%x, encrypted=%x\n", iDataPos, iIsCompressed, puKey != NULL);

	if (iIsCompressed) {
		if (puKey)
			nbl_decrypt_buffer(pstrBuffer + iDataPos, puKey, NBL_READ_UINT(pstrBuffer, NBL_HEADER_COMPRESSED_DATA_SIZE));

		if (uOptions & OPTION_DEBUG)
			debug_save_buffer("comp-decrypt.dbg", pstrBuffer, NBL_READ_UINT(pstrBuffer, NBL_HEADER_COMPRESSED_DATA_SIZE));

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
		if (puKey)
			nbl_decrypt_buffer(pstrBuffer + iDataPos, puKey, NBL_READ_UINT(pstrBuffer, NBL_HEADER_DATA_SIZE));

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

	if (puKey)
		nbl_decrypt_headers(pstrBuffer + iTMLLPos, puKey, NBL_TMLL_HEADER_CHUNKS);

	iDataPos = nbl_get_data_pos(pstrBuffer + iTMLLPos);
	iIsCompressed = nbl_is_compressed(pstrBuffer + iTMLLPos);

	if (uOptions & OPTION_VERBOSE)
		printf("data=%x, compressed=%x, encrypted=%x\n", iDataPos, iIsCompressed, puKey != NULL);

	if (iIsCompressed) {
		if (puKey)
			nbl_decrypt_buffer(pstrBuffer + iTMLLPos + iDataPos, puKey, NBL_READ_UINT(pstrBuffer + iTMLLPos, NBL_HEADER_COMPRESSED_DATA_SIZE));

		if (uOptions & OPTION_DEBUG)
			debug_save_buffer("tmll-comp-decrypt.dbg", pstrBuffer + iTMLLPos, NBL_READ_UINT(pstrBuffer + iTMLLPos, NBL_HEADER_COMPRESSED_DATA_SIZE));

		/* TODO: find out the correct decompress algorithm for the TMLL chunk; disabled meanwhile */
		return 0;

		pstrData = malloc(NBL_READ_UINT(pstrBuffer + iTMLLPos, NBL_HEADER_DATA_SIZE));
		if (pstrData == NULL) {
			free(puKey);
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
		if (puKey)
			nbl_decrypt_buffer(pstrBuffer + iTMLLPos + iDataPos, puKey, NBL_READ_UINT(pstrBuffer + iTMLLPos, NBL_HEADER_DATA_SIZE));

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

void list(unsigned int uOptions, char* pstrBuffer, unsigned int* puKey)
{
	int iTMLLPos;

	if (puKey)
		nbl_decrypt_headers(pstrBuffer, puKey, NBL_HEADER_CHUNKS);
	nbl_list_files(pstrBuffer, NBL_HEADER_CHUNKS);

	if (!nbl_has_tmll(pstrBuffer))
		return;

	iTMLLPos = nbl_get_tmll_pos(pstrBuffer);
	if (uOptions & OPTION_VERBOSE)
		printf("TMLL section found at position 0x%x!\n", iTMLLPos);

	if (puKey)
		nbl_decrypt_headers(pstrBuffer + iTMLLPos, puKey, NBL_TMLL_HEADER_CHUNKS);
	nbl_list_files(pstrBuffer + iTMLLPos, NBL_TMLL_HEADER_CHUNKS);
}

/**
 * Entry point.
 */

int main(int argc, char** argv)
{
	char* pstrBuffer = NULL;
	char* pstrDestPath = NULL;
	unsigned int* puKey = NULL;
	unsigned int uSeed;
	unsigned int uOptions = 0;
	int i;
	int ret = 0;

	opterr = 0;
	while ((i = getopt(argc, argv, "c:do:tv")) != -1) {
		switch (i) {
			case 'c':
				uOptions |= OPTION_PACK;
				pstrDestPath = optarg;
				break;

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

	if (uOptions & OPTION_PACK) {
		if (uOptions & OPTION_VERBOSE)
			printf("dest=%s, nbfiles=%d\n", pstrDestPath, argc - i);

		/* TODO: the packer is broken, continue work on this after figuring out TMLL files */
		nbl_pack(pstrDestPath, &argv[i], argc - i);
	} else {
		if (i + 1 != argc) {
			fprintf(stderr, "Usage: %s [-o destpath] file.nbl\n", argv[0]);
			return 1;
		}

		pstrBuffer = nbl_load(argv[i]);
		if (pstrBuffer == NULL) {
			fprintf(stderr, "Error opening file %s\n", argv[i]);
			return -1;
		}

		uSeed = NBL_READ_UINT(pstrBuffer, NBL_HEADER_KEY_SEED);
		if (uSeed != 0) {
			puKey = nbl_build_key(uSeed);
			if (puKey == NULL) {
				free(pstrBuffer);
				return -2;
			}
		}

		if (uOptions & OPTION_LIST)
			list(uOptions, pstrBuffer, puKey);
		else
			ret = extract(uOptions, pstrBuffer, puKey, pstrDestPath);
	}

	if (puKey)
		free(puKey);
	if (pstrBuffer)
		free(pstrBuffer);

	return ret;
}
