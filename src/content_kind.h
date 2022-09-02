/*
    Copyright (C) 2022 Carl Hetherington <cth@carlh.net>

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


#ifndef LIBDCP_CONTENT_KIND_H
#define LIBDCP_CONTENT_KIND_H


#include <boost/optional.hpp>
#include <string>
#include <vector>


namespace dcp {


class ContentKind
{
public:
	ContentKind(std::string name, boost::optional<std::string> scope)
		: _name(name)
		, _scope(scope)
	{}

	std::string name() const {
		return _name;
	}

	boost::optional<std::string> scope() const {
		return _scope;
	}

	static const ContentKind FEATURE;
	static const ContentKind SHORT;
	static const ContentKind TRAILER;
	static const ContentKind TEST;
	static const ContentKind TRANSITIONAL;
	static const ContentKind RATING;
	static const ContentKind TEASER;
	static const ContentKind POLICY;
	static const ContentKind PUBLIC_SERVICE_ANNOUNCEMENT;
	static const ContentKind ADVERTISEMENT;
	static const ContentKind CLIP;
	static const ContentKind PROMO;
	static const ContentKind STEREOCARD;
	static const ContentKind EPISODE;
	static const ContentKind HIGHLIGHTS;
	static const ContentKind EVENT;

	static ContentKind from_name(std::string name);
	static std::vector<ContentKind> all();

private:
	explicit ContentKind(std::string name)
		: _name(name)
	{}

	std::string _name;
	boost::optional<std::string> _scope;
};


bool operator==(ContentKind const& a, ContentKind const& b);
bool operator!=(ContentKind const& a, ContentKind const& b);


}


#endif

