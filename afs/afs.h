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

#ifndef __GASETOOLS_AFS_H__
#define __GASETOOLS_AFS_H__

/* Filetype identifier */

#define AFS_ID 0x00534641

/* Positions */

#define AFS_HEADER_IDENTIFIER	0x00
#define AFS_HEADER_NB_CHUNKS	0x04
#define AFS_HEADER_CHUNKS		0x08
#define AFS_CHUNK_HEADER_SIZE	0x08
#define AFS_CHUNK_POS			0x00
#define AFS_CHUNK_SIZE			0x04
#define AFS_CHUNK_FILENAME_SIZE	0x20

/* Identification and loading */

char* afs_load(char* pstrFilename);

/* List and extract contents */

void afs_list_files(char* pstrBuffer);
void afs_extract_all(char* pstrBuffer, char* pstrDestPath);

#endif /* __GASETOOLS_AFS_H__ */
