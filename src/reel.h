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

#ifndef LIBDCP_REEL_H
#define LIBDCP_REEL_H

#include "key.h"
#include "types.h"
#include "ref.h"
#include <libxml++/libxml++.h>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <list>

namespace cxml {
	class Node;
}

namespace dcp {

class DecryptedKDM;
class ReelAsset;
class ReelPictureAsset;
class ReelSoundAsset;
class ReelSubtitleAsset;
class Content;

/** @brief A reel within a DCP; the part which actually refers to picture, sound and subtitle data */
class Reel : public Object
{
public:
	Reel () {}

	Reel (
		boost::shared_ptr<ReelPictureAsset> picture,
		boost::shared_ptr<ReelSoundAsset> sound,
		boost::shared_ptr<ReelSubtitleAsset> subtitle
		)
		: _main_picture (picture)
		, _main_sound (sound)
		, _main_subtitle (subtitle)
	{}

	Reel (boost::shared_ptr<const cxml::Node>);

	boost::shared_ptr<ReelPictureAsset> main_picture () const {
		return _main_picture;
	}

	boost::shared_ptr<ReelSoundAsset> main_sound () const {
		return _main_sound;
	}

	boost::shared_ptr<ReelSubtitleAsset> main_subtitle () const {
		return _main_subtitle;
	}

	int64_t duration () const;

	void add (boost::shared_ptr<ReelAsset> asset);

	void write_to_cpl (xmlpp::Element* node, Standard standard) const;

	bool encrypted () const;

	bool equals (boost::shared_ptr<const Reel> other, EqualityOptions opt, NoteHandler notes) const;

	void add (DecryptedKDM const &);

	void resolve_refs (std::list<boost::shared_ptr<Asset> >);

private:
	boost::shared_ptr<ReelPictureAsset> _main_picture;
	boost::shared_ptr<ReelSoundAsset> _main_sound;
	boost::shared_ptr<ReelSubtitleAsset> _main_subtitle;
};

}

#endif
