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


#include "cpl.h"
#include "test.h"
#include <boost/test/unit_test.hpp>


using std::list;
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;


BOOST_AUTO_TEST_CASE (cpl_ratings)
{
	dcp::CPL cpl ("annotation", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);

	vector<dcp::Rating> ratings = {
		dcp::Rating("http://www.mpaa.org/2003-ratings", "PG-13"),
		dcp::Rating("http://www.movielabs.com/md/ratings/GB/BBFC/1/12A%3C/Agency", "12A")
	};
	cpl.set_ratings (ratings);

	auto reel = make_shared<dcp::Reel>();
	cpl.add (reel);

	cpl.write_xml ("build/test/cpl_ratings.xml", {});

	vector<string> ignore = { "Id", "Issuer", "Creator", "IssueDate", "LabelText" };
	check_xml (
		dcp::file_to_string("build/test/cpl_ratings.xml"),
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<CompositionPlaylist xmlns=\"http://www.smpte-ra.org/schemas/429-7/2006/CPL\">\n"
		"  <Id>urn:uuid:c34fc31a-1f45-4740-85cc-086e88104f5e</Id>\n"
		"  <AnnotationText>annotation</AnnotationText>\n"
		"  <IssueDate>2019-03-19T16:56:12+00:00</IssueDate>\n"
		"  <Issuer>libdcp1.6.4devel</Issuer>\n"
		"  <Creator>libdcp1.6.4devel</Creator>\n"
		"  <ContentTitleText>annotation</ContentTitleText>\n"
		"  <ContentKind>feature</ContentKind>\n"
		"  <ContentVersion>\n"
		"    <Id>urn:uuid:9aa4d5ae-2669-4090-a201-3c68a33cda64</Id>\n"
		"    <LabelText>9aa4d5ae-2669-4090-a201-3c68a33cda642019-03-19T16:56:12+00:00</LabelText>\n"
		"  </ContentVersion>\n"
		"  <RatingList>\n"
		"    <Rating>\n"
		"      <Agency>http://www.mpaa.org/2003-ratings</Agency>\n"
		"      <Label>PG-13</Label>\n"
		"    </Rating>\n"
		"    <Rating>\n"
		"      <Agency>http://www.movielabs.com/md/ratings/GB/BBFC/1/12A%3C/Agency</Agency>\n"
		"      <Label>12A</Label>\n"
		"    </Rating>\n"
		"  </RatingList>\n"
		"  <ReelList>\n"
		"    <Reel>\n"
		"      <Id>urn:uuid:56a781ed-ace3-4cdf-8391-93b1bcea54eb</Id>\n"
		"      <AssetList/>\n"
		"    </Reel>\n"
		"  </ReelList>\n"
		"</CompositionPlaylist>\n",
		ignore
		);

	dcp::CPL cpl2 ("build/test/cpl_ratings.xml");
	BOOST_TEST(ratings == cpl2.ratings());
}
