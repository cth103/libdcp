/*
    Copyright (C) 2018 Carl Hetherington <cth@carlh.net>

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

#include "verify.h"
#include <boost/test/unit_test.hpp>
#include <cstdio>

using std::list;
using std::pair;
using std::string;
using std::vector;
using std::make_pair;
using boost::optional;

static list<pair<string, optional<boost::filesystem::path> > > stages;

static void
stage (string s, optional<boost::filesystem::path> p)
{
	stages.push_back (make_pair (s, p));
}

static void
progress (float)
{

}

BOOST_AUTO_TEST_CASE (verify_test1)
{
	boost::filesystem::remove_all ("build/test/verify_test1");
	boost::filesystem::create_directory ("build/test/verify_test1");
	for (boost::filesystem::directory_iterator i("test/ref/DCP/dcp_test1"); i != boost::filesystem::directory_iterator(); ++i) {
		boost::filesystem::copy_file (i->path(), "build/test/verify_test1" / i->path().filename());
	}

	vector<boost::filesystem::path> directories;
	directories.push_back ("build/test/verify_test1");
	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress);

	list<pair<string, optional<boost::filesystem::path> > >::const_iterator st = stages.begin();
	BOOST_CHECK_EQUAL (st->first, "Checking DCP");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical("build/test/verify_test1"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking CPL");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical("build/test/verify_test1/cpl_81fb54df-e1bf-4647-8788-ea7ba154375b.xml"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking reel");
	BOOST_REQUIRE (!st->second);
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking picture asset hash");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical("build/test/verify_test1/video.mxf"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking sound asset hash");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical("build/test/verify_test1/audio.mxf"));
	++st;
	BOOST_REQUIRE (st == stages.end());

	BOOST_CHECK_EQUAL (notes.size(), 0);

	FILE* mod = fopen("build/test/verify_test1/video.mxf", "r+b");
	BOOST_REQUIRE (mod);
	fseek (mod, 4096, SEEK_SET);
	int x = 42;
	fwrite (&x, sizeof(x), 1, mod);
	fclose (mod);

	mod = fopen("build/test/verify_test1/audio.mxf", "r+b");
	BOOST_REQUIRE (mod);
	fseek (mod, 4096, SEEK_SET);
	BOOST_REQUIRE (fwrite (&x, sizeof(x), 1, mod) == 1);
	fclose (mod);

	notes = dcp::verify (directories, &stage, &progress);
	BOOST_CHECK_EQUAL (notes.size(), 2);
	BOOST_CHECK_EQUAL (notes.front().type(), dcp::VerificationNote::VERIFY_ERROR);
	BOOST_CHECK_EQUAL (notes.front().note(), "Picture asset hash is incorrect");
	BOOST_CHECK_EQUAL (notes.back().type(), dcp::VerificationNote::VERIFY_ERROR);
	BOOST_CHECK_EQUAL (notes.back().note(), "Sound asset hash is incorrect");
}
