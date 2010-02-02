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

void debug_save_buffer(char* pstrFilename, char* pstrBuffer, int iSize)
{
	FILE* pFile;

	pFile = fopen(pstrFilename, "wb");
	if (pFile) {
		fwrite(pstrBuffer, 1, iSize, pFile);
		fclose(pFile);
	}
}

int main(int argc, char** argv)
{
	char* pstrBuffer;
	char* pstrData;
	char* pstrDestPath = NULL;
	int iIsCompressed = 0;
	int iDebug = 0;
	int iListOnly = 0;
	int iVerbose = 0;
	int iDataPos;
	int i;
	unsigned int* puKey;
	unsigned int uSeed;

	opterr = 0;
	while ((i = getopt(argc, argv, "do:tv")) != -1) {
		switch (i) {
			case 'd':
				iDebug = 1;
				break;

			case 'o':
				pstrDestPath = optarg;
				break;

			case 't':
				iListOnly = 1;
				break;

			case 'v':
				iVerbose = 1;
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
		fprintf(stderr, "Usage: %s [-o destpath] file.nbl\n", argv[0]);
		return 2;
	}

	pstrBuffer = nbl_load(argv[i]);
	if (pstrBuffer == NULL) {
		fprintf(stderr, "Error opening file %s\n", argv[i]);
		return -1;
	}

	uSeed = NBL_READ_UINT(pstrBuffer, NBL_HEADER_KEY_SEED);
	if (uSeed == 0)
		puKey = NULL;
	else {
		puKey = nbl_build_key(uSeed);
		if (puKey == NULL) {
			free(pstrBuffer);
			return -2;
		}

		nbl_decrypt_headers(pstrBuffer, puKey);
	}

	if (iListOnly == 1) {
		nbl_list_files(pstrBuffer);
		goto main_ret;
	}

	iDataPos = nbl_get_data_pos(pstrBuffer);
	iIsCompressed = nbl_is_compressed(pstrBuffer);

	if (iVerbose) {
		printf("data=%x, compressed=%x, encrypted=%x\n", iDataPos, iIsCompressed, uSeed != 0);
	}

	if (iIsCompressed) {
		if (puKey) {
			nbl_decrypt_buffer(pstrBuffer + iDataPos, puKey, NBL_READ_UINT(pstrBuffer, NBL_HEADER_COMPRESSED_DATA_SIZE));
		}

		if (iDebug)
			debug_save_buffer("comp-decrypt.dbg", pstrBuffer, NBL_READ_UINT(pstrBuffer, NBL_HEADER_COMPRESSED_DATA_SIZE));

		pstrData = malloc(NBL_READ_UINT(pstrBuffer, NBL_HEADER_DATA_SIZE));
		if (pstrData == NULL) {
			free(puKey);
			free(pstrBuffer);
			return -3;
		}

		nbl_decompress(
			pstrBuffer + iDataPos,
			NBL_READ_UINT(pstrBuffer, NBL_HEADER_COMPRESSED_DATA_SIZE),
			pstrData,
			NBL_READ_UINT(pstrBuffer, NBL_HEADER_DATA_SIZE)
		);
		/* TODO: check the return value */
	} else {
		if (puKey) {
			nbl_decrypt_buffer(pstrBuffer + iDataPos, puKey, NBL_READ_UINT(pstrBuffer, NBL_HEADER_DATA_SIZE));
		}

		pstrData = pstrBuffer + iDataPos;
	}

	if (iDebug)
		debug_save_buffer("decomp-decrypt.dbg", pstrData, NBL_READ_UINT(pstrBuffer, NBL_HEADER_DATA_SIZE));

	nbl_extract_all(pstrBuffer, pstrData, pstrDestPath);

main_ret:
	if (iIsCompressed)
		free(pstrData);
	if (puKey)
		free(puKey);
	free(pstrBuffer);

	return 0;
}
