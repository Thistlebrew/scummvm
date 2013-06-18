/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

/*
 * This code is based on the original source code of Lord Avalot d'Argent version 1.3.
 * Copyright (c) 1994-1995 Mike, Mark and Thomas Thurman.
 */

namespace Avalanche {

struct sezheader {
	array<1, 2, char> initials;   /* should be "TT" */
	word gamecode;
	word revision; /* as 3- or 4-digit code (eg v1.00 = 100) */
	longint chains; /* number of scroll chains */
	longint size; /* total size of all scroll chains */
};

const string crlf = string('\15') + '\12';
const string tabs = string('\11') + '\11' + '\11' + '\11' + '\11' + '\11' + '\11';
const char eof_ = '\32';

untyped_file sez;
sezheader header;
string x;
char check;

int main(int argc, const char *argv[]) {
	pio_initialize(argc, argv);
	;
	fillchar(x, sizeof(x), '\261');
	x = string("This is a Sez file for an Avvy game, and its contents are subject") + crlf +
	    "to copyright. Have fun with the game!" + crlf + crlf + tabs + "tt" + crlf + crlf +
	    "[Lord Avalot D'Argent]" + crlf + crlf + eof_ +
	    crlf + crlf + "Thomas was here!";
	{
		;
		header.initials = "TT";
		header.gamecode = 2; /* code for Avalot */
		header.revision = 100; /* version 1.00 */
		header.chains = 0; /* no chains */
		header.size = 0; /* empty! */
	}
	check = '\261';
	assign(sez, "avalot.sez");
	rewrite(sez, 1);
	blockwrite(sez, x[1], 255);
	blockwrite(sez, header, sizeof(header));
	blockwrite(sez, check, 1);
	x = string('\0') + '\0' + '\0' + "Thomas was here, too!" + crlf + crlf + "Good luck...";
	blockwrite(sez, x[1], 39); /* footer */
	close(sez);
	return EXIT_SUCCESS;
}


} // End of namespace Avalanche.