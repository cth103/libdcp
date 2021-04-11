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
*/

/** @file examples/make_dcp.cc
 *  @brief Shows how to make a DCP from some JPEG2000 and audio data.
 */

#include <vector>
#include <string>

/* If you are using an installed libdcp, these #includes would need to be changed to
#include <dcp/dcp.h>
#include <dcp/cpl.h>
#include <dcp/mono_picture_asset.h>
... etc. ...
*/

#include "dcp.h"
#include "cpl.h"
#include "mono_picture_asset.h"
#include "mono_picture_asset_writer.h"
#include "sound_asset.h"
#include "sound_asset_writer.h"
#include "reel.h"
#include "reel_mono_picture_asset.h"
#include "reel_sound_asset.h"
#include <cmath>

int
main ()
{
	/* Set up libdcp */
	dcp::init();

	/* Create a directory to put the DCP in */
	boost::filesystem::create_directory("DCP");

	/* Make a picture asset.  This is a file which combines JPEG2000 files together to make
	   up the video of the DCP.  First, create the object, specifying a frame rate of 24 frames
	   per second.
	*/

	auto picture_asset = std::make_shared<dcp::MonoPictureAsset>(dcp::Fraction(24, 1), dcp::Standard::SMPTE);

	/* Start off a write to it */
	auto picture_writer = picture_asset->start_write("DCP/picture.mxf", false);

	/* Write 24 frames of the same JPEG2000 file */
	dcp::ArrayData picture("examples/help.j2c");
	for (int i = 0; i < 24; ++i) {
		picture_writer->write (picture);
	}

	/* And finish off */
	picture_writer->finalize();

	/* Now create a sound MXF.  As before, create an object and a writer.
	   When creating the object we specify the sampling rate (48kHz) and the number of channels (2).
	*/
	auto sound_asset = std::make_shared<dcp::SoundAsset>(dcp::Fraction(24, 1), 48000, 2, dcp::LanguageTag("en-GB"), dcp::Standard::SMPTE);
	auto sound_writer = sound_asset->start_write("DCP/sound.mxf");

	/* Write some sine waves */
	std::array<float, 48000> left;
	std::array<float, 48000> right;
	for (int i = 0; i < 48000; ++i) {
		left[i] = sin (2 * M_PI * i * 440 / 48000) * 0.25;
		right[i] = sin (2 * M_PI * i * 880 / 48000) * 0.25;
	}
	std::array<float*, 2> audio;
	audio[0] = left.data();
	audio[1] = right.data();
	sound_writer->write (audio.data(), 48000);

	/* And finish off */
	sound_writer->finalize ();

	/* Now create a reel */
	auto reel = std::make_shared<dcp::Reel>();

	/* Add picture and sound to it.  The zeros are the `entry points', i.e. the first
	   (video) frame from the assets that the reel should play.
	*/
	reel->add(std::make_shared<dcp::ReelMonoPictureAsset>(picture_asset, 0));
	reel->add(std::make_shared<dcp::ReelSoundAsset>(sound_asset, 0));

	/* Make a CPL with this reel */
	auto cpl = std::make_shared<dcp::CPL>("My film", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	cpl->add(reel);

	/* Write the DCP */
	dcp::DCP dcp ("DCP");
	dcp.add (cpl);
	dcp.write_xml ();

	return 0;
}
