/*
    Copyright (C) 2018-2020 Carl Hetherington <cth@carlh.net>

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
#include "j2k.h"
#include "reel.h"
#include "reel_mono_picture_asset.h"
#include "cpl.h"
#include "dcp.h"
#include "openjpeg_image.h"
#include "mono_picture_asset.h"
#include "mono_picture_asset_writer.h"
#include "interop_subtitle_asset.h"
#include "smpte_subtitle_asset.h"
#include "reel_subtitle_asset.h"
#include "compose.hpp"
#include "test.h"
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
using std::shared_ptr;


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
setup (int reference_number, int verify_test_number)
{
	boost::filesystem::remove_all (dcp::String::compose("build/test/verify_test%1", verify_test_number));
	boost::filesystem::create_directory (dcp::String::compose("build/test/verify_test%1", verify_test_number));
	for (boost::filesystem::directory_iterator i(dcp::String::compose("test/ref/DCP/dcp_test%1", reference_number)); i != boost::filesystem::directory_iterator(); ++i) {
		boost::filesystem::copy_file (i->path(), dcp::String::compose("build/test/verify_test%1", verify_test_number) / i->path().filename());
	}

	vector<boost::filesystem::path> directories;
	directories.push_back (dcp::String::compose("build/test/verify_test%1", verify_test_number));
	return directories;

}


/** Class that can alter a file by searching and replacing strings within it.
 *  On destruction modifies the file whose name was given to the constructor.
 */
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


#if 0
static
void
dump_notes (list<dcp::VerificationNote> const & notes)
{
	BOOST_FOREACH (dcp::VerificationNote i, notes) {
		std::cout << dcp::note_to_string(i) << "\n";
	}
}
#endif

/* Check DCP as-is (should be OK) */
BOOST_AUTO_TEST_CASE (verify_test1)
{
	stages.clear ();
	vector<boost::filesystem::path> directories = setup (1, 1);
	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, xsd_test);

	boost::filesystem::path const cpl_file = "build/test/verify_test1/cpl_81fb54df-e1bf-4647-8788-ea7ba154375b.xml";
	boost::filesystem::path const pkl_file = "build/test/verify_test1/pkl_cd49971e-bf4c-4594-8474-54ebef09a40c.xml";
	boost::filesystem::path const assetmap_file = "build/test/verify_test1/ASSETMAP.xml";

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
	BOOST_CHECK_EQUAL (st->first, "Checking picture frame sizes");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical("build/test/verify_test1/video.mxf"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking sound asset hash");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical("build/test/verify_test1/audio.mxf"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking PKL");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical(pkl_file));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking ASSETMAP");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical(assetmap_file));
	++st;
	BOOST_REQUIRE (st == stages.end());

	BOOST_CHECK_EQUAL (notes.size(), 0);
}

/* Corrupt the MXFs and check that this is spotted */
BOOST_AUTO_TEST_CASE (verify_test2)
{
	vector<boost::filesystem::path> directories = setup (1, 2);

	FILE* mod = fopen("build/test/verify_test2/video.mxf", "r+b");
	BOOST_REQUIRE (mod);
	fseek (mod, 4096, SEEK_SET);
	int x = 42;
	fwrite (&x, sizeof(x), 1, mod);
	fclose (mod);

	mod = fopen("build/test/verify_test2/audio.mxf", "r+b");
	BOOST_REQUIRE (mod);
	BOOST_REQUIRE_EQUAL (fseek(mod, -64, SEEK_END), 0);
	BOOST_REQUIRE (fwrite (&x, sizeof(x), 1, mod) == 1);
	fclose (mod);

	list<dcp::VerificationNote> notes;
	{
		dcp::ASDCPErrorSuspender sus;
		notes = dcp::verify (directories, &stage, &progress, xsd_test);
	}

	BOOST_REQUIRE_EQUAL (notes.size(), 2);
	BOOST_CHECK_EQUAL (notes.front().type(), dcp::VerificationNote::VERIFY_ERROR);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::PICTURE_HASH_INCORRECT);
	BOOST_CHECK_EQUAL (notes.back().type(), dcp::VerificationNote::VERIFY_ERROR);
	BOOST_CHECK_EQUAL (notes.back().code(), dcp::VerificationNote::SOUND_HASH_INCORRECT);
}

