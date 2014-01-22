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

#ifndef LIBDCP_CONTENT_H
#define LIBDCP_CONTENT_H

#include <string>
#include <list>
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <libxml++/libxml++.h>
#include "types.h"
#include "asset.h"

namespace ASDCP {
	class WriterInfo;
}

namespace xmlpp {
	class Element;
}

namespace dcp
{

/** @class Content
 *  @brief An asset that represents a piece of content, i.e. picture, sound or subtitle.
 *
 *  Such a piece of content will be contained in a file (either MXF or XML) within a DCP.
 */
class Content : public Asset
{
public:
	Content (boost::filesystem::path file);
	Content (int edit_rate);

	virtual ~Content () {}

	/** Write details of the asset to a PKL AssetList node.
	 *  @param p Parent node.
	 */
	void write_to_pkl (xmlpp::Node *) const;

	/** Write details of the asset to a ASSETMAP stream.
	 *  @param s Stream.
	 */
	void write_to_assetmap (xmlpp::Node *) const;

	boost::filesystem::path file () const {
		return _file;
	}

	void set_file (boost::filesystem::path file) {
		_file = file;
	}

	int edit_rate () const {
		return _edit_rate;
	}

	void set_edit_rate (int r) {
		_edit_rate = r;
	}

	void set_entry_point (int64_t p) {
		_entry_point = p;
	}

	int64_t intrinsic_duration () const {
		return _intrinsic_duration;
	}

	void set_intrinsic_duration (int64_t d) {
		_intrinsic_duration = d;
	}

	int64_t duration () const {
		return _duration;
	}

	void set_duration (int64_t d) {
		_duration = d;
	}

	virtual bool equals (boost::shared_ptr<const Content> other, EqualityOptions opt, boost::function<void (NoteType, std::string)>) const;

protected:
	boost::filesystem::path _file;
	/** The edit rate; this is normally equal to the number of video frames per second */
	int _edit_rate;
	int64_t _entry_point;
	int64_t _intrinsic_duration;
	int64_t _duration;
};

}

#endif
