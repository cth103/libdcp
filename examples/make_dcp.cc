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

/** @file examples/make_dcp.cc
 *  @brief Shows how to make a DCP from some JPEG2000 and WAV files.
 */

#include <vector>
#include <string>

/* If you are using an installed libdcp, these #includes would need to be changed to
#include <libdcp/dcp.h>
#include <libdcp/cpl.h>
#include <libdcp/mono_picture_asset.h>
... etc. ...
*/

#include "dcp.h"
#include "cpl.h"
#include "mono_picture_mxf.h"
#include "mono_picture_mxf_writer.h"
#include "sound_mxf.h"
#include "reel.h"

int
main ()
{
	/* Create a directory to put the DCP in */
	boost::filesystem::create_directory ("DCP");
	
	/* Make a picture MXF.  This is a file which combines JPEG2000 files together to make
	   up the video of the DCP.  First, create the object, specifying a frame rate of 24 frames
	   per second.
	*/

	boost::shared_ptr<dcp::MonoPictureMXF> picture_mxf (new dcp::MonoPictureMXF (24));

	/* Start off a write to it */
	boost::shared_ptr<dcp::MonoPictureMXFWriter> picture_writer = picture_mxf->start_write ("DCP/picture.mxf", false);

	/* Write 24 frames of the same JPEG2000 file */
	dcp::File picture ("examples/help.j2c");
	for (int i = 0; i < 24; ++i) {
		picture_writer->write (picture.data(), picture.size());
	}

	/* And finish off */
	picture_writer->finalize ();

	/* Now create a sound MXF.  As before, create an object and a writer.
	   When creating the object we specify the sampling rate (48kHz) and the number of channels (2).
	*/
	boost::shared_ptr<dcp::SoundMXF> sound_mxf (new dcp::SoundMXF (48000, 2));
	boost::shared_ptr<dcp::SoundMXFWriter> sound_writer = sound_mxf->start_write ("DCP/sound.mxf", false);

	/* Write some sine waves */
	float* audio[2];
	audio[0] = new float[48000];
	audio[1] = new float[48000];
	for (int i = 0; i < 48000; ++i) {
		audio[0][i] = sin (2 * M_PI * i * 440 / 48000) * 0.25;
		audio[1][i] = sin (2 * M_PI * i * 880 / 48000) * 0.25;
	}
	sound_writer->write (audio, 48000);

	/* And tidy up */
	delete[] audio[0];
	delete[] audio[1];
	sound_writer->finalize ();

	/* Now create a reel */
	shared_ptr<dcp::Reel> reel (new dcp::Reel ());

	/* Add picture and sound to it.  The zeros are the `entry points', i.e. the first
	   (video) frame from the MXFs that the reel should play.
	*/
	reel->add (picture, 0);
	reel->add (sound, 0);

	/* Make a CPL with this reel */
	shared_ptr<dcp::CPL> cpl (new dcp::CPL ("My film", dcp::FEATURE));
	cpl->add (reel);

	/* Write the DCP */
	list<shared_ptr<dcp::Asset> > assets;
	asset.push_back (cpl);
	asset.push_back (picture);
	asset.push_back (sound);
	dcp::write ("DCP", assets);
	
	return 0;
}