/* Corrupt the hashes in the PKL and check that the disagreement between CPL and PKL is spotted */
BOOST_AUTO_TEST_CASE (verify_test3)
{
	vector<boost::filesystem::path> directories = setup (1, 3);

	{
		Editor e ("build/test/verify_test3/pkl_cd49971e-bf4c-4594-8474-54ebef09a40c.xml");
		e.replace ("<Hash>", "<Hash>x");
	}

	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, xsd_test);

	BOOST_REQUIRE_EQUAL (notes.size(), 6);
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
	BOOST_CHECK_EQUAL (i->type(), dcp::VerificationNote::VERIFY_ERROR);
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::XML_VALIDATION_ERROR);
	++i;
	BOOST_CHECK_EQUAL (i->type(), dcp::VerificationNote::VERIFY_ERROR);
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::XML_VALIDATION_ERROR);
	++i;
	BOOST_CHECK_EQUAL (i->type(), dcp::VerificationNote::VERIFY_ERROR);
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::XML_VALIDATION_ERROR);
	++i;
}

/* Corrupt the ContentKind in the CPL */
BOOST_AUTO_TEST_CASE (verify_test4)
{
	vector<boost::filesystem::path> directories = setup (1, 4);

	{
		Editor e ("build/test/verify_test4/cpl_81fb54df-e1bf-4647-8788-ea7ba154375b.xml");
		e.replace ("<ContentKind>", "<ContentKind>x");
	}

	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, xsd_test);

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
boost::filesystem::path
pkl (int n)
{
	return dcp::String::compose("build/test/verify_test%1/pkl_cd49971e-bf4c-4594-8474-54ebef09a40c.xml", n);
}

static
boost::filesystem::path
asset_map (int n)
{
	return dcp::String::compose("build/test/verify_test%1/ASSETMAP.xml", n);
}

static
void check_after_replace (int n, boost::function<boost::filesystem::path (int)> file, string from, string to, dcp::VerificationNote::Code code1)
{
	vector<boost::filesystem::path> directories = setup (1, n);

	{
		Editor e (file(n));
		e.replace (from, to);
	}

	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, xsd_test);

	BOOST_REQUIRE_EQUAL (notes.size(), 1);
	BOOST_CHECK_EQUAL (notes.front().code(), code1);
}

static
void check_after_replace (int n, boost::function<boost::filesystem::path (int)> file, string from, string to, dcp::VerificationNote::Code code1, dcp::VerificationNote::Code code2)
{
	vector<boost::filesystem::path> directories = setup (1, n);

	{
		Editor e (file(n));
		e.replace (from, to);
	}

	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, xsd_test);

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
	vector<boost::filesystem::path> directories = setup (1, n);

	{
		Editor e (file(n));
		e.replace (from, to);
	}

	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, xsd_test);

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
	vector<boost::filesystem::path> directories = setup (1, 6);

	boost::filesystem::remove ("build/test/verify_test6/video.mxf");
	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, xsd_test);

	BOOST_REQUIRE_EQUAL (notes.size(), 1);
	BOOST_CHECK_EQUAL (notes.front().type(), dcp::VerificationNote::VERIFY_ERROR);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::MISSING_ASSET);
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
			dcp::VerificationNote::EMPTY_ASSET_PATH
			);
}

/* Mismatched standard */
BOOST_AUTO_TEST_CASE (verify_test8)
{
	check_after_replace (
			8, &cpl,
			"http://www.smpte-ra.org/schemas/429-7/2006/CPL", "http://www.digicine.com/PROTO-ASDCP-CPL-20040511#",
			dcp::VerificationNote::MISMATCHED_STANDARD,
			dcp::VerificationNote::XML_VALIDATION_ERROR,
			dcp::VerificationNote::CPL_HASH_INCORRECT
			);
}

