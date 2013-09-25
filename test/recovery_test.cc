/*
    Copyright (C) 2012-2013 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include "mono_picture_asset_writer.h"
#include "mono_picture_asset.h"
#include "KM_util.h"

using std::string;
using boost::shared_ptr;

/* Check that recovery from a partially-written MXF works */
BOOST_AUTO_TEST_CASE (recovery)
{
	Kumu::libdcp_test = true;

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
	shared_ptr<libdcp::MonoPictureAsset> mp (new libdcp::MonoPictureAsset ("build/test/baz", "video1.mxf"));
	mp->set_edit_rate (24);
	mp->set_size (libdcp::Size (32, 32));
	shared_ptr<libdcp::PictureAssetWriter> writer = mp->start_write (false);

	int written_size = 0;
	for (int i = 0; i < 24; ++i) {
		libdcp::FrameInfo info = writer->write (data, size);
		written_size = info.size;
	}

	writer->finalize ();
	writer.reset ();

	boost::filesystem::copy_file ("build/test/baz/video1.mxf", "build/test/baz/video2.mxf");
	boost::filesystem::resize_file ("build/test/baz/video2.mxf", 16384 + 353 * 11);

	{
		FILE* f = fopen ("build/test/baz/video2.mxf", "r+");
		rewind (f);
		char zeros[256];
		memset (zeros, 0, 256);
		fwrite (zeros, 1, 256, f);
		fclose (f);
	}

#ifdef LIBDCP_POSIX	
	Kumu::ResetTestRNG ();
#endif	

	mp.reset (new libdcp::MonoPictureAsset ("build/test/baz", "video2.mxf"));
	mp->set_edit_rate (24);
	mp->set_size (libdcp::Size (32, 32));
	writer = mp->start_write (true);

	writer->write (data, size);

	for (int i = 1; i < 4; ++i) {
		writer->fake_write (written_size);
	}

	for (int i = 4; i < 24; ++i) {
		writer->write (data, size);
	}
	
	writer->finalize ();
}
