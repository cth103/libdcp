/*
    Copyright (C) 2018-2021 Carl Hetherington <cth@carlh.net>

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

#include <boost/test/unit_test.hpp>
#include "smpte_subtitle_asset.h"

using std::make_shared;
using std::string;
using std::shared_ptr;
using boost::optional;

BOOST_AUTO_TEST_CASE (smpte_subtitle_id_test)
{
	dcp::SMPTESubtitleAsset subs;
	subs.add(
		make_shared<dcp::SubtitleString>(
			optional<string>(),
			false, false, false,
			dcp::Colour(),
			64,
			1,
			dcp::Time(0, 1, 2, 3, 24),
			dcp::Time(0, 2, 2, 3, 24),
			0.5,
			dcp::HAlign::CENTER,
			0.5,
			dcp::VAlign::CENTER,
			dcp::Direction::LTR,
			"Hello",
			dcp::Effect::NONE,
			dcp::Colour(),
			dcp::Time(0, 0, 0, 0, 24),
			dcp::Time(0, 0, 0, 0, 24)
			)
		);
	subs.write("build/test/smpte_subtitle_id_test.mxf");

	dcp::SMPTESubtitleAsset check("build/test/smpte_subtitle_id_test.mxf");
	BOOST_CHECK(check.id() != check.xml_id());
}
