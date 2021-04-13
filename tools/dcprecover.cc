/*
    Copyright (C) 2018-2021 Carl Hetherington <cth@carlh.net>

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


#include "asset_factory.h"
#include "cpl.h"
#include "dcp.h"
#include "exceptions.h"
#include "reel_asset.h"
#include "warnings.h"
#include <getopt.h>
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <boost/filesystem.hpp>
#include <iostream>


using std::cerr;
using std::cout;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::vector;
using boost::optional;


static void
help (string n)
{
	cerr << "Syntax: " << n << " [OPTION] <DCP>]\n"
	     << "  -h, --help         show this help\n"
	     << "  -o, --output       output DCP directory\n";
}


void progress (float f)
{
	cout << (f * 100) << "%               \r";
}


int
main (int argc, char* argv[])
{
	dcp::init ();

	int option_index = 0;
	optional<boost::filesystem::path> output;
	while (true) {
		struct option long_options[] = {
			{ "help", no_argument, 0, 'h' },
			{ "output", required_argument, 0, 'o' },
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long (argc, argv, "ho:", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'h':
			help (argv[0]);
			exit (EXIT_SUCCESS);
		case 'o':
			output = optarg;
			break;
		}
	}

	if (optind >= argc) {
		help (argv[0]);
		exit (EXIT_FAILURE);
	}

	boost::filesystem::path dcp_dir = argv[optind];

	/* Try to read it and report errors */

	dcp::DCP dcp (dcp_dir);
	vector<dcp::VerificationNote> notes;
	try {
		dcp.read (&notes, true);
	} catch (dcp::ReadError& e) {
		cout << "Error:" <<  e.what() << "\n";
	}

	for (auto i: notes) {
		cout << "Error: " << dcp::note_to_string(i) << "\n";
	}

	/* Look for a CPL */

	shared_ptr<dcp::CPL> cpl;
	for (auto i: boost::filesystem::directory_iterator(dcp_dir)) {
		if (i.path().extension() == ".xml") {
			try {
				cpl = make_shared<dcp::CPL>(i.path());
			} catch (dcp::ReadError& e) {
				cout << "Error: " << e.what() << "\n";
			} catch (xmlpp::parse_error& e) {
				cout << "Error: " << e.what() << "\n";
			}
		}
	}

	if (cpl) {
		cout << "Got a CPL!\n";

		if (!output) {
			cerr << "No output directory specified.\n";
			exit(1);
		}

		/* Read all MXF assets */
		vector<shared_ptr<dcp::Asset>> assets;
		for (auto i: boost::filesystem::directory_iterator(dcp_dir)) {
			if (i.path().extension() == ".mxf") {
				try {
					auto asset = dcp::asset_factory(i.path(), true);
					asset->set_file (*output / i.path().filename());
					cout << "Hashing " << i.path().filename() << "\n";
					asset->hash (&progress);
					cout << "100%                     \n";
					assets.push_back (asset);
				} catch (dcp::ReadError& e) {
					cout << "Error: " << e.what() << "\n";
				}
			}
		}

		dcp::DCP fixed (*output);
		fixed.add (cpl);
		fixed.resolve_refs (assets);
		fixed.write_xml ();
		cout << "Fixed XML files written to " << output->string() << "\n";
	}

	return 0;
}
