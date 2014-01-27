/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_LUT_CACHE_H
#define LIBDCP_LUT_CACHE_H

#include <list>
#include <boost/shared_ptr.hpp>

template<class T>
class LUTCache : public boost::noncopyable
{
public:
	boost::shared_ptr<T> get (int bit_depth, float gamma)
	{
		for (typename std::list<boost::shared_ptr<T> >::iterator i = _cache.begin(); i != _cache.end(); ++i) {
			if ((*i)->bit_depth() == bit_depth && (*i)->gamma() == gamma) {
				return *i;
			}
		}

		boost::shared_ptr<T> lut (new T (bit_depth, gamma));
		_cache.push_back (lut);
		return lut;
	}

private:
	std::list<boost::shared_ptr<T> > _cache;
};

#endif
