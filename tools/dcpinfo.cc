/*
    Copyright (C) 2012-2018 Carl Hetherington <cth@carlh.net>

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
#include "subtitle_image.h"
#include "interop_subtitle_asset.h"
#include "smpte_subtitle_asset.h"
#include "mono_picture_asset.h"
#include "encrypted_kdm.h"
#include "decrypted_kdm.h"
#include "cpl.h"
#include "common.h"
#include <getopt.h>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <iostream>
#include <cstdlib>
#include <inttypes.h>

using std::string;
using std::cerr;
using std::cout;
using std::list;
using std::pair;
using std::min;
using std::max;
using std::exception;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using boost::optional;
using namespace dcp;

static void
help (string n)
{
	cerr << "Syntax: " << n << " [options] [<DCP>] [<CPL>]\n"
	     << "  -s, --subtitles              list all subtitles\n"
	     << "  -p, --picture                analyse picture\n"
	     << "  -d, --decompress             decompress picture when analysing (this is slow)\n"
	     << "  -k, --keep-going             carry on in the event of errors, if possible\n"
	     << "      --kdm                    KDM to decrypt DCP\n"
	     << "      --private-key            private key for the certificate that the KDM is targeted at\n"
	     << "      --ignore-missing-assets  ignore missing asset files\n";
}

static double
mbits_per_second (int size, Fraction frame_rate)
{
	return size * 8 * frame_rate.as_float() / 1e6;
}

static void
main_picture (shared_ptr<Reel> reel, bool analyse, bool decompress)
{
	if (!reel->main_picture()) {
		return;
	}

	cout << "      Picture ID:  " << reel->main_picture()->id()
	     << " entry " << reel->main_picture()->entry_point()
	     << " duration " << reel->main_picture()->duration()
	     << " intrinsic " << reel->main_picture()->intrinsic_duration();

	if (reel->main_picture()->asset_ref().resolved()) {
		if (reel->main_picture()->asset()) {
			cout << "\n      Picture:     "
			     << reel->main_picture()->asset()->size().width
			     << "x"
			     << reel->main_picture()->asset()->size().height << "\n";
		}

		shared_ptr<MonoPictureAsset> ma = dynamic_pointer_cast<MonoPictureAsset>(reel->main_picture()->asset());
		if (analyse && ma) {
			shared_ptr<MonoPictureAssetReader> reader = ma->start_read ();
			pair<int, int> j2k_size_range (INT_MAX, 0);
			for (int64_t i = 0; i < ma->intrinsic_duration(); ++i) {
				shared_ptr<const MonoPictureFrame> frame = reader->get_frame (i);
				printf("Frame %" PRId64 " J2K size %7d", i, frame->j2k_size());
				j2k_size_range.first = min(j2k_size_range.first, frame->j2k_size());
				j2k_size_range.second = max(j2k_size_range.second, frame->j2k_size());

				if (decompress) {
					try {
						frame->xyz_image();
						printf(" decrypted OK");
					} catch (exception& e) {
						printf(" decryption FAILED");
					}
				}

				printf("\n");

			}
			printf(
				"J2K size ranges from %d (%.1f Mbit/s) to %d (%.1f Mbit/s)\n",
				j2k_size_range.first, mbits_per_second(j2k_size_range.first, ma->frame_rate()),
				j2k_size_range.second, mbits_per_second(j2k_size_range.second, ma->frame_rate())
				);
		}
	} else {
		cout << " - not present in this DCP.\n";
	}
}

static void
main_sound (shared_ptr<Reel> reel)
{
	if (reel->main_sound()) {
		cout << "      Sound ID:    " << reel->main_sound()->id()
		     << " entry " << reel->main_picture()->entry_point()
		     << " duration " << reel->main_picture()->duration()
		     << " intrinsic " << reel->main_picture()->intrinsic_duration();
		if (reel->main_sound()->asset_ref().resolved()) {
			if (reel->main_sound()->asset()) {
				cout << "\n      Sound:       "
				     << reel->main_sound()->asset()->channels()
				     << " channels at "
				     << reel->main_sound()->asset()->sampling_rate() << "Hz\n";
			}
		} else {
			cout << " - not present in this DCP.\n";
		}
	}
}

static void
main_subtitle (shared_ptr<Reel> reel, bool list_subtitles)
{
	if (!reel->main_subtitle()) {
		return;
	}

	cout << "      Subtitle ID: " << reel->main_subtitle()->id();

	if (reel->main_subtitle()->asset_ref().resolved()) {
		list<shared_ptr<Subtitle> > subs = reel->main_subtitle()->asset()->subtitles ();
		cout << "\n      Subtitle:    " << subs.size() << " subtitles";
		shared_ptr<InteropSubtitleAsset> iop = dynamic_pointer_cast<InteropSubtitleAsset> (reel->main_subtitle()->asset());
		if (iop) {
			cout << " in " << iop->language() << "\n";
		}
		shared_ptr<SMPTESubtitleAsset> smpte = dynamic_pointer_cast<SMPTESubtitleAsset> (reel->main_subtitle()->asset());
		if (smpte && smpte->language ()) {
			cout << " in " << smpte->language().get() << "\n";
		}
		if (list_subtitles) {
			BOOST_FOREACH (shared_ptr<Subtitle> k, subs) {
				shared_ptr<SubtitleString> ks = dynamic_pointer_cast<SubtitleString> (k);
				if (ks) {
					cout << *ks << "\n";
				}
				shared_ptr<SubtitleImage> is = dynamic_pointer_cast<SubtitleImage> (k);
				if (is) {
					cout << *is << "\n";
				}
			}
		}
	} else {
		cout << " - not present in this DCP.\n";
	}
}

int
main (int argc, char* argv[])
{
	bool subtitles = false;
	bool keep_going = false;
	bool picture = false;
	bool decompress = false;
	bool ignore_missing_assets = false;
	optional<boost::filesystem::path> kdm;
	optional<boost::filesystem::path> private_key;

	int option_index = 0;
	while (true) {
		static struct option long_options[] = {
			{ "version", no_argument, 0, 'v' },
			{ "help", no_argument, 0, 'h' },
			{ "subtitles", no_argument, 0, 's' },
			{ "keep-going", no_argument, 0, 'k' },
			{ "picture", no_argument, 0, 'p' },
			{ "decompress", no_argument, 0, 'd' },
			{ "ignore-missing-assets", no_argument, 0, 'A' },
			{ "kdm", required_argument, 0, 'B' },
			{ "private-key", required_argument, 0, 'C' },
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long (argc, argv, "vhskpdAB:C:", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'v':
			cout << "libdcp version " << LIBDCP_VERSION << "\n";
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
		case 'p':
			picture = true;
			break;
		case 'd':
			decompress = true;
			break;
		case 'A':
			ignore_missing_assets = true;
			break;
		case 'B':
			kdm = optarg;
			break;
		case 'C':
			private_key = optarg;
			break;
		}
	}

	if (argc <= optind || argc > (optind + 1)) {
		help (argv[0]);
		exit (EXIT_FAILURE);
	}

	if (!boost::filesystem::exists (argv[optind])) {
		cerr << argv[0] << ": DCP or CPL " << argv[optind] << " not found.\n";
		exit (EXIT_FAILURE);
	}

	list<shared_ptr<CPL> > cpls;
	if (boost::filesystem::is_directory(argv[optind])) {
		DCP* dcp = 0;
		DCP::ReadErrors errors;
		try {
			dcp = new DCP (argv[optind]);
			dcp->read (keep_going, &errors);
			if (kdm && private_key) {
				dcp->add(DecryptedKDM(EncryptedKDM(file_to_string(*kdm)), file_to_string(*private_key)));
			}
		} catch (FileError& e) {
			cerr << "Could not read DCP " << argv[optind] << "; " << e.what() << "\n";
			exit (EXIT_FAILURE);
		} catch (DCPReadError& e) {
			cerr << "Could not read DCP " << argv[optind] << "; " << e.what() << "\n";
			exit (EXIT_FAILURE);
		} catch (KDMDecryptionError& e) {
			cerr << e.what() << "\n";
			exit (EXIT_FAILURE);
		}

		cout << "DCP: " << boost::filesystem::path(argv[optind]).string() << "\n";

		dcp::filter_errors (errors, ignore_missing_assets);
		for (DCP::ReadErrors::const_iterator i = errors.begin(); i != errors.end(); ++i) {
			cerr << "Error: " << (*i)->what() << "\n";
		}

		cpls = dcp->cpls ();
	} else {
		cpls.push_back (shared_ptr<CPL>(new CPL(boost::filesystem::path(argv[optind]))));
		keep_going = true;
		ignore_missing_assets = true;
	}

	BOOST_FOREACH (shared_ptr<CPL> i, cpls) {
		cout << "  CPL: " << i->annotation_text() << "\n";

		int R = 1;
		BOOST_FOREACH (shared_ptr<Reel> j, i->reels()) {
			cout << "    Reel " << R << "\n";

			try {
				main_picture (j, picture, decompress);
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
				main_sound (j);
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
				main_subtitle (j, subtitles);
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
