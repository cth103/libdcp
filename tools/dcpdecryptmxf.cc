/*
    Copyright (C) 2016 Carl Hetherington <cth@carlh.net>

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
#include "decryption_context.h"
#include "key.h"
#include "util.h"
#include "atmos_asset.h"
#include "atmos_frame.h"
#include "atmos_asset_reader.h"
#include "atmos_asset_writer.h"
#include "exceptions.h"
#include <asdcp/AS_DCP.h>
#include <boost/foreach.hpp>
#include <getopt.h>
#include <string>

using std::string;
using std::cerr;
using std::cout;
using boost::optional;
using boost::shared_ptr;

static void
help (string n)
{
	cerr << "Syntax: " << n << " [OPTION] <MXF>]\n"
	     << "  -v, --version      show libdcp version\n"
	     << "  -h, --help         show this help\n"
	     << "  -o, --output       output filename\n"
	     << "  -k, --kdm          KDM file\n"
	     << "  -p, --private-key  private key file\n";
}

int
atmos (
	ASDCP::ATMOS::MXFReader& reader,
	boost::filesystem::path output_file,
	dcp::DecryptedKDM kdm
	)
{
	ASDCP::WriterInfo info;
	if (ASDCP_FAILURE (reader.FillWriterInfo (info))) {
		cerr << "Could not read ATMOS MXF information\n";
		exit (EXIT_FAILURE);
	}

	if (!info.EncryptedEssence) {
		cerr << "MXF is not encrypted!\n";
		exit (EXIT_FAILURE);
	}

	char key_buffer[64];
	Kumu::bin2UUIDhex (info.CryptographicKeyID, ASDCP::UUIDlen, key_buffer, sizeof (key_buffer));
	string const key_id = key_buffer;

	optional<dcp::Key> key;
	BOOST_FOREACH (dcp::DecryptedKDMKey const & i, kdm.keys()) {
		if (i.id() == key_id) {
			key = i.key();
		}
	}

	if (!key) {
		cerr << "Could not find key in KDM.\n";
		exit (EXIT_FAILURE);
	}

	dcp::DecryptionContext dc (key.get());

	ASDCP::ATMOS::AtmosDescriptor desc;
	if (ASDCP_FAILURE (reader.FillAtmosDescriptor (desc))) {
		cerr << "could not read ATMOS descriptor.\n";
		exit (EXIT_FAILURE);
	}

	ASDCP::DCData::FrameBuffer buffer (Kumu::Megabyte);

	for (size_t i = 0; i < desc.ContainerDuration; ++i) {
		reader.ReadFrame (i, buffer, dc.decryption(), 0);
	}

	return 0;
}

int
main (int argc, char* argv[])
{
	optional<boost::filesystem::path> output_file;
	optional<boost::filesystem::path> kdm_file;
	optional<boost::filesystem::path> private_key_file;

	int option_index = 0;
	while (true) {
		struct option long_options[] = {
			{ "version", no_argument, 0, 'v' },
			{ "help", no_argument, 0, 'h' },
			{ "output", required_argument, 0, 'o'},
			{ "kdm", required_argument, 0, 'k'},
			{ "private-key", required_argument, 0, 'p'},
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long (argc, argv, "vho:k:p:", long_options, &option_index);

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
		case 'o':
			output_file = optarg;
			break;
		case 'k':
			kdm_file = optarg;
			break;
		case 'p':
			private_key_file = optarg;
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

	dcp::EncryptedKDM encrypted_kdm (dcp::file_to_string (kdm_file.get ()));
	dcp::DecryptedKDM decrypted_kdm (encrypted_kdm, dcp::file_to_string (private_key_file.get()));

	try {
		dcp::AtmosAsset in (input_file);
		shared_ptr<dcp::AtmosAssetReader> reader = in.start_read ();
		dcp::AtmosAsset out (
			in.edit_rate(),
			in.first_frame(),
			in.max_channel_count(),
			in.max_object_count(),
			in.atmos_id(),
			in.atmos_version()
			);
		shared_ptr<dcp::AtmosAssetWriter> writer = out.start_write (output_file.get());
		for (int64_t i = 0; i < in.intrinsic_duration(); ++i) {
			shared_ptr<const dcp::AtmosFrame> f = reader->get_frame (i);
			writer->write (f->data(), f->size());
		}
	} catch (dcp::DCPReadError& e) {
		cerr << "Unknown MXF format.\n";
		return EXIT_FAILURE;
	}

	return 0;
}
