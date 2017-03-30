/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

/* If you are using an installed libdcp, these #includes would need to be changed to
#include <dcp/dcp.h>
#include <dcp/picture_asset.h>
... etc. ...
*/

#include "dcp.h"
#include "cpl.h"
#include "reel.h"
#include "reel_picture_asset.h"
#include "mono_picture_frame.h"
#include "mono_picture_asset.h"
#include "mono_picture_asset_reader.h"
#include "stereo_picture_asset.h"
#include "sound_asset.h"
#include "subtitle_asset.h"
#include "openjpeg_image.h"
#include "colour_conversion.h"
#include "rgb_xyz.h"
#include <Magick++.h>
#include <boost/scoped_array.hpp>

/** @file examples/read_dcp.cc
 *  @brief Shows how to read a DCP.
 */

int
main ()
{
	/* Create a DCP, specifying where our existing data is */
	dcp::DCP dcp ("/home/carl/diagonal.com/APPASSIONATA_TLR_F_UK-DEFR_CH_51_2K_LOK_20121115_DGL_OV");
	/* Read the DCP to find out about it */
	dcp.read ();

	if (dcp.encrypted ()) {
		std::cout << "DCP is encrypted.\n";
	} else {
		std::cout << "DCP is not encrypted.\n";
	}

	std::cout << "DCP has " << dcp.cpls().size() << " CPLs.\n";
	std::list<boost::shared_ptr<dcp::Asset> > assets = dcp.assets ();
	std::cout << "DCP has " << assets.size() << " assets.\n";
	for (std::list<boost::shared_ptr<dcp::Asset> >::const_iterator i = assets.begin(); i != assets.end(); ++i) {
		if (boost::dynamic_pointer_cast<dcp::MonoPictureAsset> (*i)) {
			std::cout << "2D picture\n";
		} else if (boost::dynamic_pointer_cast<dcp::StereoPictureAsset> (*i)) {
			std::cout << "3D picture\n";
		} else if (boost::dynamic_pointer_cast<dcp::SoundAsset> (*i)) {
			std::cout << "Sound\n";
		} else if (boost::dynamic_pointer_cast<dcp::SubtitleAsset> (*i)) {
			std::cout << "Subtitle\n";
		} else if (boost::dynamic_pointer_cast<dcp::CPL> (*i)) {
			std::cout << "CPL\n";
		}
		std::cout << "\t" << (*i)->file()->leaf().string() << "\n";
	}

	/* Take the first CPL */
	boost::shared_ptr<dcp::CPL> cpl = dcp.cpls().front ();

	/* Get the picture asset in the first reel */
	boost::shared_ptr<dcp::MonoPictureAsset> picture_asset = boost::dynamic_pointer_cast<dcp::MonoPictureAsset> (
		cpl->reels().front()->main_picture()->asset()
		);

	/* Get a reader for it */
	boost::shared_ptr<dcp::MonoPictureAssetReader> picture_asset_reader = picture_asset->start_read();

	/* Get the 1000th frame of it */
	boost::shared_ptr<const dcp::MonoPictureFrame> picture_frame_j2k = picture_asset_reader->get_frame(999);

	/* Get the frame as an XYZ image */
	boost::shared_ptr<const dcp::OpenJPEGImage> picture_image_xyz = picture_frame_j2k->xyz_image ();

	/* Convert to ARGB */
	boost::scoped_array<uint8_t> rgba (new uint8_t[picture_image_xyz->size().width * picture_image_xyz->size().height * 4]);
	dcp::xyz_to_rgba (picture_image_xyz, dcp::ColourConversion::srgb_to_xyz(), rgba.get(), picture_image_xyz->size().width * 4);

	Magick::Image image (picture_image_xyz->size().width, picture_image_xyz->size().height, "BGRA", Magick::CharPixel, rgba.get ());
	image.write ("frame.png");

	return 0;
}
