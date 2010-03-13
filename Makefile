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
	cd afs && make
	cd exp && make
	cd fpb && make
	cd nbl && make
	-mkdir build
	cp afs/afs build
	cp exp/exp build
	cp fpb/fpb build
	cp nbl/cbkey.dat build
	cp nbl/nbl build
	cp docs/* build
	cp scripts/* build

win: clean
	cd afs && make win
	cd exp && make win
	cd fpb && make win
	cd nbl && make win
	-mkdir build
	cp afs/afs.exe build
	cp exp/exp.exe build
	cp fpb/fpb.exe build
	cp nbl/cbkey.dat build
	cp nbl/nbl.exe build
	cp docs/* build
	cp scripts/* build

clean:
	cd afs && make clean
	cd exp && make clean
	cd fpb && make clean
	cd nbl && make clean
	-rm build/*
