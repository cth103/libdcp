/*
    Copyright (C) 2017 Carl Hetherington <cth@carlh.net>

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

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
*/

#include "encrypted_kdm.h"
#include "decrypted_kdm.h"
#include "util.h"
#include "exceptions.h"
#include <getopt.h>

using std::string;
using std::cout;
using std::cerr;
using boost::optional;

static void
help (string n)
{
	cerr << "Syntax: " << n << " [OPTION] <certificate .pem>]\n"
	     << "  -h, --help         show this help\n";
}

int
main (int argc, char* argv[])
{
	dcp::init ();

	int option_index = 0;
	while (true) {
		struct option long_options[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long (argc, argv, "hp:", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'h':
			help (argv[0]);
			exit (EXIT_SUCCESS);
		}
	}

	if (optind >= argc) {
		help (argv[0]);
		exit (EXIT_FAILURE);
	}

	try {
		cout << dcp::Certificate(dcp::file_to_string(argv[optind])).thumbprint() << "\n";
	} catch (boost::filesystem::filesystem_error& e) {
		cerr << e.what() << "\n";
		return -1;
	}

	return 0;
}
