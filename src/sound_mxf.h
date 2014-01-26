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

#ifndef LIBDCP_SOUND_MXF_H
#define LIBDCP_SOUND_MXF_H

#include "mxf.h"
#include "types.h"
#include "metadata.h"

namespace dcp
{

class SoundFrame;
class SoundMXFWriter;

class SoundMXF : public MXF
{
public:
	SoundMXF (boost::filesystem::path file);
	SoundMXF (Fraction edit_rate, int sampling_rate, int channels);

	boost::shared_ptr<SoundMXFWriter> start_write (boost::filesystem::path file, Standard standard);
	
	bool equals (boost::shared_ptr<const Content> other, EqualityOptions opt, boost::function<void (NoteType, std::string)> note) const;

	boost::shared_ptr<const SoundFrame> get_frame (int n) const;

	void set_channels (int c) {
		_channels = c;
	}
	
	int channels () const {
		return _channels;
	}

	void set_sampling_rate (int s) {
		_sampling_rate = s;
	}

	int sampling_rate () const {
		return _sampling_rate;
	}

private:
	std::string key_type () const;

	/** Number of channels in the asset */
	int _channels;
	int _sampling_rate;
};

}

#endif
