/*
    Copyright (C) 2016-2021 Carl Hetherington <cth@carlh.net>

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


#include "atmos_asset.h"
#include "atmos_asset_reader.h"
#include "atmos_asset_writer.h"
#include "atmos_frame.h"
#include "crypto_context.h"
#include "decrypted_kdm.h"
#include "encrypted_kdm.h"
#include "exceptions.h"
#include "key.h"
#include "mono_picture_asset.h"
#include "mono_picture_asset_writer.h"
#include "util.h"
#include <asdcp/AS_DCP.h>
#include <getopt.h>
#include <iostream>
#include <string>


using std::cerr;
using std::cout;
using std::shared_ptr;
using std::string;
using boost::optional;


static void
help (string n)
{
	cerr << "Re-write a MXF (decrypting it if required)\n"
	     << "Syntax: " << n << " [OPTION] <MXF>]\n"
	     << "  --version          show libdcp version\n"
	     << "  -v, --verbose      be verbose\n"
	     << "  -h, --help         show this help\n"
	     << "  -o, --output       output filename\n"
	     << "  -k, --kdm          KDM file\n"
	     << "  -p, --private-key  private key file\n"
	     << "  -t, --type         MXF type: picture or atmos\n"
	     << "  -i, --ignore-hmac  don't raise an error if HMACs don't agree\n";
}

template <class T, class U>
void copy (T const& in, shared_ptr<U> writer, bool ignore_hmac)
{
	auto reader = in.start_read();
	reader->set_check_hmac (!ignore_hmac);
	for (int64_t i = 0; i < in.intrinsic_duration(); ++i) {
		auto frame = reader->get_frame (i);
		writer->write (frame->data(), frame->size());
	}
	writer->finalize();
};


int
main (int argc, char* argv[])
{
	dcp::init ();

	bool verbose = false;
	optional<boost::filesystem::path> output_file;
	optional<boost::filesystem::path> kdm_file;
	optional<boost::filesystem::path> private_key_file;
	bool ignore_hmac = false;

	enum class Type {
		PICTURE,
		ATMOS,
	};

	optional<Type> type;

	int option_index = 0;
	while (true) {
		struct option long_options[] = {
			{ "version", no_argument, 0, 'A' },
			{ "verbose", no_argument, 0, 'v' },
			{ "help", no_argument, 0, 'h' },
			{ "output", required_argument, 0, 'o'},
			{ "kdm", required_argument, 0, 'k'},
			{ "private-key", required_argument, 0, 'p'},
			{ "type", required_argument, 0, 't' },
			{ "ignore-hmac", no_argument, 0, 'i' },
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long (argc, argv, "Avho:k:p:t:i", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'A':
			cout << "libdcp version " << LIBDCP_VERSION << "\n";
			exit (EXIT_SUCCESS);
		case 'v':
			verbose = true;
			break;
		case 'h':
			help (argv[0]);
			exit (EXIT_SUCCESS);
		case 'o':
			output_file = optarg;
			break;
		case 'k':
			kdm_file = optarg;
			break;
		case 'p':
			private_key_file = optarg;
			break;
		case 't':
			if (strcmp(optarg, "picture") == 0) {
				type = Type::PICTURE;
			} else if (strcmp(optarg, "atmos") == 0) {
				type = Type::ATMOS;
			} else {
				cerr << "Unknown MXF type " << optarg << "\n";
				exit (EXIT_FAILURE);
			}
			break;
		case 'i':
			ignore_hmac = true;
			break;
		}
	}

	if (optind >= argc) {
		help (argv[0]);
		exit (EXIT_FAILURE);
	}

	boost::filesystem::path input_file = argv[optind];

	if (!output_file) {
		cerr << "You must specify -o or --output\n";
		exit (EXIT_FAILURE);
	}

	if (!kdm_file) {
		cerr << "You must specify -k or --kdm\n";
		exit (EXIT_FAILURE);
	}

	if (!private_key_file) {
		cerr << "You must specify -p or --private-key\n";
		exit (EXIT_FAILURE);
	}

	if (!type) {
		cerr << "You must specify -t or --type\n";
		exit (EXIT_FAILURE);
	}

	dcp::EncryptedKDM encrypted_kdm (dcp::file_to_string (kdm_file.get ()));
	dcp::DecryptedKDM decrypted_kdm (encrypted_kdm, dcp::file_to_string (private_key_file.get()));

	auto add_key = [verbose](dcp::MXF& mxf, dcp::DecryptedKDM const& kdm) {
		auto key_id = mxf.key_id();
		if (key_id) {
			if (verbose) {
				cout << "Asset is encrypted.\n";
			}
			auto keys = kdm.keys();
			auto key = std::find_if (keys.begin(), keys.end(), [key_id](dcp::DecryptedKDMKey const& k) { return k.id() == *key_id; });
			if (key == keys.end()) {
				cout << "No key found in KDM.\n";
				exit(EXIT_FAILURE);
			}
			if (verbose) {
				cout << "Key found in KDM.\n";
			}
			mxf.set_key (key->key());
		}
	};

	try {
		switch (*type) {
		case Type::ATMOS:
		{
			dcp::AtmosAsset in (input_file);
			add_key (in, decrypted_kdm);
			dcp::AtmosAsset out (
				in.edit_rate(),
				in.first_frame(),
				in.max_channel_count(),
				in.max_object_count(),
				in.atmos_version()
				);
			auto writer = out.start_write (output_file.get());
			copy (in, writer, ignore_hmac);
			break;
		}
		case Type::PICTURE:
		{
			dcp::MonoPictureAsset in (input_file);
			add_key (in, decrypted_kdm);
			dcp::MonoPictureAsset out (in.edit_rate(), dcp::Standard::SMPTE);
			auto writer = out.start_write (output_file.get(), false);
			copy (in, writer, ignore_hmac);
			break;
		}
		}
	} catch (dcp::ReadError& e) {
		cerr << "Read error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}

	return 0;
}
