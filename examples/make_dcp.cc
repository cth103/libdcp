/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

#include <vector>
#include <string>
#include <sigc++/sigc++.h>
#include "dcp.h"
#include "picture_asset.h"
#include "sound_asset.h"
#include "reel.h"

std::string
video_frame (int /* frame */)
{
	return "examples/help.j2c";
}

int
main ()
{
	libdcp::DCP dcp ("My Film DCP", "My Film", libdcp::FEATURE, 24, 48);

	boost::shared_ptr<libdcp::MonoPictureAsset> picture_asset (
		new libdcp::MonoPictureAsset (sigc::ptr_fun (&video_frame), "My Film DCP", "video.mxf", 0, 24, 48, 1998, 1080)
		);

	std::vector<std::string> sound_files;
	sound_files.push_back ("examples/sine_440_-12dB.wav");
	sound_files.push_back ("examples/sine_880_-12dB.wav");
	
	boost::shared_ptr<libdcp::SoundAsset> sound_asset (
		new libdcp::SoundAsset (sound_files, "My Film DCP", "audio.mxf", 0, 24, 48)
		);

	dcp.add_reel (
		boost::shared_ptr<libdcp::Reel> (
			new libdcp::Reel (picture_asset, sound_asset, boost::shared_ptr<libdcp::SubtitleAsset> ())
			)
		);

	dcp.write_xml ();
}
