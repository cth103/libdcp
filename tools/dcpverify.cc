/*
    Copyright (C) 2020 Carl Hetherington <cth@carlh.net>

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


#include "common.h"
#include "compose.hpp"
#include "filesystem.h"
#include "raw_convert.h"
#include "verify.h"
#include "version.h"
#include <boost/bind/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <getopt.h>
#include <iostream>
#include <cstdlib>


using std::cerr;
using std::cout;
using std::list;
using std::string;
using std::vector;
using boost::bind;
using boost::optional;
#if BOOST_VERSION >= 106100
using namespace boost::placeholders;
#endif


static void
help (string n)
{
	cerr << "Syntax: " << n << " [OPTION] <DCP>\n"
	     << "  -V, --version                                show libdcp version\n"
	     << "  -h, --help                                   show this help\n"
	     << "  --ignore-missing-assets                      don't give errors about missing assets\n"
	     << "  --ignore-bv21-smpte                          don't give the SMPTE Bv2.1 error about a DCP not being SMPTE\n"
	     << "  --no-asset-hash-check                        don't check asset hashes\n"
	     << "  --asset-hash-check-maximum-size <size-in-MB> only check hashes for assets smaller than this size (in MB)\n"
	     << "  -q, --quiet                                  don't report progress\n";
}


int
main (int argc, char* argv[])
{
	dcp::init ();

	bool ignore_missing_assets = false;
	bool ignore_bv21_smpte = false;
	bool quiet = false;

	dcp::VerificationOptions verification_options;

	int option_index = 0;
	while (true) {
		static struct option long_options[] = {
			{ "version", no_argument, 0, 'V' },
			{ "help", no_argument, 0, 'h' },
			{ "ignore-missing-assets", no_argument, 0, 'A' },
			{ "ignore-bv21-smpte", no_argument, 0, 'B' },
			{ "no-asset-hash-check", no_argument, 0, 'C' },
			{ "asset-hash-check-maximum-size", required_argument, 0, 'D' },
			{ "quiet", no_argument, 0, 'q' },
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long (argc, argv, "VhABCD:q", long_options, &option_index);

		if (c == -1) {
			break;
		} else if (c == '?' || c == ':') {
			exit(EXIT_FAILURE);
		}

		switch (c) {
		case 'V':
			cout << "dcpverify version " << dcp::version << "\n";
			exit (EXIT_SUCCESS);
		case 'h':
			help(boost::filesystem::path(argv[0]).filename().string());
			exit (EXIT_SUCCESS);
		case 'A':
			ignore_missing_assets = true;
			break;
		case 'B':
			ignore_bv21_smpte = true;
			break;
		case 'C':
			verification_options.check_asset_hashes = false;
			break;
		case 'D':
			verification_options.maximum_asset_size_for_hash_check = dcp::raw_convert<int>(optarg) * 1000000LL;
			break;
		case 'q':
			quiet = true;
			break;
		}
	}

	if (argc <= optind) {
		help (argv[0]);
		exit (EXIT_FAILURE);
	}

	if (!dcp::filesystem::exists(argv[optind])) {
		cerr << argv[0] << ": DCP " << argv[optind] << " not found.\n";
		exit (EXIT_FAILURE);
	}

	auto stage = [quiet](string s, optional<boost::filesystem::path> path) {
		if (quiet) {
			return;
		}

		if (path) {
			cout << s << ": " << path->string() << "\n";
		} else {
			cout << s << "\n";
		}
	};

	auto progress = [quiet](float amount) {
		if (quiet) {
			return;
		}
		int const width = 60;
		int const index = std::rint(amount * width);
		cout << "[";
		for (int i = 0; i < width; ++i) {
			if (i < index) {
				std::cout << "=";
			} else if (i == index) {
				std::cout << ">";
			} else {
				std::cout << " ";
			}
		}
		cout << "] " << std::rint(amount * 100) << "%\r";
		cout.flush();
	};

	vector<boost::filesystem::path> directories;
	directories.push_back (argv[optind]);
	auto notes = dcp::verify(directories, {}, stage, progress, verification_options);
	dcp::filter_notes (notes, ignore_missing_assets);

	if (!quiet) {
		cout << "\n";
	}

	bool failed = false;
	bool bv21_failed = false;
	bool warned = false;
	for (auto i: notes) {
		if (ignore_bv21_smpte && i.code() == dcp::VerificationNote::Code::INVALID_STANDARD) {
			continue;
		}
		switch (i.type()) {
		case dcp::VerificationNote::Type::ERROR:
			cout << "Error: " << note_to_string(i) << "\n";
			failed = true;
			break;
		case dcp::VerificationNote::Type::BV21_ERROR:
			cout << "Bv2.1 error: " << note_to_string(i) << "\n";
			bv21_failed = true;
			break;
		case dcp::VerificationNote::Type::WARNING:
			cout << "Warning: " << note_to_string(i) << "\n";
			warned = true;
			break;
		}
	}

	if (!failed && !quiet) {
		if (bv21_failed && warned) {
			cout << "\nDCP verified OK (but with Bv2.1 errors and warnings).\n";
		} else if (bv21_failed) {
			cout << "\nDCP verified OK (but with Bv2.1 errors).\n";
		} else if (warned) {
			cout << "\nDCP verified OK (but with warnings).\n";
		} else {
			cout << "DCP verified OK.\n";
		}
	}

	exit (failed ? EXIT_FAILURE : EXIT_SUCCESS);
}
