/*
    Copyright (C) 2015 Carl Hetherington <cth@carlh.net>

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

#include <boost/test/unit_test.hpp>
#include "asset.h"

using std::string;
using boost::shared_ptr;

class DummyAsset : public dcp::Asset
{
protected:
	std::string pkl_type (dcp::Standard standard) const {
		return "none";
	}
};

static void
note_handler (dcp::NoteType, string)
{

}

/** Test a few dusty corners of Asset */
BOOST_AUTO_TEST_CASE (asset_test)
{
	shared_ptr<DummyAsset> a (new DummyAsset);
	a->_hash = "abc";
	shared_ptr<DummyAsset> b (new DummyAsset);
	b->_hash = "def";

	BOOST_CHECK (!a->equals (b, dcp::EqualityOptions (), boost::bind (&note_handler, _1, _2)));

	b->_hash = "abc";
	BOOST_CHECK (a->equals (b, dcp::EqualityOptions (), boost::bind (&note_handler, _1, _2)));

	b->_file = "foo/bar/baz";
	BOOST_CHECK (a->equals (b, dcp::EqualityOptions (), boost::bind (&note_handler, _1, _2)));
}
