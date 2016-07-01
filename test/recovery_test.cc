/*
    Copyright (C) 2012-2013 Carl Hetherington <cth@carlh.net>

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

#include "mono_picture_asset_writer.h"
#include "mono_picture_asset.h"
#include <asdcp/KM_util.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

using std::string;
using boost::shared_ptr;

/** Check that recovery from a partially-written MXF works */
BOOST_AUTO_TEST_CASE (recovery)
{
	Kumu::cth_test = true;

	string const picture = "test/data/32x32_red_square.j2c";
	int const size = boost::filesystem::file_size (picture);
	uint8_t* data = new uint8_t[size];
	{
		FILE* f = fopen (picture.c_str(), "rb");
		BOOST_CHECK (f);
		fread (data, 1, size, f);
		fclose (f);
	}

#ifdef LIBDCP_POSIX
	/* XXX: fix this posix-only stuff */
	Kumu::ResetTestRNG ();
#endif

	boost::filesystem::remove_all ("build/test/baz");
	boost::filesystem::create_directories ("build/test/baz");
	shared_ptr<dcp::MonoPictureAsset> mp (new dcp::MonoPictureAsset (dcp::Fraction (24, 1)));
	shared_ptr<dcp::PictureAssetWriter> writer = mp->start_write ("build/test/baz/video1.mxf", dcp::SMPTE, false);

	int written_size = 0;
	for (int i = 0; i < 24; ++i) {
		dcp::FrameInfo info = writer->write (data, size);
		written_size = info.size;
	}

	writer->finalize ();
	writer.reset ();

	boost::filesystem::copy_file ("build/test/baz/video1.mxf", "build/test/baz/video2.mxf");
	boost::filesystem::resize_file ("build/test/baz/video2.mxf", 16384 + 353 * 11);

	{
		FILE* f = fopen ("build/test/baz/video2.mxf", "rb+");
		rewind (f);
		char zeros[256];
		memset (zeros, 0, 256);
		fwrite (zeros, 1, 256, f);
		fclose (f);
	}

#ifdef LIBDCP_POSIX
	Kumu::ResetTestRNG ();
#endif

	mp.reset (new dcp::MonoPictureAsset (dcp::Fraction (24, 1)));
	writer = mp->start_write ("build/test/baz/video2.mxf", dcp::SMPTE, true);

	writer->write (data, size);

	for (int i = 1; i < 4; ++i) {
		writer->fake_write (written_size);
	}

	for (int i = 4; i < 24; ++i) {
		writer->write (data, size);
	}

	writer->finalize ();
}
