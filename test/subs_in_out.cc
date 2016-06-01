/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

    This file is part of libdcp.

    libdcp is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    libdcp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdcp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "interop_subtitle_asset.h"
#include "smpte_subtitle_asset.h"
#include <iostream>

using namespace std;

int
main (int argc, char* argv[])
{
	if (argc != 2) {
		cerr << "Syntax: " << argv[0] << " <subtitle file>\n";
		exit (EXIT_FAILURE);
	}

	try {
		dcp::InteropSubtitleAsset sc (argv[1]);
		cout << sc.xml_as_string ();
	} catch (exception& e) {
		cerr << "Could not load as interop: " << e.what() << "\n";
		try {
			dcp::SMPTESubtitleAsset sc (argv[1]);
			cout << sc.xml_as_string();
		} catch (exception& e) {
			cerr << "Could not load as SMPTE (" << e.what() << ")\n";
		}
	}
	return 0;
}
