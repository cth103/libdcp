/*
    Copyright (C) 2018-2019 Carl Hetherington <cth@carlh.net>

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
#include "util.h"
#include "compose.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <cstdio>
#include <iostream>

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

static vector<boost::filesystem::path>
setup (int n)
{
	boost::filesystem::remove_all (dcp::String::compose("build/test/verify_test%1", n));
	boost::filesystem::create_directory (dcp::String::compose("build/test/verify_test%1", n));
	for (boost::filesystem::directory_iterator i("test/ref/DCP/dcp_test1"); i != boost::filesystem::directory_iterator(); ++i) {
		boost::filesystem::copy_file (i->path(), dcp::String::compose("build/test/verify_test%1", n) / i->path().filename());
	}

	vector<boost::filesystem::path> directories;
	directories.push_back (dcp::String::compose("build/test/verify_test%1", n));
	return directories;

}

class Editor
{
public:
	Editor (boost::filesystem::path path)
		: _path(path)
	{
		_content = dcp::file_to_string (_path);
	}

	~Editor ()
	{
		FILE* f = fopen(_path.string().c_str(), "w");
		BOOST_REQUIRE (f);
		fwrite (_content.c_str(), _content.length(), 1, f);
		fclose (f);
	}

	void replace (string a, string b)
	{
		boost::algorithm::replace_all (_content, a, b);
	}

private:
	boost::filesystem::path _path;
	std::string _content;
};

static
void
dump_notes (list<dcp::VerificationNote> const & notes)
{
	BOOST_FOREACH (dcp::VerificationNote i, notes) {
		std::cout << dcp::note_to_string(i) << "\n";
	}
}

/* Check DCP as-is (should be OK) */
BOOST_AUTO_TEST_CASE (verify_test1)
{
	vector<boost::filesystem::path> directories = setup (1);
	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, "xsd");

	boost::filesystem::path const cpl_file = "build/test/verify_test1/cpl_81fb54df-e1bf-4647-8788-ea7ba154375b.xml";

	list<pair<string, optional<boost::filesystem::path> > >::const_iterator st = stages.begin();
	BOOST_CHECK_EQUAL (st->first, "Checking DCP");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical("build/test/verify_test1"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking CPL");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical(cpl_file));
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

	dump_notes (notes);

	BOOST_CHECK_EQUAL (notes.size(), 0);
}

/* Corrupt the MXFs and check that this is spotted */
BOOST_AUTO_TEST_CASE (verify_test2)
{
	vector<boost::filesystem::path> directories = setup (2);

	FILE* mod = fopen("build/test/verify_test2/video.mxf", "r+b");
	BOOST_REQUIRE (mod);
	fseek (mod, 4096, SEEK_SET);
	int x = 42;
	fwrite (&x, sizeof(x), 1, mod);
	fclose (mod);

	mod = fopen("build/test/verify_test2/audio.mxf", "r+b");
	BOOST_REQUIRE (mod);
	fseek (mod, 4096, SEEK_SET);
	BOOST_REQUIRE (fwrite (&x, sizeof(x), 1, mod) == 1);
	fclose (mod);

	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, "xsd");

	BOOST_REQUIRE_EQUAL (notes.size(), 2);
	BOOST_CHECK_EQUAL (notes.front().type(), dcp::VerificationNote::VERIFY_ERROR);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::PICTURE_HASH_INCORRECT);
	BOOST_CHECK_EQUAL (notes.back().type(), dcp::VerificationNote::VERIFY_ERROR);
	BOOST_CHECK_EQUAL (notes.back().code(), dcp::VerificationNote::SOUND_HASH_INCORRECT);
}

/* Corrupt the hashes in the PKL and check that the disagreement between CPL and PKL is spotted */
BOOST_AUTO_TEST_CASE (verify_test3)
{
	vector<boost::filesystem::path> directories = setup (3);

	{
		Editor e ("build/test/verify_test3/pkl_ae8a9818-872a-4f86-8493-11dfdea03e09.xml");
		e.replace ("<Hash>", "<Hash>x");
	}

	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, "xsd");

	BOOST_REQUIRE_EQUAL (notes.size(), 3);
	list<dcp::VerificationNote>::const_iterator i = notes.begin();
	BOOST_CHECK_EQUAL (i->type(), dcp::VerificationNote::VERIFY_ERROR);
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::CPL_HASH_INCORRECT);
	++i;
	BOOST_CHECK_EQUAL (i->type(), dcp::VerificationNote::VERIFY_ERROR);
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::PKL_CPL_PICTURE_HASHES_DISAGREE);
	++i;
	BOOST_CHECK_EQUAL (i->type(), dcp::VerificationNote::VERIFY_ERROR);
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::PKL_CPL_SOUND_HASHES_DISAGREE);
	++i;
}

/* Corrupt the ContentKind in the CPL */
BOOST_AUTO_TEST_CASE (verify_test4)
{
	vector<boost::filesystem::path> directories = setup (4);

	{
		Editor e ("build/test/verify_test4/cpl_81fb54df-e1bf-4647-8788-ea7ba154375b.xml");
		e.replace ("<ContentKind>", "<ContentKind>x");
	}

	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, "xsd");

	BOOST_REQUIRE_EQUAL (notes.size(), 1);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::GENERAL_READ);
	BOOST_CHECK_EQUAL (*notes.front().note(), "Bad content kind 'xfeature'");
}

