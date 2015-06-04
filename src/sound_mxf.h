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

/** @file  src/sound_mxf.h
 *  @brief SoundMXF class
 */

#ifndef LIBDCP_SOUND_MXF_H
#define LIBDCP_SOUND_MXF_H

#include "mxf.h"
#include "types.h"
#include "metadata.h"

namespace dcp
{

class SoundFrame;
class SoundMXFWriter;

/** @class SoundMXF
 *  @brief Representation of a MXF file containing sound
 */
class SoundMXF : public MXF
{
public:
	SoundMXF (boost::filesystem::path file);
	SoundMXF (Fraction edit_rate, int sampling_rate, int channels);

	boost::shared_ptr<SoundMXFWriter> start_write (boost::filesystem::path file, Standard standard);
	
	bool equals (
		boost::shared_ptr<const Asset> other,
		EqualityOptions opt,
		NoteHandler note
		) const;

	boost::shared_ptr<const SoundFrame> get_frame (int n) const;

	/** @return number of channels */
	int channels () const {
		return _channels;
	}

	/** @return sampling rate in Hz */
	int sampling_rate () const {
		return _sampling_rate;
	}

	Fraction edit_rate () const {
		return _edit_rate;
	}

	int64_t intrinsic_duration () const {
		return _intrinsic_duration;
	}
	
private:
	friend class SoundMXFWriter;
	
	std::string asdcp_kind () const {
		return "Sound";
	}

	Fraction _edit_rate;
	/** The total length of this content in video frames.  The amount of
	 *  content presented may be less than this.
	 */
	int64_t _intrinsic_duration;
	int _channels;      ///< number of channels
	int _sampling_rate; ///< sampling rate in Hz
};

}

#endif
