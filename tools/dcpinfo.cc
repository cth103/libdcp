/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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
#include "compose.hpp"
#include <getopt.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <inttypes.h>

using std::string;
using std::cerr;
using std::cout;
using std::list;
using std::pair;
using std::min;
using std::max;
using std::exception;
using std::vector;
using std::stringstream;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using boost::optional;
using namespace dcp;

static void
help (string n)
{
	cerr << "Syntax: " << n << " [options] [<DCP>] [<CPL>]\n"
	     << "  -s, --subtitles              list all subtitles\n"
	     << "  -p, --picture                analyse picture\n"
	     << "  -d, --decompress             decompress picture when analysing (this is slow)\n"
	     << "  -o, --only                   only output certain pieces of information; see below.\n"
	     << "      --kdm                    KDM to decrypt DCP\n"
	     << "      --private-key            private key for the certificate that the KDM is targeted at\n"
	     << "      --ignore-missing-assets  ignore missing asset files\n";

	cerr << "--only takes a comma-separated list of strings, one or more of:\n"
		"    dcp-path     DCP path\n"
		"    cpl-name-id  CPL name and ID\n"
		"    picture      picture information\n"
		"    sound        sound information\n"
		"    subtitle     picture information\n"
		"    total-time   total DCP time\n";
}

static double
mbits_per_second (int size, Fraction frame_rate)
{
	return size * 8 * frame_rate.as_float() / 1e6;
}

#define OUTPUT_DCP_PATH(...)    maybe_output(only, "dcp-path", String::compose(__VA_ARGS__));
#define OUTPUT_CPL_NAME_ID(...) maybe_output(only, "cpl-name-id", String::compose(__VA_ARGS__));
#define OUTPUT_PICTURE(...)     maybe_output(only, "picture", String::compose(__VA_ARGS__));
#define OUTPUT_PICTURE_NC(x)    maybe_output(only, "picture", (x));
#define SHOULD_PICTURE          should_output(only, "picture")
#define OUTPUT_SOUND(...)       maybe_output(only, "sound", String::compose(__VA_ARGS__));
#define OUTPUT_SOUND_NC(x)      maybe_output(only, "sound", (x));
#define OUTPUT_SUBTITLE(...)    maybe_output(only, "subtitle", String::compose(__VA_ARGS__));
#define OUTPUT_SUBTITLE_NC(x)   maybe_output(only, "subtitle", (x));
#define OUTPUT_TOTAL_TIME(...)  maybe_output(only, "total-time", String::compose(__VA_ARGS__));

static bool
should_output(vector<string> const& only, string t)
{
	return only.empty() || find(only.begin(), only.end(), t) != only.end();
}

static void
maybe_output(vector<string> const& only, string t, string s)
{
	if (should_output(only, t)) {
		cout << s;
	}
}

