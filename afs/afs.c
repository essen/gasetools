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
#include "afs.h"

#define AFS_READ_INT(buf, pos) (*((int*)(buf + pos)))
#define AFS_READ_UINT(buf, pos) (*((unsigned int*)(buf + pos)))

/**
 * Open a .afs file and check its identifier for validity.
 * Returns a new pointer with the file contents.
 */

char* afs_load(char* pstrFilename)
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

	if (AFS_READ_UINT(pstrBuffer, AFS_HEADER_IDENTIFIER) != AFS_ID) {
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
 * Return the position of the filenames list.
 */

static inline int afs_get_filenames_pos(char* pstrBuffer)
{
	return AFS_READ_INT(pstrBuffer, AFS_HEADER_CHUNKS + AFS_READ_INT(pstrBuffer, AFS_HEADER_NB_CHUNKS) * AFS_CHUNK_HEADER_SIZE);
}

/**
 * List the files from the decrypted headers.
 */

void afs_list_files(char* pstrBuffer)
{
	int i, iFilenamesPos, iNbChunks;

	iFilenamesPos = afs_get_filenames_pos(pstrBuffer);
	iNbChunks = AFS_READ_INT(pstrBuffer, AFS_HEADER_NB_CHUNKS);

	for (i = 0; i < iNbChunks; i++)
		printf("%s\n", pstrBuffer + iFilenamesPos + i * 0x30);
}

/**
 * Extract all the files from the data.
 */

void afs_extract_all(char* pstrBuffer, char* pstrDestPath)
{
	int i, iFilenamesPos, iNbChunks, iLen;
	FILE* pFile;
	char* pstrFilename;

	if (pstrDestPath == NULL) {
		iLen = 0;
		pstrFilename = malloc(iLen + AFS_CHUNK_FILENAME_SIZE + 1);
	} else {
		iLen = strlen(pstrDestPath);
		pstrFilename = malloc(iLen + AFS_CHUNK_FILENAME_SIZE + 1);
		/* TODO: check return value */
		strcpy(pstrFilename, pstrDestPath);
		if (pstrDestPath[iLen - 1] != '/' && pstrDestPath[iLen - 1] != '\\')
			pstrFilename[iLen++] = '/';
	}

	iFilenamesPos = afs_get_filenames_pos(pstrBuffer);
	iNbChunks = AFS_READ_INT(pstrBuffer, AFS_HEADER_NB_CHUNKS);

	for (i = 0; i < iNbChunks; i++) {
		strncpy(pstrFilename + iLen, pstrBuffer + iFilenamesPos + i * 0x30, AFS_CHUNK_FILENAME_SIZE);

		pFile = fopen(pstrFilename, "wb");
		if (pFile) {
			fwrite(pstrBuffer + AFS_READ_UINT(pstrBuffer, AFS_HEADER_CHUNKS + i * AFS_CHUNK_HEADER_SIZE + AFS_CHUNK_POS),
				1, AFS_READ_UINT(pstrBuffer, AFS_HEADER_CHUNKS + i * AFS_CHUNK_HEADER_SIZE + AFS_CHUNK_SIZE), pFile);
			fclose(pFile);
		}
	}

	free(pstrFilename);
}
