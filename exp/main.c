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
#include "../nbl/nbl.h"

/**
 * Expand the given file using nbl_decompress.
 */

int main(int argc, char** argv)
{
	FILE* pFile;
	char* pstrCmp;
	char* pstrExp;
	char pstrFilename[FILENAME_MAX];
	int iCmpSize, iExpSize, iRead;

	if (2 != argc) {
		fprintf(stderr, "Usage: %s file\n", argv[0]);
		return 2;
	}

	pFile = fopen(argv[1], "rb");
	if (pFile == NULL)
		return -1;

	iRead = fread(&iExpSize, sizeof(unsigned int), 1, pFile);
	/* TODO: iRead */
	iRead = fread(&iCmpSize, sizeof(unsigned int), 1, pFile);
	/* TODO: iRead */

	pstrExp = malloc(iExpSize);
	/* TODO */
	pstrCmp = malloc(iCmpSize);
	/* TODO */

	fseek(pFile, 0x1C, SEEK_SET);
	iRead = fread(pstrCmp, 1, iCmpSize, pFile);
	/* TODO */
	fclose(pFile);

	nbl_decompress(pstrCmp, iCmpSize, pstrExp, iExpSize);

	sprintf(pstrFilename, "%s.exp", argv[1]);
	pFile = fopen(pstrFilename, "wb");
	fwrite(pstrExp, 1, iExpSize, pFile);
	/* TODO */
	fclose(pFile);

	free(pstrCmp);
	free(pstrExp);

	return 0;
}