static
dcp::Time
main_picture (vector<string> const& only, shared_ptr<Reel> reel, bool analyse, bool decompress)
{
	shared_ptr<dcp::ReelPictureAsset> mp = reel->main_picture ();
	if (!mp) {
		return dcp::Time();
	}

	OUTPUT_PICTURE("      Picture ID:  %1", mp->id());
	if (mp->entry_point()) {
		OUTPUT_PICTURE(" entry %1", *mp->entry_point());
	}
	if (mp->duration()) {
		OUTPUT_PICTURE(
			" duration %1 (%2) intrinsic %3",
			*mp->duration(),
			dcp::Time(*mp->duration(), mp->frame_rate().as_float(), mp->frame_rate().as_float()).as_string(dcp::Standard::SMPTE),
		     	mp->intrinsic_duration()
			);
	} else {
		OUTPUT_PICTURE(" intrinsic duration %1", mp->intrinsic_duration());
	}

	if (mp->asset_ref().resolved()) {
		if (mp->asset()) {
			OUTPUT_PICTURE("\n      Picture:     %1x%2\n", mp->asset()->size().width, mp->asset()->size().height);
		}

		shared_ptr<MonoPictureAsset> ma = dynamic_pointer_cast<MonoPictureAsset>(mp->asset());
		if (analyse && ma) {
			shared_ptr<MonoPictureAssetReader> reader = ma->start_read ();
			pair<int, int> j2k_size_range (INT_MAX, 0);
			for (int64_t i = 0; i < ma->intrinsic_duration(); ++i) {
				shared_ptr<const MonoPictureFrame> frame = reader->get_frame (i);
				if (SHOULD_PICTURE) {
					printf("Frame %" PRId64 " J2K size %7d", i, frame->size());
				}
				j2k_size_range.first = min(j2k_size_range.first, frame->size());
				j2k_size_range.second = max(j2k_size_range.second, frame->size());

				if (decompress) {
					try {
						frame->xyz_image();
						if (SHOULD_PICTURE) {
							printf(" decrypted OK");
						}
					} catch (exception& e) {
						if (SHOULD_PICTURE) {
							printf(" decryption FAILED");
						}
					}
				}

				if (SHOULD_PICTURE) {
					printf("\n");
				}

			}
			if (SHOULD_PICTURE) {
				printf(
						"J2K size ranges from %d (%.1f Mbit/s) to %d (%.1f Mbit/s)\n",
						j2k_size_range.first, mbits_per_second(j2k_size_range.first, ma->frame_rate()),
						j2k_size_range.second, mbits_per_second(j2k_size_range.second, ma->frame_rate())
				      );
			}
		}
	} else {
		OUTPUT_PICTURE_NC(" - not present in this DCP.\n");
	}

	return dcp::Time (
			mp->duration().get_value_or(mp->intrinsic_duration()),
			mp->frame_rate().as_float(),
			mp->frame_rate().as_float()
			);
}

static
void
main_sound (vector<string> const& only, shared_ptr<Reel> reel)
{
	shared_ptr<dcp::ReelSoundAsset> ms = reel->main_sound ();
	if (!ms) {
		return;
	}

	OUTPUT_SOUND("      Sound ID:    %1", ms->id());
	if (ms->entry_point()) {
		OUTPUT_SOUND(" entry %1", *ms->entry_point());
	}
	if (ms->duration()) {
		OUTPUT_SOUND(" duration %1 intrinsic %2", *ms->duration(), ms->intrinsic_duration());
	} else {
		OUTPUT_SOUND(" intrinsic duration %1", ms->intrinsic_duration());
	}

	if (ms->asset_ref().resolved()) {
		if (ms->asset()) {
			OUTPUT_SOUND(
				"\n      Sound:       %1 channels at %2Hz\n",
				ms->asset()->channels(),
				ms->asset()->sampling_rate()
				);
		}
	} else {
		OUTPUT_SOUND_NC(" - not present in this DCP.\n");
	}
}

static
void
main_subtitle (vector<string> const& only, shared_ptr<Reel> reel, bool list_subtitles)
{
	shared_ptr<dcp::ReelSubtitleAsset> ms = reel->main_subtitle ();
	if (!ms) {
		return;
	}

	OUTPUT_SUBTITLE("      Subtitle ID: %1", ms->id());

	if (ms->asset_ref().resolved()) {
		auto subs = ms->asset()->subtitles ();
		OUTPUT_SUBTITLE("\n      Subtitle:    %1 subtitles", subs.size());
		shared_ptr<InteropSubtitleAsset> iop = dynamic_pointer_cast<InteropSubtitleAsset> (ms->asset());
		if (iop) {
			OUTPUT_SUBTITLE(" in %1\n", iop->language());
		}
		shared_ptr<SMPTESubtitleAsset> smpte = dynamic_pointer_cast<SMPTESubtitleAsset> (ms->asset());
		if (smpte && smpte->language ()) {
			OUTPUT_SUBTITLE(" in %1\n", smpte->language().get());
		}
		if (list_subtitles) {
			for (auto k: subs) {
				auto ks = dynamic_pointer_cast<const SubtitleString>(k);
				if (ks) {
					stringstream s;
					s << *ks;
					OUTPUT_SUBTITLE("%1\n", s.str());
				}
				auto is = dynamic_pointer_cast<const SubtitleImage>(k);
				if (is) {
					stringstream s;
					s << *is;
					OUTPUT_SUBTITLE("%1\n", s.str());
				}
			}
		}
	} else {
		OUTPUT_SUBTITLE_NC(" - not present in this DCP.\n");
	}
}