/* Badly formatted <Id> in CPL */
BOOST_AUTO_TEST_CASE (verify_test9)
{
	/* There's no CPL_HASH_INCORRECT error here because it can't find the correct hash by ID (since the ID is wrong) */
	check_after_replace (
			9, &cpl,
			"<Id>urn:uuid:81fb54df-e1bf-4647-8788-ea7ba154375b", "<Id>urn:uuid:81fb54df-e1bf-4647-8788-ea7ba154375",
			dcp::VerificationNote::XML_VALIDATION_ERROR
			);
}

/* Badly formatted <IssueDate> in CPL */
BOOST_AUTO_TEST_CASE (verify_test10)
{
	check_after_replace (
			10, &cpl,
			"<IssueDate>", "<IssueDate>x",
			dcp::VerificationNote::XML_VALIDATION_ERROR,
			dcp::VerificationNote::CPL_HASH_INCORRECT
			);
}

/* Badly-formatted <Id> in PKL */
BOOST_AUTO_TEST_CASE (verify_test11)
{
	check_after_replace (
		11, &pkl,
		"<Id>urn:uuid:cd4", "<Id>urn:uuid:xd4",
		dcp::VerificationNote::XML_VALIDATION_ERROR
		);
}

/* Badly-formatted <Id> in ASSETMAP */
BOOST_AUTO_TEST_CASE (verify_test12)
{
	check_after_replace (
		12, &asset_map,
		"<Id>urn:uuid:63c", "<Id>urn:uuid:x3c",
		dcp::VerificationNote::XML_VALIDATION_ERROR
		);
}

/* Basic test of an Interop DCP */
BOOST_AUTO_TEST_CASE (verify_test13)
{
	stages.clear ();
	vector<boost::filesystem::path> directories = setup (3, 13);
	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, xsd_test);

	boost::filesystem::path const cpl_file = "build/test/verify_test13/cpl_cbfd2bc0-21cf-4a8f-95d8-9cddcbe51296.xml";
	boost::filesystem::path const pkl_file = "build/test/verify_test13/pkl_d87a950c-bd6f-41f6-90cc-56ccd673e131.xml";
	boost::filesystem::path const assetmap_file = "build/test/verify_test13/ASSETMAP";

	list<pair<string, optional<boost::filesystem::path> > >::const_iterator st = stages.begin();
	BOOST_CHECK_EQUAL (st->first, "Checking DCP");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical("build/test/verify_test13"));
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
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical("build/test/verify_test13/j2c_c6035f97-b07d-4e1c-944d-603fc2ddc242.mxf"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking picture frame sizes");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical("build/test/verify_test13/j2c_c6035f97-b07d-4e1c-944d-603fc2ddc242.mxf"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking sound asset hash");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical("build/test/verify_test13/pcm_69cf9eaf-9a99-4776-b022-6902208626c3.mxf"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking PKL");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical(pkl_file));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking ASSETMAP");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical(assetmap_file));
	++st;
	BOOST_REQUIRE (st == stages.end());

	BOOST_REQUIRE_EQUAL (notes.size(), 1U);
	list<dcp::VerificationNote>::const_iterator i = notes.begin ();
	BOOST_CHECK_EQUAL (i->type(), dcp::VerificationNote::VERIFY_BV21_ERROR);
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::NOT_SMPTE);
}

/* DCP with a short asset */
BOOST_AUTO_TEST_CASE (verify_test14)
{
	vector<boost::filesystem::path> directories = setup (8, 14);
	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, xsd_test);

	BOOST_REQUIRE_EQUAL (notes.size(), 5);
	list<dcp::VerificationNote>::const_iterator i = notes.begin ();
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::NOT_SMPTE);
	++i;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::DURATION_TOO_SMALL);
	++i;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::INTRINSIC_DURATION_TOO_SMALL);
	++i;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::DURATION_TOO_SMALL);
	++i;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::INTRINSIC_DURATION_TOO_SMALL);
	++i;
}


