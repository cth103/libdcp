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


#include "content_kind.h"
#include "dcp_assert.h"
#include <algorithm>
#include <iostream>


using std::string;
using std::vector;
using namespace dcp;


static std::string const smpte_429_16_scope = "http://www.smpte-ra.org/schemas/429-16/2014/CPL-Metadata#scope/content-kind";
static std::string const smpte_2067_3_scope = "http://www.smpte-ra.org/schemas/2067-3/2013#content-kind";


ContentKind const ContentKind::FEATURE                     = ContentKind{"feature"};
ContentKind const ContentKind::SHORT                       = ContentKind{"short"};
ContentKind const ContentKind::TRAILER                     = ContentKind{"trailer"};
ContentKind const ContentKind::TEST                        = ContentKind{"test"};
ContentKind const ContentKind::TRANSITIONAL                = ContentKind{"transitional"};
ContentKind const ContentKind::RATING                      = ContentKind{"rating"};
ContentKind const ContentKind::TEASER                      = ContentKind{"teaser"};
ContentKind const ContentKind::POLICY                      = ContentKind{"policy"};
ContentKind const ContentKind::PUBLIC_SERVICE_ANNOUNCEMENT = ContentKind{"psa"};
ContentKind const ContentKind::ADVERTISEMENT               = ContentKind{"advertisement"};
ContentKind const ContentKind::CLIP                        = ContentKind{"clip", smpte_429_16_scope};
ContentKind const ContentKind::PROMO                       = ContentKind{"promo", smpte_429_16_scope};
ContentKind const ContentKind::STEREOCARD                  = ContentKind{"stereocard", smpte_429_16_scope};
ContentKind const ContentKind::EPISODE                     = ContentKind{"episode", smpte_2067_3_scope};
ContentKind const ContentKind::HIGHLIGHTS                  = ContentKind{"highlights", smpte_2067_3_scope};
ContentKind const ContentKind::EVENT                       = ContentKind{"event", smpte_2067_3_scope};


vector<ContentKind>
ContentKind::all()
{
	return {
		ContentKind::FEATURE,
		ContentKind::SHORT,
		ContentKind::TRAILER,
		ContentKind::TEST,
		ContentKind::TRANSITIONAL,
		ContentKind::RATING,
		ContentKind::TEASER,
		ContentKind::POLICY,
		ContentKind::PUBLIC_SERVICE_ANNOUNCEMENT,
		ContentKind::ADVERTISEMENT,
		ContentKind::EPISODE,
		ContentKind::PROMO
	};
}


ContentKind
ContentKind::from_name(string name)
{
	auto const all_kinds = all();
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	auto iter = std::find_if(all_kinds.begin(), all_kinds.end(), [&name](ContentKind const& k) { return k.name() == name; });
	if (iter == all_kinds.end()) {
		throw BadContentKindError(name);
	}
	return *iter;
}


bool
dcp::operator==(ContentKind const& a, ContentKind const& b)
{
	return a.name() == b.name() && a.scope() == b.scope();
}


bool
dcp::operator!=(ContentKind const& a, ContentKind const& b)
{
	return !(a == b);
}


