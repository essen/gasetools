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
#include "afs.h"

int main(int argc, char** argv)
{
	char* pstrBuffer;
	char* pstrDestPath = NULL;
	int iListOnly = 0;
	int i;

	opterr = 0;
	while ((i = getopt(argc, argv, "o:t")) != -1) {
		switch (i) {
			case 'o':
				pstrDestPath = optarg;
				break;

			case 't':
				iListOnly = 1;
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
		fprintf(stderr, "Usage: %s [-o destpath] file.afs\n", argv[0]);
		return 2;
	}

	pstrBuffer = afs_load(argv[i]);
	if (pstrBuffer == NULL)
		return -1;

	if (iListOnly == 1) {
		afs_list_files(pstrBuffer);
		goto main_ret;
	}

	afs_extract_all(pstrBuffer, pstrDestPath);

main_ret:
	free(pstrBuffer);

	return 0;
}