int
main (int argc, char* argv[])
{
	dcp::init ();

	bool subtitles = false;
	bool picture = false;
	bool decompress = false;
	bool ignore_missing_assets = false;
	optional<boost::filesystem::path> kdm;
	optional<boost::filesystem::path> private_key;
	optional<string> only_string;

	int option_index = 0;
	while (true) {
		static struct option long_options[] = {
			{ "version", no_argument, 0, 'v' },
			{ "help", no_argument, 0, 'h' },
			{ "subtitles", no_argument, 0, 's' },
			{ "picture", no_argument, 0, 'p' },
			{ "decompress", no_argument, 0, 'd' },
			{ "only", required_argument, 0, 'o' },
			{ "ignore-missing-assets", no_argument, 0, 'A' },
			{ "kdm", required_argument, 0, 'B' },
			{ "private-key", required_argument, 0, 'C' },
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long (argc, argv, "vhspdo:AB:C:", long_options, &option_index);

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
		case 'p':
			picture = true;
			break;
		case 'd':
			decompress = true;
			break;
		case 'o':
			only_string = optarg;
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

	vector<string> only;
	if (only_string) {
		only = boost::split(only, *only_string, boost::is_any_of(","));
	}

	vector<shared_ptr<CPL> > cpls;
	if (boost::filesystem::is_directory(argv[optind])) {
		DCP* dcp = 0;
		vector<dcp::VerificationNote> notes;
		try {
			dcp = new DCP (argv[optind]);
			dcp->read (&notes);
			if (kdm && private_key) {
				dcp->add(DecryptedKDM(EncryptedKDM(file_to_string(*kdm)), file_to_string(*private_key)));
			}
		} catch (FileError& e) {
			cerr << "Could not read DCP " << argv[optind] << "; " << e.what() << "\n";
			exit (EXIT_FAILURE);
		} catch (ReadError& e) {
			cerr << "Could not read DCP " << argv[optind] << "; " << e.what() << "\n";
			exit (EXIT_FAILURE);
		} catch (KDMDecryptionError& e) {
			cerr << e.what() << "\n";
			exit (EXIT_FAILURE);
		}

		OUTPUT_DCP_PATH("DCP: %1\n", boost::filesystem::path(argv[optind]).string());

		dcp::filter_notes (notes, ignore_missing_assets);
		for (auto i: notes) {
			cerr << "Error: " << note_to_string(i) << "\n";
		}

		cpls = dcp->cpls ();
	} else {
		cpls.push_back (shared_ptr<CPL>(new CPL(boost::filesystem::path(argv[optind]))));
		ignore_missing_assets = true;
	}

	dcp::Time total_time;

	for (auto i: cpls) {
		OUTPUT_CPL_NAME_ID("  CPL: %1 %2\n", i->annotation_text().get_value_or(""), i->id());

		int R = 1;
		for (auto j: i->reels()) {
			if (should_output(only, "picture") || should_output(only, "sound") || should_output(only, "subtitle")) {
				cout << "    Reel " << R << "\n";
			}

			try {
				total_time += main_picture(only, j, picture, decompress);
			} catch (UnresolvedRefError& e) {
				if (!ignore_missing_assets) {
					cerr << e.what() << " (for main picture)\n";
				}
			}

			try {
				main_sound(only, j);
			} catch (UnresolvedRefError& e) {
				if (!ignore_missing_assets) {
					cerr << e.what() << " (for main sound)\n";
				}
			}

			try {
				main_subtitle (only, j, subtitles);
			} catch (UnresolvedRefError& e) {
				if (!ignore_missing_assets) {
					cerr << e.what() << " (for main subtitle)\n";
				}
			}

			++R;
		}
	}

	OUTPUT_TOTAL_TIME("Total: %1\n", total_time.as_string(dcp::Standard::SMPTE));

	return 0;
}
