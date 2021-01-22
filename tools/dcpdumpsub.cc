/*
    Copyright (C) 2015 Carl Hetherington <cth@carlh.net>

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

#include "smpte_subtitle_asset.h"
#include "decrypted_kdm.h"
#include "decrypted_kdm_key.h"
#include "encrypted_kdm.h"
#include "util.h"
#include <getopt.h>
#include <cstdlib>
#include <string>
#include <iostream>

using std::string;
using std::cout;
using std::cerr;
using std::map;
using boost::optional;

static void
help (string n)
{
	cerr << "Syntax: " << n << " [OPTION] <MXF>\n"
	     << "  -h, --help         show this help\n"
	     << "  -n, --no-fonts     don't extract fonts\n"
	     << "  -k, --kdm          KDM file\n"
	     << "  -p, --private-key  private key file\n";
}

int
main (int argc, char* argv[])
{
	dcp::init ();

	bool extract_fonts = true;
	optional<boost::filesystem::path> kdm_file;
	optional<boost::filesystem::path> private_key_file;

	int option_index = 0;
	while (1) {
		static struct option long_options[] = {
			{ "help", no_argument, 0, 'h'},
			{ "no-fonts", no_argument, 0, 'n'},
			{ "kdm", required_argument, 0, 'k'},
			{ "private-key", required_argument, 0, 'p'},
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long (argc, argv, "hnk:p:", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'h':
			help (argv[0]);
			exit (EXIT_SUCCESS);
		case 'n':
			extract_fonts = false;
			break;
		case 'k':
			kdm_file = optarg;
			break;
		case 'p':
			private_key_file = optarg;
			break;
		}
	}

	if (argc <= optind || argc > (optind + 2)) {
		help (argv[0]);
		exit (EXIT_FAILURE);
	}

	dcp::SMPTESubtitleAsset sub (argv[optind]);

	if (sub.key_id() && (!kdm_file || !private_key_file)) {
		cerr << "Subtitle MXF is encrypted so you must provide a KDM and private key.\n";
		exit (EXIT_FAILURE);
	}

	if (sub.key_id()) {
		dcp::EncryptedKDM encrypted_kdm (dcp::file_to_string (kdm_file.get ()));
		dcp::DecryptedKDM decrypted_kdm (encrypted_kdm, dcp::file_to_string (private_key_file.get()));
		bool done = false;
		for (auto const& i: decrypted_kdm.keys()) {
			if (i.id() == *sub.key_id()) {
				sub.set_key (i.key ());
				done = true;
			}
		}
		if (!done) {
			cerr << "Could not find required key in KDM.\n";
			exit (EXIT_FAILURE);
		}
	}

	cout << sub.xml_as_string() << "\n";

	if (extract_fonts) {
		map<string, dcp::ArrayData> fonts = sub.font_data ();
		for (map<string, dcp::ArrayData>::const_iterator i = fonts.begin(); i != fonts.end(); ++i) {
			FILE* f = dcp::fopen_boost (i->first + ".ttf", "wb");
			if (!f) {
				cerr << "Could not open font file " << i->first << ".ttf for writing";
				exit (EXIT_FAILURE);
			}
			fwrite (i->second.data(), 1, i->second.size(), f);
			fclose (f);
		}
	}
}
