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

#include "ref.h"

using std::list;
using boost::shared_ptr;
using namespace dcp;

/** Look through a list of assets and copy a shared_ptr to any asset
 *  which matches the ID of this one.
 */
void
Ref::resolve (list<shared_ptr<Asset> > assets)
{
	list<shared_ptr<Asset> >::iterator i = assets.begin();
	while (i != assets.end() && !ids_equal ((*i)->id(), _id)) {
		++i;
	}

	if (i != assets.end ()) {
		_asset = *i;
	}
}
