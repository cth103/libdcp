/*
    Copyright (C) 2019 Carl Hetherington <cth@carlh.net>

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

#include "verify.h"
#include "compose.hpp"
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <getopt.h>
#include <iostream>
#include <cstdlib>

using std::cout;
using std::cerr;
using std::string;
using std::vector;
using std::list;
using boost::bind;
using boost::optional;

static void
help (string n)
{
	cerr << "Syntax: " << n << " [OPTION] <DCP>\n"
	     << "  -V, --version   show libdcp version\n"
	     << "  -h, --help      show this help\n";
}

void
stage (string s, optional<boost::filesystem::path> path)
{
	if (path) {
		cout << s << ": " << path->string() << "\n";
	} else {
		cout << s << "\n";
	}
}

void
progress ()
{

}

std::string
note_to_string (dcp::VerificationNote note)
{
	switch (note.code()) {
	case dcp::VerificationNote::GENERAL_READ:
		return *note.note();
	case dcp::VerificationNote::CPL_HASH_INCORRECT:
		return "The hash of the CPL in the PKL does not agree with the CPL file";
	case dcp::VerificationNote::INVALID_PICTURE_FRAME_RATE:
		return "The picture in a reel has an invalid frame rate";
	case dcp::VerificationNote::PICTURE_HASH_INCORRECT:
		return dcp::String::compose("The hash of the picture asset %1 does not agree with the PKL file", note.file()->filename());
	case dcp::VerificationNote::PKL_CPL_PICTURE_HASHES_DISAGREE:
		return "The PKL and CPL hashes disagree for a picture asset.";
	case dcp::VerificationNote::SOUND_HASH_INCORRECT:
		return dcp::String::compose("The hash of the sound asset %1 does not agree with the PKL file", note.file()->filename());
	case dcp::VerificationNote::PKL_CPL_SOUND_HASHES_DISAGREE:
		return "The PKL and CPL hashes disagree for a sound asset.";
	}

	return "";
}

int
main (int argc, char* argv[])
{
	int option_index = 0;
	while (true) {
		static struct option long_options[] = {
			{ "version", no_argument, 0, 'V'},
			{ "help", no_argument, 0, 'h'},
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long (argc, argv, "Vh", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'V':
			cout << "dcpverify version " << LIBDCP_VERSION << "\n";
			exit (EXIT_SUCCESS);
		case 'h':
			help (argv[0]);
			exit (EXIT_SUCCESS);
		}
	}

	if (argc <= optind) {
		help (argv[0]);
		exit (EXIT_FAILURE);
	}

	if (!boost::filesystem::exists (argv[optind])) {
		cerr << argv[0] << ": DCP " << argv[optind] << " not found.\n";
		exit (EXIT_FAILURE);
	}

	vector<boost::filesystem::path> directories;
	directories.push_back (argv[optind]);
	list<dcp::VerificationNote> notes = dcp::verify (directories, bind(&stage, _1, _2), bind(&progress));

	bool failed = false;
	BOOST_FOREACH (dcp::VerificationNote i, notes) {
		switch (i.type()) {
		case dcp::VerificationNote::VERIFY_ERROR:
			cout << "Error: " << note_to_string(i) << "\n";
			failed = true;
			break;
		case dcp::VerificationNote::VERIFY_WARNING:
			cout << "Warning: " << note_to_string(i) << "\n";
			break;
		}
	}

	if (!failed) {
		cout << "DCP verified OK.\n";
	}

	exit (failed ? EXIT_FAILURE : EXIT_SUCCESS);
}
