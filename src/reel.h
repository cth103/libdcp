/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_REEL_H
#define LIBDCP_REEL_H

#include "key.h"
#include "types.h"
#include "ref.h"
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <list>

namespace cxml {
	class Node;
}

namespace xmlpp {
	class Element;
}

namespace dcp {

class DecryptedKDM;
class ReelAsset;
class ReelPictureAsset;
class ReelSoundAsset;
class ReelSubtitleAsset;
class ReelAtmosAsset;
class Content;

/** @brief A reel within a DCP; the part which actually refers to picture, sound and subtitle data */
class Reel : public Object
{
public:
	Reel () {}

	Reel (
		boost::shared_ptr<ReelPictureAsset> picture,
		boost::shared_ptr<ReelSoundAsset> sound = boost::shared_ptr<ReelSoundAsset> (),
		boost::shared_ptr<ReelSubtitleAsset> subtitle = boost::shared_ptr<ReelSubtitleAsset> (),
		boost::shared_ptr<ReelAtmosAsset> atmos = boost::shared_ptr<ReelAtmosAsset> ()
		)
		: _main_picture (picture)
		, _main_sound (sound)
		, _main_subtitle (subtitle)
		, _atmos (atmos)
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

	boost::shared_ptr<ReelAtmosAsset> atmos () const {
		return _atmos;
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
	boost::shared_ptr<ReelAtmosAsset> _atmos;
};

}

#endif
