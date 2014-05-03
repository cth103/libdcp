/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <iostream>
#include <boost/filesystem.hpp>
#include <getopt.h>
#include "dcp.h"
#include "exceptions.h"
#include "common.h"

using namespace std;
using namespace boost;
using namespace dcp;

static bool verbose = false;

static void
help (string n)
{
	cerr << "Syntax: " << n << " [OPTION] <DCP> <DCP>\n"
	     << "  -V, --version                show libdcp version\n"
	     << "  -h, --help                   show this help\n"
	     << "  -v, --verbose                be verbose\n"
	     << "  -n, --names                  allow differing MXF names\n"
	     << "  -m, --mean-pixel             maximum allowed mean pixel error (default 5)\n"
	     << "  -s, --std-dev-pixel          maximum allowed standard deviation of pixel error (default 5)\n"
	     << "  -k, --keep-going             carry on in the event of errors, if possible\n"
	     << "      --ignore-missing-assets  ignore missing asset files\n"
	     << "\n"
	     << "The <DCP>s are the DCP directories to compare.\n"
	     << "Comparison is of metadata and content, ignoring timestamps\n"
	     << "and differing UUIDs.\n";
}

void
note (NoteType t, string n)
{
	if (t == ERROR || verbose) {
		cout << " " << n << "\n";
	}
}

DCP *
load_dcp (boost::filesystem::path path, bool keep_going, bool ignore_missing_assets)
{
	DCP* dcp = 0;
	try {
		dcp = new DCP (path);
		DCP::ReadErrors errors;
		dcp->read (keep_going, &errors);
		filter_errors (errors, ignore_missing_assets);
		for (DCP::ReadErrors::const_iterator i = errors.begin(); i != errors.end(); ++i) {
			cerr << (*i)->what() << "\n";
		}
	} catch (FileError& e) {
		cerr << "Could not read DCP " << path.string() << "; " << e.what() << " " << e.filename() << "\n";
		exit (EXIT_FAILURE);
	}

	return dcp;
}

int
main (int argc, char* argv[])
{
	EqualityOptions options;
	options.max_mean_pixel_error = 5;
	options.max_std_dev_pixel_error = 5;
	bool keep_going = false;
	bool ignore_missing_assets = false;
	
	int option_index = 0;
	while (1) {
		static struct option long_options[] = {
			{ "version", no_argument, 0, 'V'},
			{ "help", no_argument, 0, 'h'},
			{ "verbose", no_argument, 0, 'v'},
			{ "names", no_argument, 0, 'n'},
			{ "mean-pixel", required_argument, 0, 'm'},
			{ "std-dev-pixel", required_argument, 0, 's'},
			{ "keep-going", no_argument, 0, 'k'},
			{ "ignore-missing-assets", no_argument, 0, 'A'},
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long (argc, argv, "Vhvnm:s:kA", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'V':
			cout << "dcpdiff version " << LIBDCP_VERSION << "\n";
			exit (EXIT_SUCCESS);
		case 'h':
			help (argv[0]);
			exit (EXIT_SUCCESS);
		case 'v':
			verbose = true;
			break;
		case 'n':
			options.mxf_names_can_differ = true;
			break;
		case 'm':
			options.max_mean_pixel_error = atof (optarg);
			break;
		case 's':
			options.max_std_dev_pixel_error = atof (optarg);
			break;
		case 'k':
			keep_going = true;
			break;
		case 'A':
			ignore_missing_assets = true;
			break;
		}
	}

	if (argc <= optind || argc > (optind + 2)) {
		help (argv[0]);
		exit (EXIT_FAILURE);
	}

	if (!filesystem::exists (argv[optind])) {
		cerr << argv[0] << ": DCP " << argv[optind] << " not found.\n";
		exit (EXIT_FAILURE);
	}

	if (!filesystem::exists (argv[optind + 1])) {
		cerr << argv[0] << ": DCP " << argv[optind + 1] << " not found.\n";
		exit (EXIT_FAILURE);
	}

	DCP* a = load_dcp (argv[optind], keep_going, ignore_missing_assets);
	DCP* b = load_dcp (argv[optind + 1], keep_going, ignore_missing_assets);

	/* I think this is just below the LSB at 16-bits (ie the 8th most significant bit at 24-bit) */
	options.max_audio_sample_error = 255;

	bool const equals = a->equals (*b, options, boost::bind (note, _1, _2));

	if (equals) {
		exit (EXIT_SUCCESS);
	}

	exit (EXIT_FAILURE);
}
