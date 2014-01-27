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

#include "content.h"
#include "util.h"
#include "metadata.h"
#include "AS_DCP.h"
#include "KM_util.h"
#include <libxml++/nodes/element.h>
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <iostream>

using namespace std;
using namespace boost;
using namespace dcp;

Content::Content (boost::filesystem::path file)
	: Asset (file)
	, _edit_rate (24, 1)
	, _intrinsic_duration (0)
{

}

Content::Content (Fraction edit_rate)
	: _edit_rate (edit_rate)
	, _intrinsic_duration (0)
{

}

bool
Content::equals (shared_ptr<const Content> other, EqualityOptions opt, boost::function<void (NoteType, string)> note) const
{
	if (!Asset::equals (other, opt, note)) {
		return false;
	}
	
	if (_edit_rate != other->_edit_rate) {
		note (ERROR, "content edit rates differ");
	 	return false;
	}
	
	if (_intrinsic_duration != other->_intrinsic_duration) {
	 	note (ERROR, "asset intrinsic durations differ");
		return false;
	}

	return true;
}
