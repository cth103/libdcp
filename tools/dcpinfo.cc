/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#include "dcp.h"
#include "exceptions.h"
#include "reel.h"
#include "sound_asset.h"
#include "picture_asset.h"
#include "subtitle_asset.h"
#include "reel_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_subtitle_asset.h"
#include "subtitle_string.h"
#include "interop_subtitle_asset.h"
#include "smpte_subtitle_asset.h"
#include "cpl.h"
#include "common.h"
#include <getopt.h>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <iostream>
#include <cstdlib>

using std::string;
using std::cerr;
using std::cout;
using std::list;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using namespace dcp;

static void
help (string n)
{
	cerr << "Syntax: " << n << " [options] <DCP>\n"
	     << "  -s, --subtitles              list all subtitles\n"
	     << "  -k, --keep-going             carry on in the event of errors, if possible\n"
	     << "      --ignore-missing-assets  ignore missing asset files\n";
}

static void
main_picture (shared_ptr<Reel> reel)
{
	if (reel->main_picture() && reel->main_picture()->asset()) {
		cout << "      Picture:  "
		     << reel->main_picture()->asset()->size().width
		     << "x"
		     << reel->main_picture()->asset()->size().height << "\n";
	}
}

static void
main_sound (shared_ptr<Reel> reel)
{
	if (reel->main_sound() && reel->main_sound()->asset()) {
		cout << "      Sound:    "
		     << reel->main_sound()->asset()->channels()
		     << " channels at "
		     << reel->main_sound()->asset()->sampling_rate() << "Hz\n";
	}
}

static void
main_subtitle (shared_ptr<Reel> reel, bool list_subtitles)
{
	if (!reel->main_subtitle()) {
		return;
	}

	list<SubtitleString> subs = reel->main_subtitle()->asset()->subtitles ();
	cout << "      Subtitle: " << subs.size() << " subtitles";
	shared_ptr<InteropSubtitleAsset> iop = dynamic_pointer_cast<InteropSubtitleAsset> (reel->main_subtitle()->asset());
	if (iop) {
		cout << " in " << iop->language() << "\n";
	}
	shared_ptr<SMPTESubtitleAsset> smpte = dynamic_pointer_cast<SMPTESubtitleAsset> (reel->main_subtitle()->asset());
	if (smpte && smpte->language ()) {
		cout << " in " << smpte->language().get() << "\n";
	}
	if (list_subtitles) {
		BOOST_FOREACH (SubtitleString const& k, subs) {
			cout << k << "\n";
		}
	}
}

int
main (int argc, char* argv[])
{
	bool subtitles = false;
	bool keep_going = false;
	bool ignore_missing_assets = false;

	int option_index = 0;
	while (1) {
		static struct option long_options[] = {
			{ "version", no_argument, 0, 'v' },
			{ "help", no_argument, 0, 'h' },
			{ "subtitles", no_argument, 0, 's' },
			{ "keep-going", no_argument, 0, 'k' },
			{ "ignore-missing-assets", no_argument, 0, 'A' },
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long (argc, argv, "vhskA", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'v':
			cout << "dcpdiff version " << LIBDCP_VERSION << "\n";
			exit (EXIT_SUCCESS);
		case 'h':
			help (argv[0]);
			exit (EXIT_SUCCESS);
		case 's':
			subtitles = true;
			break;
		case 'k':
			keep_going = true;
			break;
		case 'A':
			ignore_missing_assets = true;
			break;
		}
	}

	if (argc <= optind || argc > (optind + 1)) {
		help (argv[0]);
		exit (EXIT_FAILURE);
	}

	if (!boost::filesystem::exists (argv[optind])) {
		cerr << argv[0] << ": DCP " << argv[optind] << " not found.\n";
		exit (EXIT_FAILURE);
	}

	DCP* dcp = 0;
	DCP::ReadErrors errors;
	try {
		dcp = new DCP (argv[optind]);
		dcp->read (keep_going, &errors);
	} catch (FileError& e) {
		cerr << "Could not read DCP " << argv[optind] << "; " << e.what() << "\n";
		exit (EXIT_FAILURE);
	} catch (DCPReadError& e) {
		cerr << "Could not read DCP " << argv[optind] << "; " << e.what() << "\n";
		exit (EXIT_FAILURE);
	}

	cout << "DCP: " << boost::filesystem::path(argv[optind]).filename().string() << "\n";

	dcp::filter_errors (errors, ignore_missing_assets);
	for (DCP::ReadErrors::const_iterator i = errors.begin(); i != errors.end(); ++i) {
		cerr << "Error: " << (*i)->what() << "\n";
	}

	list<shared_ptr<CPL> > cpls = dcp->cpls ();

	for (list<shared_ptr<CPL> >::iterator i = cpls.begin(); i != cpls.end(); ++i) {
		cout << "  CPL: " << (*i)->annotation_text() << "\n";

		list<shared_ptr<Reel> > reels = (*i)->reels ();

		int R = 1;
		for (list<shared_ptr<Reel> >::const_iterator j = reels.begin(); j != reels.end(); ++j) {
			cout << "    Reel " << R << "\n";

			try {
				main_picture (*j);
			} catch (UnresolvedRefError& e) {
				if (keep_going) {
					if (!ignore_missing_assets) {
						cerr << e.what() << " (for main picture)\n";
					}
				} else {
					throw;
				}
			}

			try {
				main_sound (*j);
			} catch (UnresolvedRefError& e) {
				if (keep_going) {
					if (!ignore_missing_assets) {
						cerr << e.what() << " (for main sound)\n";
					}
				} else {
					throw;
				}
			}

			try {
				main_subtitle (*j, subtitles);
			} catch (UnresolvedRefError& e) {
				if (keep_going) {
					if (!ignore_missing_assets) {
						cerr << e.what() << " (for main subtitle)\n";
					}
				} else {
					throw;
				}
			}

			++R;
		}
	}

	return 0;
}
