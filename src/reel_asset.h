/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_REEL_ASSET_H
#define LIBDCP_REEL_ASSET_H

#include "object.h"
#include "util.h"
#include "ref.h"
#include <boost/shared_ptr.hpp>

namespace cxml {
	class Node;
}

namespace xmlpp {
	class Node;
}

namespace dcp {

class Content;

class ReelAsset : public Object
{
public:
	ReelAsset ();
	ReelAsset (boost::shared_ptr<Content> content, int64_t entry_point);
	ReelAsset (boost::shared_ptr<const cxml::Node>);

	Ref<Content> content () const {
		return _content;
	}

	bool encrypted () const {
		return !_key_id.empty ();
	}

	std::string key_id () const {
		return _key_id;
	}

	virtual void write_to_cpl (xmlpp::Node* node, Standard standard) const;
	virtual bool equals (boost::shared_ptr<const ReelAsset>, EqualityOptions, boost::function<void (NoteType, std::string)>) const {
		return false;
	}

protected:
	virtual std::string cpl_node_name () const = 0;
	virtual std::pair<std::string, std::string> cpl_node_attribute () const;

	Ref<Content> _content;

private:
	std::string _annotation_text;
	Fraction _edit_rate;
	int64_t _intrinsic_duration;
	int64_t _entry_point;
	int64_t _duration;
	std::string _hash;
	std::string _key_id;
};

}

#endif
