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

/** @file  src/content.h
 *  @brief Content class.
 */

#ifndef LIBDCP_CONTENT_H
#define LIBDCP_CONTENT_H

#include "types.h"
#include "asset.h"
#include <libxml++/libxml++.h>
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <string>
#include <list>

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
	Content (Fraction edit_rate);
	virtual ~Content () {}

	bool equals (
		boost::shared_ptr<const Content> other,
		EqualityOptions opt,
		boost::function<void (NoteType, std::string)>
		) const;

	Fraction edit_rate () const {
		return _edit_rate;
	}

	int64_t intrinsic_duration () const {
		return _intrinsic_duration;
	}

protected:
	friend class MXFWriter;
	
	Fraction _edit_rate;
	int64_t _intrinsic_duration;
};

}

#endif