static
void
dcp_from_frame (dcp::ArrayData const& frame, boost::filesystem::path dir)
{
	shared_ptr<dcp::MonoPictureAsset> asset(new dcp::MonoPictureAsset(dcp::Fraction(24, 1), dcp::SMPTE));
	boost::filesystem::create_directories (dir);
	shared_ptr<dcp::PictureAssetWriter> writer = asset->start_write (dir / "pic.mxf", true);
	for (int i = 0; i < 24; ++i) {
		writer->write (frame.data(), frame.size());
	}
	writer->finalize ();

	shared_ptr<dcp::ReelAsset> reel_asset(new dcp::ReelMonoPictureAsset(asset, 0));
	shared_ptr<dcp::Reel> reel(new dcp::Reel());
	reel->add (reel_asset);
	shared_ptr<dcp::CPL> cpl(new dcp::CPL("hello", dcp::FEATURE));
	cpl->add (reel);
	shared_ptr<dcp::DCP> dcp(new dcp::DCP(dir));
	dcp->add (cpl);
	dcp->write_xml (dcp::SMPTE);
}


/* DCP with an over-sized JPEG2000 frame */
BOOST_AUTO_TEST_CASE (verify_test15)
{
	int const too_big = 1302083 * 2;

	/* Compress a black image */
	shared_ptr<dcp::OpenJPEGImage> image = black_image ();
	dcp::ArrayData frame = dcp::compress_j2k (image, 100000000, 24, false, false);
	BOOST_REQUIRE (frame.size() < too_big);

	/* Place it in a bigger block with some zero padding at the end */
	dcp::ArrayData oversized_frame(too_big);
	memcpy (oversized_frame.data(), frame.data(), frame.size());
	memset (oversized_frame.data() + frame.size(), 0, too_big - frame.size());

	boost::filesystem::path const dir("build/test/verify_test15");
	boost::filesystem::remove_all (dir);
	dcp_from_frame (oversized_frame, dir);

	vector<boost::filesystem::path> dirs;
	dirs.push_back (dir);
	list<dcp::VerificationNote> notes = dcp::verify (dirs, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), 1);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::PICTURE_FRAME_TOO_LARGE);
}


/* DCP with a nearly over-sized JPEG2000 frame */
BOOST_AUTO_TEST_CASE (verify_test16)
{
	int const nearly_too_big = 1302083 * 0.98;

	/* Compress a black image */
	shared_ptr<dcp::OpenJPEGImage> image = black_image ();
	dcp::ArrayData frame = dcp::compress_j2k (image, 100000000, 24, false, false);
	BOOST_REQUIRE (frame.size() < nearly_too_big);

	/* Place it in a bigger block with some zero padding at the end */
	dcp::ArrayData oversized_frame(nearly_too_big);
	memcpy (oversized_frame.data(), frame.data(), frame.size());
	memset (oversized_frame.data() + frame.size(), 0, nearly_too_big - frame.size());

	boost::filesystem::path const dir("build/test/verify_test16");
	boost::filesystem::remove_all (dir);
	dcp_from_frame (oversized_frame, dir);

	vector<boost::filesystem::path> dirs;
	dirs.push_back (dir);
	list<dcp::VerificationNote> notes = dcp::verify (dirs, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), 1);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::PICTURE_FRAME_NEARLY_TOO_LARGE);
}


/* DCP with a within-range JPEG2000 frame */
BOOST_AUTO_TEST_CASE (verify_test17)
{
	/* Compress a black image */
	shared_ptr<dcp::OpenJPEGImage> image = black_image ();
	dcp::ArrayData frame = dcp::compress_j2k (image, 100000000, 24, false, false);
	BOOST_REQUIRE (frame.size() < 230000000 / (24 * 8));

	boost::filesystem::path const dir("build/test/verify_test17");
	boost::filesystem::remove_all (dir);
	dcp_from_frame (frame, dir);

	vector<boost::filesystem::path> dirs;
	dirs.push_back (dir);
	list<dcp::VerificationNote> notes = dcp::verify (dirs, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), 0);
}


