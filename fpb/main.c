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
 * We're just going through every NBL_CHUNK_PADDING_SIZE and extract all the nbl files we find.
 */

int main(int argc, char** argv)
{
	FILE* pFile;
	FILE* pOut;
	char* pstrBuffer;
	char pstrFilename[32];
	int iCurrentPos = 0;
	int i, iRead, iTotal;
	int iNMLL = 0, iTMLL = 0;
	int aFiles[10000];
	unsigned int iTmp;

	if (2 != argc) {
		fprintf(stderr, "Usage: %s file.fpb\n", argv[0]);
		return 2;
	}

	pFile = fopen(argv[1], "rb");
	if (pFile == NULL)
		return -1;

	iTotal = 0;
	while (1) {
		fseek(pFile, iCurrentPos, SEEK_SET);
		iRead = fread(&iTmp, sizeof(unsigned int), 1, pFile);

		if (iRead <= 0)
			break;

		if (iTmp == NBL_ID_NMLL || iTmp == NBL_ID_NMLB || iTmp == NBL_ID_TMLL) {
			aFiles[iTotal] = iCurrentPos;
			iTotal++;
		}

		iCurrentPos += NBL_CHUNK_PADDING_SIZE;
	}

	fseek(pFile, 0, SEEK_END);
	aFiles[iTotal] = ftell(pFile);

	for (i = 0; i < iTotal; i++) {
		fseek(pFile, aFiles[i], SEEK_SET);
		pstrBuffer = malloc(aFiles[i + 1] - aFiles[i]);
		iRead = fread(pstrBuffer, 1, aFiles[i + 1] - aFiles[i], pFile);
		if (iRead > 0) {
			if (NBL_READ_UINT(pstrBuffer, NBL_HEADER_IDENTIFIER) == NBL_ID_TMLL)
				sprintf(pstrFilename, "tmll-%d-new-format.nbl", iTMLL++);
			else if (pstrBuffer[5])
				sprintf(pstrFilename, "nmll-%d-new-format.nbl", iNMLL++);
			else
				sprintf(pstrFilename, "nmll-%d.nbl", iNMLL++);
			pOut = fopen(pstrFilename, "wb");
			fwrite(pstrBuffer, 1, aFiles[i + 1] - aFiles[i], pOut);
			fclose(pOut);
		}

		free(pstrBuffer);
	}

	fclose(pFile);

	return 0;
}
