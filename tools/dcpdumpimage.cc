/*
    Copyright (C) 2023 Carl Hetherington <cth@carlh.net>

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


#include "colour_conversion.h"
#include "cpl.h"
#include "dcp.h"
#include "mono_picture_asset.h"
#include "openjpeg_image.h"
#include "reel_picture_asset.h"
#include "raw_convert.h"
#include "reel.h"
#include "rgb_xyz.h"
#include "util.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <Magick++.h>
LIBDCP_ENABLE_WARNINGS
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>


using std::cerr;
using std::dynamic_pointer_cast;
using std::exception;
using std::string;
using std::vector;
using boost::optional;



static void
help(string n)
{
	cerr << "Syntax: " << n << " [OPTION] <DCP>\n"
	     << "  -h, --help                 show this help\n"
	     << "  -f, --frame-index <index>  frame index (from 0) to extract\n"
	     << "  --horizontal-line <y>      drop a horizontal line over the image at the given position (origin is the top of the frame)\n"
	     << "  -o, --output <filename>    output PNG file\n";
}


int
main(int argc, char* argv[])
{
	dcp::init();
	Magick::InitializeMagick(nullptr);

	int frame_index = 0;
	vector<int> horizontal_lines;
	optional<boost::filesystem::path> output_filename;

	int option_index = 0;
	while (true) {
		static struct option long_options[] = {
			{ "help", no_argument, 0, 'h' },
			{ "frame-index", required_argument, 0, 'f' },
			{ "horizontal-line", required_argument, 0, 'A' },
			{ "output", required_argument, 0, 'o' },
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long(argc, argv, "hf:o:", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'h':
			help(argv[0]);
			exit (EXIT_SUCCESS);
		case 'f':
			frame_index = dcp::raw_convert<int>(optarg);
			break;
		case 'A':
			horizontal_lines.push_back(dcp::raw_convert<int>(optarg));
			break;
		case 'o':
			output_filename = optarg;
			break;
		}
	}

	if (argc <= optind || argc > (optind + 1)) {
		help(argv[0]);
		exit(EXIT_FAILURE);
	}

	if (!output_filename) {
		std::cerr << "You must specify -o or --output\n";
		exit(EXIT_FAILURE);
	}

	dcp::DCP dcp(argv[optind]);

	try {
		dcp.read();
	} catch (exception& e) {
		std::cerr << e.what() << "\n";
		exit(EXIT_FAILURE);
	}

	if (dcp.cpls().empty()) {
		std::cerr << "No CPLs found in DCP.\n";
		exit(EXIT_FAILURE);
	}

	if (dcp.cpls().size() > 1) {
		std::cerr << "More than one CPLs found in DCP.\n";
		exit(EXIT_FAILURE);
	}

	bool found = false;
	auto cpl = dcp.cpls()[0];
	for (auto reel: cpl->reels()) {
		auto duration = reel->main_picture()->actual_duration();
		if (frame_index >= duration) {
			frame_index -= duration;
		} else {
			auto reader = dynamic_pointer_cast<dcp::MonoPictureAsset>(reel->main_picture()->asset())->start_read();
			auto frame = reader->get_frame(frame_index);
			auto xyz = frame->xyz_image();
			std::vector<uint8_t> rgba(xyz->size().width * xyz->size().height * 4);
			dcp::xyz_to_rgba(xyz, dcp::ColourConversion::srgb_to_xyz(), rgba.data(), xyz->size().width * 4);

			Magick::Image image(xyz->size().width, xyz->size().height, "BGRA", Magick::CharPixel, rgba.data());

			image.strokeColor("white");
			image.strokeWidth(1);
			for (auto line: horizontal_lines) {
				image.draw(Magick::DrawableLine(0, line, xyz->size().width, line));
			}

			image.write(output_filename->string());

			found = true;
		}
	}

	if (!found) {
		std::cerr << "Frame index " << frame_index << " is beyond the end of th DCP.\n";
		exit(EXIT_FAILURE);
	}

	return 0;
}
