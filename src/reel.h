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

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
*/


/** @file  src/reel.cc
 *  @brief Reel class
 */


#ifndef LIBDCP_REEL_H
#define LIBDCP_REEL_H


#include "key.h"
#include "types.h"
#include "ref.h"
#include <memory>
#include <boost/function.hpp>


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
class ReelMarkersAsset;
class ReelClosedCaptionAsset;
class ReelAtmosAsset;
class Content;


/** @brief A reel within a DCP; the part which actually refers to picture, sound, subtitle, marker and Atmos data */
class Reel : public Object
{
public:
	Reel () {}

	Reel (
		std::shared_ptr<ReelPictureAsset> picture,
		std::shared_ptr<ReelSoundAsset> sound = std::shared_ptr<ReelSoundAsset> (),
		std::shared_ptr<ReelSubtitleAsset> subtitle = std::shared_ptr<ReelSubtitleAsset> (),
		std::shared_ptr<ReelMarkersAsset> markers = std::shared_ptr<ReelMarkersAsset> (),
		std::shared_ptr<ReelAtmosAsset> atmos = std::shared_ptr<ReelAtmosAsset> ()
		)
		: _main_picture (picture)
		, _main_sound (sound)
		, _main_subtitle (subtitle)
		, _main_markers (markers)
		, _atmos (atmos)
	{}

	explicit Reel (std::shared_ptr<const cxml::Node>, dcp::Standard standard);

	std::shared_ptr<ReelPictureAsset> main_picture () const {
		return _main_picture;
	}

	std::shared_ptr<ReelSoundAsset> main_sound () const {
		return _main_sound;
	}

	std::shared_ptr<ReelSubtitleAsset> main_subtitle () const {
		return _main_subtitle;
	}

	std::shared_ptr<ReelMarkersAsset> main_markers () const {
		return _main_markers;
	}

	std::vector<std::shared_ptr<ReelClosedCaptionAsset>> closed_captions () const {
		return _closed_captions;
	}

	std::shared_ptr<ReelAtmosAsset> atmos () const {
		return _atmos;
	}

	int64_t duration () const;

	void add (std::shared_ptr<ReelAsset> asset);

	std::vector<std::shared_ptr<ReelAsset>> assets () const;

	xmlpp::Element* write_to_cpl (xmlpp::Element* node, Standard standard) const;

	bool any_encrypted () const;
	bool all_encrypted () const;

	bool equals (std::shared_ptr<const Reel> other, EqualityOptions opt, NoteHandler notes) const;

	void add (DecryptedKDM const &);

	void resolve_refs (std::vector<std::shared_ptr<Asset>>);

private:
	std::shared_ptr<ReelPictureAsset> _main_picture;
	std::shared_ptr<ReelSoundAsset> _main_sound;
	std::shared_ptr<ReelSubtitleAsset> _main_subtitle;
	std::shared_ptr<ReelMarkersAsset> _main_markers;
	std::vector<std::shared_ptr<ReelClosedCaptionAsset>> _closed_captions;
	std::shared_ptr<ReelAtmosAsset> _atmos;
};

}

#endif
