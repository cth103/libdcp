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

#ifndef LIBDCP_REEL_H
#define LIBDCP_REEL_H

#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <libxml++/libxml++.h>
#include "key.h"
#include "types.h"

namespace xmlpp {
	class Node;
}

namespace libdcp {

class PictureAsset;
class SoundAsset;
class SubtitleAsset;
class KDM;	

/** @brief A reel within a DCP; the part which actually contains picture, sound and subtitle data */	
class Reel
{
public:
	Reel (
		boost::shared_ptr<PictureAsset> picture,
		boost::shared_ptr<SoundAsset> sound,
		boost::shared_ptr<SubtitleAsset> subtitle
		)
		: _main_picture (picture)
		, _main_sound (sound)
		, _main_subtitle (subtitle)
	{}
	
	boost::shared_ptr<const PictureAsset> main_picture () const {
		return _main_picture;
	}

	boost::shared_ptr<const SoundAsset> main_sound () const {
		return _main_sound;
	}
	
	boost::shared_ptr<const SubtitleAsset> main_subtitle () const {
		return _main_subtitle;
	}

	void write_to_cpl (xmlpp::Element *) const;

	bool encrypted () const;

	void set_mxf_keys (libdcp::Key);
	
	bool equals (boost::shared_ptr<const Reel> other, EqualityOptions opt, boost::function<void (NoteType, std::string)> notes) const;

	void add_kdm (KDM const &);

private:
	boost::shared_ptr<PictureAsset> _main_picture;
	boost::shared_ptr<SoundAsset> _main_sound;
	boost::shared_ptr<SubtitleAsset> _main_subtitle;
};

}

#endif
