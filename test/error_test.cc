/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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
#include "dcp.h"
#include "picture_asset.h"
#include "sound_asset.h"
#include "util.h"
#include "exceptions.h"

using std::vector;
using std::string;

/* Check that an exception is thrown when trying to create MXFs from non-existant sources */
BOOST_AUTO_TEST_CASE (error_test)
{
	/* Create an empty DCP */
	libdcp::DCP d ("build/test/fred");

	/* Random filename that does not exist */
	vector<boost::filesystem::path> p;
	p.push_back ("frobozz");

	/* Trying to create video/audio MXFs using a non-existant file should throw an exception */
	BOOST_CHECK_THROW (
		new libdcp::MonoPictureAsset (p, "build/test/fred", "video.mxf", &d.Progress, 24, 24, libdcp::Size (32, 32), false),
		libdcp::FileError
		);
	
	BOOST_CHECK_THROW (
		new libdcp::SoundAsset (p, "build/test/fred", "audio.mxf", &d.Progress, 24, 24, false),
		libdcp::FileError
		);
}
