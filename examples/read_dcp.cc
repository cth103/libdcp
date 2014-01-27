/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

/* If you are using an installed libdcp, these #includes would need to be changed to
#include <libdcp/dcp.h>
#include <libdcp/picture_mxf.h>
... etc. ...
*/

#include "dcp.h"
#include "cpl.h"
#include "reel.h"
#include "reel_picture_asset.h"
#include "mono_picture_frame.h"
#include "mono_picture_mxf.h"
#include "stereo_picture_mxf.h"
#include "sound_mxf.h"
#include "subtitle_content.h"
#include "argb_frame.h"
#include <Magick++.h>

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
		if (boost::dynamic_pointer_cast<dcp::MonoPictureMXF> (*i)) {
			std::cout << "2D picture\n";
		} else if (boost::dynamic_pointer_cast<dcp::StereoPictureMXF> (*i)) {
			std::cout << "3D picture\n";
		} else if (boost::dynamic_pointer_cast<dcp::SoundMXF> (*i)) {
			std::cout << "Sound\n";
		} else if (boost::dynamic_pointer_cast<dcp::SubtitleContent> (*i)) {
			std::cout << "Subtitle\n";
		} else if (boost::dynamic_pointer_cast<dcp::CPL> (*i)) {
			std::cout << "CPL\n";
		}
		std::cout << "\t" << (*i)->file().leaf().string() << "\n";
	}

	/* Take the first CPL */
	boost::shared_ptr<dcp::CPL> cpl = dcp.cpls().front ();

	/* Get the MXF of the picture asset in the first reel */
	boost::shared_ptr<dcp::MonoPictureMXF> picture_mxf = boost::dynamic_pointer_cast<dcp::MonoPictureMXF> (cpl->reels().front()->main_picture()->mxf ());

	/* Get the 1000th frame of it */
	boost::shared_ptr<const dcp::MonoPictureFrame> picture_frame_j2k = picture_mxf->get_frame(999);

	/* Get a ARGB copy of it */
	boost::shared_ptr<dcp::ARGBFrame> picture_frame_rgb = picture_frame_j2k->argb_frame ();

	Magick::Image image (picture_frame_rgb->size().width, picture_frame_rgb->size().height, "BGRA", Magick::CharPixel, picture_frame_rgb->data ());
	image.write ("frame.png");

	return 0;
}
