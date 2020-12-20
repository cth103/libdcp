/*
    Copyright (C) 2019 Carl Hetherington <cth@carlh.net>

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


#include <boost/bind.hpp>
#include <memory>
#include <boost/test/unit_test.hpp>
#include "cpl.h"
#include "reel.h"
#include "reel_markers_asset.h"

using std::string;
using std::shared_ptr;

BOOST_AUTO_TEST_CASE (markers_write_test)
{
	dcp::CPL cpl("Markers test", dcp::TEST);

	shared_ptr<dcp::ReelMarkersAsset> asset (new dcp::ReelMarkersAsset(dcp::Fraction(24, 1), 0));
	asset->set (dcp::FFOC, dcp::Time(1, 1, 9, 16, 24));
	asset->set (dcp::LFOC, dcp::Time(2, 5, 3, 0, 24));
	asset->set (dcp::FFTC, dcp::Time(0, 6, 4, 2, 24));
	asset->set (dcp::LFTC, dcp::Time(0, 6, 4, 18, 24));
	asset->set (dcp::FFOI, dcp::Time(3, 6, 4, 18, 24));
	asset->set (dcp::LFOI, dcp::Time(3, 2, 4, 18, 24));
	asset->set (dcp::FFEC, dcp::Time(3, 2, 7, 18, 24));
	asset->set (dcp::LFEC, dcp::Time(3, 2, 8, 18, 24));
	asset->set (dcp::FFMC, dcp::Time(4, 2, 8, 18, 24));
	asset->set (dcp::LFMC, dcp::Time(4, 3, 8, 18, 24));

	shared_ptr<dcp::Reel> reel (new dcp::Reel());
	reel->add (asset);

	cpl.add (reel);

	cpl.write_xml ("build/test/markers_test.xml", dcp::SMPTE, shared_ptr<dcp::CertificateChain>());
}

static void
note_handler (dcp::NoteType, string)
{

}

BOOST_AUTO_TEST_CASE (markers_read_test, * boost::unit_test::depends_on("markers_write_test"))
{
	dcp::CPL cpl ("build/test/markers_test.xml");
	BOOST_CHECK_EQUAL (cpl.reels().size(), 1);
	shared_ptr<dcp::Reel> reel = cpl.reels().front();
	shared_ptr<dcp::ReelMarkersAsset> markers = reel->main_markers ();
	BOOST_REQUIRE (markers);

	BOOST_REQUIRE (markers->get(dcp::FFOC));
	BOOST_CHECK (markers->get(dcp::FFOC) == dcp::Time(1, 1, 9, 16, 24));
	BOOST_REQUIRE (markers->get(dcp::LFOC));
	BOOST_CHECK (markers->get(dcp::LFOC) == dcp::Time(2, 5, 3, 0, 24));
	BOOST_REQUIRE (markers->get(dcp::FFTC));
	BOOST_CHECK (markers->get (dcp::FFTC) == dcp::Time(0, 6, 4, 2, 24));
	BOOST_REQUIRE (markers->get(dcp::LFTC));
	BOOST_CHECK (markers->get (dcp::LFTC) == dcp::Time(0, 6, 4, 18, 24));
	BOOST_REQUIRE (markers->get(dcp::FFOI));
	BOOST_CHECK (markers->get (dcp::FFOI) == dcp::Time(3, 6, 4, 18, 24));
	BOOST_REQUIRE (markers->get(dcp::LFOI));
	BOOST_CHECK (markers->get (dcp::LFOI) == dcp::Time(3, 2, 4, 18, 24));
	BOOST_REQUIRE (markers->get(dcp::FFEC));
	BOOST_CHECK (markers->get (dcp::FFEC) == dcp::Time(3, 2, 7, 18, 24));
	BOOST_REQUIRE (markers->get(dcp::LFEC));
	BOOST_CHECK (markers->get (dcp::LFEC) == dcp::Time(3, 2, 8, 18, 24));
	BOOST_REQUIRE (markers->get(dcp::FFMC));
	BOOST_CHECK (markers->get (dcp::FFMC) == dcp::Time(4, 2, 8, 18, 24));
	BOOST_REQUIRE (markers->get(dcp::LFMC));
	BOOST_CHECK (markers->get (dcp::LFMC) == dcp::Time(4, 3, 8, 18, 24));

	BOOST_CHECK (markers->equals(markers, dcp::EqualityOptions(), boost::bind(&note_handler, _1, _2)));

	shared_ptr<dcp::ReelMarkersAsset> markers2 (new dcp::ReelMarkersAsset(dcp::Fraction(24, 1), 0));
	BOOST_CHECK (!markers->equals(markers2, dcp::EqualityOptions(), boost::bind(&note_handler, _1, _2)));
}