/* DCP with valid Interop subtitles */
BOOST_AUTO_TEST_CASE (verify_test18)
{
	boost::filesystem::path const dir("build/test/verify_test18");
	boost::filesystem::remove_all (dir);
	boost::filesystem::create_directories (dir);
	boost::filesystem::copy_file ("test/data/subs1.xml", dir / "subs.xml");
	shared_ptr<dcp::InteropSubtitleAsset> asset(new dcp::InteropSubtitleAsset(dir / "subs.xml"));
	shared_ptr<dcp::ReelAsset> reel_asset(new dcp::ReelSubtitleAsset(asset, dcp::Fraction(24, 1), 16 * 24, 0));
	shared_ptr<dcp::Reel> reel(new dcp::Reel());
	reel->add (reel_asset);
	shared_ptr<dcp::CPL> cpl(new dcp::CPL("hello", dcp::FEATURE));
	cpl->add (reel);
	shared_ptr<dcp::DCP> dcp(new dcp::DCP(dir));
	dcp->add (cpl);
	dcp->write_xml (dcp::INTEROP);

	vector<boost::filesystem::path> dirs;
	dirs.push_back (dir);
	list<dcp::VerificationNote> notes = dcp::verify (dirs, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), 1U);
	list<dcp::VerificationNote>::const_iterator i = notes.begin ();
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::NOT_SMPTE);
}


/* DCP with broken Interop subtitles */
BOOST_AUTO_TEST_CASE (verify_test19)
{
	boost::filesystem::path const dir("build/test/verify_test19");
	boost::filesystem::remove_all (dir);
	boost::filesystem::create_directories (dir);
	boost::filesystem::copy_file ("test/data/subs1.xml", dir / "subs.xml");
	shared_ptr<dcp::InteropSubtitleAsset> asset(new dcp::InteropSubtitleAsset(dir / "subs.xml"));
	shared_ptr<dcp::ReelAsset> reel_asset(new dcp::ReelSubtitleAsset(asset, dcp::Fraction(24, 1), 16 * 24, 0));
	shared_ptr<dcp::Reel> reel(new dcp::Reel());
	reel->add (reel_asset);
	shared_ptr<dcp::CPL> cpl(new dcp::CPL("hello", dcp::FEATURE));
	cpl->add (reel);
	shared_ptr<dcp::DCP> dcp(new dcp::DCP(dir));
	dcp->add (cpl);
	dcp->write_xml (dcp::INTEROP);

	{
		Editor e (dir / "subs.xml");
		e.replace ("</ReelNumber>", "</ReelNumber><Foo></Foo>");
	}

	vector<boost::filesystem::path> dirs;
	dirs.push_back (dir);
	list<dcp::VerificationNote> notes = dcp::verify (dirs, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), 3);
	list<dcp::VerificationNote>::const_iterator i = notes.begin ();
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::NOT_SMPTE);
	++i;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::XML_VALIDATION_ERROR);
	++i;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::XML_VALIDATION_ERROR);
	++i;
}


/* DCP with valid SMPTE subtitles */
BOOST_AUTO_TEST_CASE (verify_test20)
{
	boost::filesystem::path const dir("build/test/verify_test20");
	boost::filesystem::remove_all (dir);
	boost::filesystem::create_directories (dir);
	boost::filesystem::copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	shared_ptr<dcp::SMPTESubtitleAsset> asset(new dcp::SMPTESubtitleAsset(dir / "subs.mxf"));
	shared_ptr<dcp::ReelAsset> reel_asset(new dcp::ReelSubtitleAsset(asset, dcp::Fraction(24, 1), 16 * 24, 0));
	shared_ptr<dcp::Reel> reel(new dcp::Reel());
	reel->add (reel_asset);
	shared_ptr<dcp::CPL> cpl(new dcp::CPL("hello", dcp::FEATURE));
	cpl->add (reel);
	shared_ptr<dcp::DCP> dcp(new dcp::DCP(dir));
	dcp->add (cpl);
	dcp->write_xml (dcp::SMPTE);

	vector<boost::filesystem::path> dirs;
	dirs.push_back (dir);
	list<dcp::VerificationNote> notes = dcp::verify (dirs, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), 0);
}


