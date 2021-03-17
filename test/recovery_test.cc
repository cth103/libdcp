/*
    Copyright (C) 2012-2019 Carl Hetherington <cth@carlh.net>

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

#include "mono_picture_asset_writer.h"
#include "mono_picture_asset.h"
#include "test.h"
#include <asdcp/KM_util.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

using std::string;
using std::shared_ptr;
using std::make_shared;

/** Check that recovery from a partially-written MXF works */
BOOST_AUTO_TEST_CASE (recovery)
{
	RNGFixer fix;

	string const picture = "test/data/flat_red.j2c";
	dcp::ArrayData data(picture);

	boost::filesystem::remove_all ("build/test/baz");
	boost::filesystem::create_directories ("build/test/baz");
	auto mp = make_shared<dcp::MonoPictureAsset>(dcp::Fraction (24, 1), dcp::Standard::SMPTE);
	auto writer = mp->start_write ("build/test/baz/video1.mxf", false);

	int written_size = 0;
	for (int i = 0; i < 24; ++i) {
		auto info = writer->write (data.data(), data.size());
		BOOST_CHECK_EQUAL (info.hash, "c3c9a3adec09baf2b0bcb65806fbeac8");
		written_size = info.size;
	}

	writer->finalize ();
	writer.reset ();

	boost::filesystem::copy_file ("build/test/baz/video1.mxf", "build/test/baz/video2.mxf");
	boost::filesystem::resize_file ("build/test/baz/video2.mxf", 16384 + data.size() * 11);

	{
		auto f = fopen ("build/test/baz/video2.mxf", "rb+");
		rewind (f);
		char zeros[256];
		memset (zeros, 0, 256);
		fwrite (zeros, 1, 256, f);
		fclose (f);
	}

#ifndef LIBDCP_WINDOWS
	Kumu::ResetTestRNG ();
#endif

	mp = make_shared<dcp::MonoPictureAsset>(dcp::Fraction (24, 1), dcp::Standard::SMPTE);
	writer = mp->start_write ("build/test/baz/video2.mxf", true);

	writer->write (data.data(), data.size());

	for (int i = 1; i < 4; ++i) {
		writer->fake_write (written_size);
	}

	for (int i = 4; i < 24; ++i) {
		writer->write (data.data(), data.size());
	}

	writer->finalize ();

	check_file ("build/test/baz/video1.mxf", "build/test/baz/video2.mxf");
}
