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

#ifndef __GASETOOLS_NBL_H__
#define __GASETOOLS_NBL_H__

/* Key information */

#define NBL_COMMON_BASE_HEADER 0x0085A080
#define NBL_COMMON_BASE_FILENAME "cbkey.dat"
#define NBL_KEY_SIZE 4172 /* (1 + 18 + 1024) * 4 */

/* Filetype identifiers */

#define NBL_ID_NMLL	0x4C4C4D4E /* Low endian. */
#define NBL_ID_NMLB	0x424C4D4E /* Big endian. Currently unsupported. */

/* Chunk identifiers - probably irrelevant */

#define NBL_ID_STD	0x00445453
#define NBL_ID_REL	0x004C4552
#define NBL_ID_XNCP	0x50434E58
#define NBL_ID_XNT	0x00544E58

/* To be confirmed. Probably whether it's compressed. */

#define NBL_ID_NXIF	0x4649584E

/* Positions */

#define NBL_HEADER_IDENTIFIER	0x00
/* Unknown 0x04. */
#define NBL_HEADER_SIZE			0x08
#define NBL_HEADER_NB_CHUNKS	0x0C
#define NBL_HEADER_DATA_SIZE			0x10 /* Uncompressed. */
#define NBL_HEADER_COMPRESSED_DATA_SIZE	0x14 /* Compressed; if 0 then the file isn't compressed. */
/* Unknown 0x18. */
#define NBL_HEADER_KEY_SEED		0x1C
#define NBL_HEADER_CHUNKS		0x30

/* Positions from NBL_HEADER_CHUNKS */

#define NBL_CHUNK_IDENTIFIER		0x00
#define NBL_CHUNK_HEADER_SIZE		0x04 /* Always 0x60 so far. */
#define NBL_CHUNK_CRYPTED_HEADER	0x10
#define NBL_CHUNK_CRYPTED_SIZE		0x30
/* Encrypted part */
#define NBL_CHUNK_FILENAME			0x10
#define NBL_CHUNK_FILENAME_SIZE		0x20
#define NBL_CHUNK_W 0x30
#define NBL_CHUNK_X 0x34
#define NBL_CHUNK_Y 0x38
#define NBL_CHUNK_Z 0x3C
/* After the encrypted part. */
#define NBL_CHUNK_A 0x40 /* NBL_ID_NXIF or empty. */
#define NBL_CHUNK_B 0x44 /* Probably safely ignored. */
#define NBL_CHUNK_C 0x48 /* Probably safely ignored. */
#define NBL_CHUNK_D 0x4C /* Probably safely ignored. */
#define NBL_CHUNK_E 0x50
#define NBL_CHUNK_F 0x54
#define NBL_CHUNK_G 0x58
#define NBL_CHUNK_H 0x5C /* Probably safely ignored. */

/* Padding */

#define NBL_CHUNK_PADDING_SIZE 0x800

/* Identification and loading */

int nbl_is_nmll(char* pstrBuffer);
int nbl_get_data_pos(char* pstrBuffer);
char* nbl_load(char* pstrFilename);

#define NBL_READ_INT(buf, pos) (*((int*)(buf + pos)))
#define NBL_READ_UINT(buf, pos) (*((unsigned int*)(buf + pos)))

/* Decryption */

unsigned int* nbl_build_key(unsigned int uSeed);
void nbl_decrypt_buffer(char* pstrBuffer, unsigned int* puKey, int iSize);
void nbl_decrypt_headers(char* pstrBuffer, unsigned int* puKey);

/* Decompression */

int nbl_is_compressed(char* pstrBuffer);
int nbl_decompress(char* pstrSrc, int iSrcSize, char* pstrDest, int iDestSize);

/* List and extract contents */

void nbl_list_files(char* pstrBuffer);
void nbl_extract_all(char* pstrBuffer, char* pstrData, char* pstrDestPath);

#endif /* __GASETOOLS_NBL_H__ */