/* DCP with broken SMPTE subtitles */
BOOST_AUTO_TEST_CASE (verify_test21)
{
	boost::filesystem::path const dir("build/test/verify_test21");
	boost::filesystem::remove_all (dir);
	boost::filesystem::create_directories (dir);
	boost::filesystem::copy_file ("test/data/broken_smpte.mxf", dir / "subs.mxf");
	shared_ptr<dcp::SMPTESubtitleAsset> asset(new dcp::SMPTESubtitleAsset(dir / "subs.mxf"));
	shared_ptr<dcp::ReelAsset> reel_asset(new dcp::ReelSubtitleAsset(asset, dcp::Fraction(24, 1), 16 * 24, 0));
	shared_ptr<dcp::Reel> reel(new dcp::Reel());
	reel->add (reel_asset);
	shared_ptr<dcp::CPL> cpl(new dcp::CPL("hello", dcp::FEATURE));
	cpl->add (reel);
	shared_ptr<dcp::DCP> dcp(new dcp::DCP(dir));
	dcp->add (cpl);
	dcp->write_xml (dcp::SMPTE);

	vector<boost::filesystem::path> dirs;
	dirs.push_back (dir);
	list<dcp::VerificationNote> notes = dcp::verify (dirs, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), 2);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::XML_VALIDATION_ERROR);
	BOOST_CHECK_EQUAL (notes.back().code(), dcp::VerificationNote::XML_VALIDATION_ERROR);
}


/* VF */
BOOST_AUTO_TEST_CASE (verify_test22)
{
	boost::filesystem::path const ov_dir("build/test/verify_test22_ov");
	boost::filesystem::remove_all (ov_dir);
	boost::filesystem::create_directories (ov_dir);

	shared_ptr<dcp::OpenJPEGImage> image = black_image ();
	dcp::ArrayData frame = dcp::compress_j2k (image, 100000000, 24, false, false);
	BOOST_REQUIRE (frame.size() < 230000000 / (24 * 8));
	dcp_from_frame (frame, ov_dir);

	dcp::DCP ov (ov_dir);
	ov.read ();

	boost::filesystem::path const vf_dir("build/test/verify_test22_vf");
	boost::filesystem::remove_all (vf_dir);
	boost::filesystem::create_directories (vf_dir);

	shared_ptr<dcp::Reel> reel(new dcp::Reel());
	reel->add (ov.cpls().front()->reels().front()->main_picture());
	shared_ptr<dcp::CPL> cpl(new dcp::CPL("hello", dcp::FEATURE));
	cpl->add (reel);
	dcp::DCP vf (vf_dir);
	vf.add (cpl);
	vf.write_xml (dcp::SMPTE);

	vector<boost::filesystem::path> dirs;
	dirs.push_back (vf_dir);
	list<dcp::VerificationNote> notes = dcp::verify (dirs, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), 1);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::EXTERNAL_ASSET);
}


/* DCP with valid CompositionMetadataAsset */
BOOST_AUTO_TEST_CASE (verify_test23)
{
	boost::filesystem::path const dir("build/test/verify_test23");
	boost::filesystem::remove_all (dir);
	boost::filesystem::create_directories (dir);

	boost::filesystem::copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	shared_ptr<dcp::SMPTESubtitleAsset> asset(new dcp::SMPTESubtitleAsset(dir / "subs.mxf"));
	shared_ptr<dcp::ReelAsset> reel_asset(new dcp::ReelSubtitleAsset(asset, dcp::Fraction(24, 1), 16 * 24, 0));

	shared_ptr<dcp::Reel> reel(new dcp::Reel());
	reel->add (reel_asset);
	shared_ptr<dcp::CPL> cpl(new dcp::CPL("hello", dcp::FEATURE));
	cpl->add (reel);
	cpl->set_main_sound_configuration ("L,C,R,Lfe,-,-");
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));

	dcp::DCP dcp (dir);
	dcp.add (cpl);
	dcp.write_xml (dcp::SMPTE);

	vector<boost::filesystem::path> dirs;
	dirs.push_back (dir);
	list<dcp::VerificationNote> notes = dcp::verify (dirs, &stage, &progress, xsd_test);
}


boost::filesystem::path find_cpl (boost::filesystem::path dir)
{
	for (boost::filesystem::directory_iterator i = boost::filesystem::directory_iterator(dir); i != boost::filesystem::directory_iterator(); i++) {
		if (boost::starts_with(i->path().filename().string(), "cpl_")) {
			return i->path();
		}
	}

	BOOST_REQUIRE (false);
	return boost::filesystem::path();
}


