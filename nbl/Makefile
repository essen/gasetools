#	gasetools: a set of tools to manipulate SEGA games file formats
#	Copyright (C) 2010  Loic Hoguin
#
#	This file is part of gasetools.
#
#	gasetools is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	gasetools is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with gasetools.  If not, see <http://www.gnu.org/licenses/>.

all: clean
	cc -m64 -std=c99 -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-qual \
		-Wstrict-prototypes -Wmissing-prototypes -Werror -Wstrict-overflow=5 \
		-pedantic -O3 -D_GNU_SOURCE -o nbl main.c nbl.c fakefish.c

win: clean
	i586-mingw32msvc-cc -o nbl.exe -combine main.c nbl.c fakefish.c

clean:
	-rm nbl nbl.exe