static
boost::filesystem::path
cpl (int n)
{
	return dcp::String::compose("build/test/verify_test%1/cpl_81fb54df-e1bf-4647-8788-ea7ba154375b.xml", n);
}

static
void check_after_replace (int n, boost::function<boost::filesystem::path (int)> file, string from, string to, dcp::VerificationNote::Code code1)
{
	vector<boost::filesystem::path> directories = setup (n);

	{
		Editor e (file(n));
		e.replace (from, to);
	}

	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, "xsd");

	dump_notes (notes);

	BOOST_REQUIRE_EQUAL (notes.size(), 1);
	BOOST_CHECK_EQUAL (notes.front().code(), code1);
}

static
void check_after_replace (int n, boost::function<boost::filesystem::path (int)> file, string from, string to, dcp::VerificationNote::Code code1, dcp::VerificationNote::Code code2)
{
	vector<boost::filesystem::path> directories = setup (n);

	{
		Editor e (file(n));
		e.replace (from, to);
	}

	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, "xsd");

	dump_notes (notes);

	BOOST_REQUIRE_EQUAL (notes.size(), 2);
	BOOST_CHECK_EQUAL (notes.front().code(), code1);
	BOOST_CHECK_EQUAL (notes.back().code(), code2);
}

static
void check_after_replace (
	int n, boost::function<boost::filesystem::path (int)> file,
	string from,
	string to,
	dcp::VerificationNote::Code code1,
	dcp::VerificationNote::Code code2,
	dcp::VerificationNote::Code code3
	)
{
	vector<boost::filesystem::path> directories = setup (n);

	{
		Editor e (file(n));
		e.replace (from, to);
	}

	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, "xsd");

	dump_notes (notes);

	BOOST_REQUIRE_EQUAL (notes.size(), 3);
	list<dcp::VerificationNote>::const_iterator i = notes.begin ();
	BOOST_CHECK_EQUAL (i->code(), code1);
	++i;
	BOOST_CHECK_EQUAL (i->code(), code2);
	++i;
	BOOST_CHECK_EQUAL (i->code(), code3);
}

/* FrameRate */
BOOST_AUTO_TEST_CASE (verify_test5)
{
	check_after_replace (
			5, &cpl,
			"<FrameRate>24 1", "<FrameRate>99 1",
			dcp::VerificationNote::CPL_HASH_INCORRECT,
			dcp::VerificationNote::INVALID_PICTURE_FRAME_RATE
			);
}

/* Missing asset */
BOOST_AUTO_TEST_CASE (verify_test6)
{
	vector<boost::filesystem::path> directories = setup (6);

	boost::filesystem::remove ("build/test/verify_test6/video.mxf");
	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, "xsd");

	BOOST_REQUIRE_EQUAL (notes.size(), 1);
	BOOST_CHECK_EQUAL (notes.front().type(), dcp::VerificationNote::VERIFY_ERROR);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::Code::MISSING_ASSET);
}

static
boost::filesystem::path
assetmap (int n)
{
	return dcp::String::compose("build/test/verify_test%1/ASSETMAP.xml", n);
}

/* Empty asset filename in ASSETMAP */
BOOST_AUTO_TEST_CASE (verify_test7)
{
	check_after_replace (
			7, &assetmap,
			"<Path>video.mxf</Path>", "<Path></Path>",
			dcp::VerificationNote::Code::EMPTY_ASSET_PATH
			);
}

/* Mismatched standard */
BOOST_AUTO_TEST_CASE (verify_test8)
{
	check_after_replace (
			8, &cpl,
			"http://www.smpte-ra.org/schemas/429-7/2006/CPL", "http://www.digicine.com/PROTO-ASDCP-CPL-20040511#",
			dcp::VerificationNote::Code::MISMATCHED_STANDARD,
			dcp::VerificationNote::Code::XML_VALIDATION_ERROR,
			dcp::VerificationNote::Code::CPL_HASH_INCORRECT
			);
}

/* Badly formatted <Id> in CPL */
BOOST_AUTO_TEST_CASE (verify_test9)
{
	/* There's no CPL_HASH_INCORRECT error here because it can't find the correct hash by ID (since the ID is wrong) */
	check_after_replace (
			9, &cpl,
			"<Id>urn:uuid:81fb54df-e1bf-4647-8788-ea7ba154375b", "<Id>urn:uuid:81fb54df-e1bf-4647-8788-ea7ba154375",
			dcp::VerificationNote::Code::XML_VALIDATION_ERROR
			);
}

/* Badly formatted <IssueDate> in CPL */
BOOST_AUTO_TEST_CASE (verify_test10)
{
	check_after_replace (
			10, &cpl,
			"<IssueDate>", "<IssueDate>x",
			dcp::VerificationNote::Code::XML_VALIDATION_ERROR,
			dcp::VerificationNote::Code::CPL_HASH_INCORRECT
			);
}
