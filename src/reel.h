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

#include <list>
#include <boost/shared_ptr.hpp>
#include "types.h"

namespace libdcp {

class PictureAsset;
class SoundAsset;	
class SubtitleAsset;	

class Reel
{
public:
	Reel (
		boost::shared_ptr<const PictureAsset> picture,
		boost::shared_ptr<const SoundAsset> sound,
		boost::shared_ptr<const SubtitleAsset> subtitle
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

	void write_to_cpl (std::ostream & s) const;
	void write_to_pkl (std::ostream & s) const;
	void write_to_assetmap (std::ostream & s) const;

	std::list<std::string> equals (boost::shared_ptr<const Reel> other, EqualityOptions opt) const;

private:
	boost::shared_ptr<const PictureAsset> _main_picture;
	boost::shared_ptr<const SoundAsset> _main_sound;
	boost::shared_ptr<const SubtitleAsset> _main_subtitle;
};

}