/* DCP with invalid CompositionMetadataAsset */
BOOST_AUTO_TEST_CASE (verify_test24)
{
	boost::filesystem::path const dir("build/test/verify_test24");
	boost::filesystem::remove_all (dir);
	boost::filesystem::create_directories (dir);

	shared_ptr<dcp::Reel> reel(new dcp::Reel());
	reel->add (black_picture_asset(dir));
	shared_ptr<dcp::CPL> cpl(new dcp::CPL("hello", dcp::FEATURE));
	cpl->add (reel);
	cpl->set_main_sound_configuration ("L,C,R,Lfe,-,-");
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));

	dcp::DCP dcp (dir);
	dcp.add (cpl);
	dcp.write_xml (dcp::SMPTE);

	{
		Editor e (find_cpl("build/test/verify_test24"));
		e.replace ("MainSound", "MainSoundX");
	}

	vector<boost::filesystem::path> dirs;
	dirs.push_back (dir);
	list<dcp::VerificationNote> notes = dcp::verify (dirs, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), 4);

	list<dcp::VerificationNote>::const_iterator i = notes.begin ();
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::XML_VALIDATION_ERROR);
	++i;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::XML_VALIDATION_ERROR);
	++i;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::XML_VALIDATION_ERROR);
	++i;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::CPL_HASH_INCORRECT);
	++i;
}


/* DCP with invalid CompositionMetadataAsset */
BOOST_AUTO_TEST_CASE (verify_test25)
{
	boost::filesystem::path const dir("build/test/verify_test25");
	boost::filesystem::remove_all (dir);
	boost::filesystem::create_directories (dir);

	boost::filesystem::copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	shared_ptr<dcp::SMPTESubtitleAsset> asset(new dcp::SMPTESubtitleAsset(dir / "subs.mxf"));
	shared_ptr<dcp::ReelAsset> reel_asset(new dcp::ReelSubtitleAsset(asset, dcp::Fraction(24, 1), 16 * 24, 0));

	shared_ptr<dcp::Reel> reel(new dcp::Reel());
	reel->add (reel_asset);
	shared_ptr<dcp::CPL> cpl(new dcp::CPL("hello", dcp::FEATURE));
	cpl->add (reel);
	cpl->set_main_sound_configuration ("L,C,R,Lfe,-,-");
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));

	dcp::DCP dcp (dir);
	dcp.add (cpl);
	dcp.write_xml (dcp::SMPTE);

	{
		Editor e (find_cpl("build/test/verify_test25"));
		e.replace ("</MainPictureActiveArea>", "</MainPictureActiveArea><BadTag></BadTag>");
	}

	vector<boost::filesystem::path> dirs;
	dirs.push_back (dir);
	list<dcp::VerificationNote> notes = dcp::verify (dirs, &stage, &progress, xsd_test);
}


/* SMPTE DCP with invalid <Language> in the MainSubtitle */
BOOST_AUTO_TEST_CASE (verify_test26)
{
	boost::filesystem::path const dir("build/test/verify_test26");
	boost::filesystem::remove_all (dir);
	boost::filesystem::create_directories (dir);
	boost::filesystem::copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	shared_ptr<dcp::SMPTESubtitleAsset> asset(new dcp::SMPTESubtitleAsset(dir / "subs.mxf"));
	shared_ptr<dcp::ReelSubtitleAsset> reel_asset(new dcp::ReelSubtitleAsset(asset, dcp::Fraction(24, 1), 16 * 24, 0));
	reel_asset->_language = "badlang";
	shared_ptr<dcp::Reel> reel(new dcp::Reel());
	reel->add (reel_asset);
	shared_ptr<dcp::CPL> cpl(new dcp::CPL("hello", dcp::FEATURE));
	cpl->add (reel);
	shared_ptr<dcp::DCP> dcp(new dcp::DCP(dir));
	dcp->add (cpl);
	dcp->write_xml (dcp::SMPTE);

	vector<boost::filesystem::path> dirs;
	dirs.push_back (dir);
	list<dcp::VerificationNote> notes = dcp::verify (dirs, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), 1U);
	list<dcp::VerificationNote>::const_iterator i = notes.begin ();
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::BAD_LANGUAGE);

}

