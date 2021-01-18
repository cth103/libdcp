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

#include "verify.h"
#include "util.h"
#include "j2k.h"
#include "reel.h"
#include "reel_mono_picture_asset.h"
#include "reel_sound_asset.h"
#include "cpl.h"
#include "dcp.h"
#include "openjpeg_image.h"
#include "mono_picture_asset.h"
#include "stereo_picture_asset.h"
#include "mono_picture_asset_writer.h"
#include "interop_subtitle_asset.h"
#include "smpte_subtitle_asset.h"
#include "reel_closed_caption_asset.h"
#include "reel_stereo_picture_asset.h"
#include "reel_subtitle_asset.h"
#include "reel_markers_asset.h"
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
using std::make_shared;
using boost::optional;
using std::shared_ptr;


static list<pair<string, optional<boost::filesystem::path>>> stages;
static string const dcp_test1_pkl = "pkl_2b9b857f-ab4a-440e-a313-1ace0f1cfc95.xml";
static string const dcp_test1_cpl = "cpl_81fb54df-e1bf-4647-8788-ea7ba154375b.xml";

static void
stage (string s, optional<boost::filesystem::path> p)
{
	stages.push_back (make_pair (s, p));
}

static void
progress (float)
{

}

static void
prepare_directory (boost::filesystem::path path)
{
	using namespace boost::filesystem;
	remove_all (path);
	create_directories (path);
}


static vector<boost::filesystem::path>
setup (int reference_number, int verify_test_number)
{
	prepare_directory (dcp::String::compose("build/test/verify_test%1", verify_test_number));
	for (auto i: boost::filesystem::directory_iterator(dcp::String::compose("test/ref/DCP/dcp_test%1", reference_number))) {
		boost::filesystem::copy_file (i.path(), dcp::String::compose("build/test/verify_test%1", verify_test_number) / i.path().filename());
	}

	return { dcp::String::compose("build/test/verify_test%1", verify_test_number) };

}


static
void
write_dcp_with_single_asset (boost::filesystem::path dir, shared_ptr<dcp::ReelAsset> reel_asset, dcp::Standard standard = dcp::SMPTE)
{
	auto reel = make_shared<dcp::Reel>();
	reel->add (reel_asset);
	reel->add (simple_markers());

	auto cpl = make_shared<dcp::CPL>("hello", dcp::TRAILER);
	cpl->add (reel);
	auto dcp = make_shared<dcp::DCP>(dir);
	dcp->add (cpl);
	dcp->write_xml (
		standard,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"hello"
		);
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
		auto f = fopen(_path.string().c_str(), "w");
		BOOST_REQUIRE (f);
		fwrite (_content.c_str(), _content.length(), 1, f);
		fclose (f);
	}

	void replace (string a, string b)
	{
		auto old_content = _content;
		boost::algorithm::replace_all (_content, a, b);
		BOOST_REQUIRE (_content != old_content);
	}

	void delete_lines (string from, string to)
	{
		vector<string> lines;
		boost::algorithm::split (lines, _content, boost::is_any_of("\r\n"), boost::token_compress_on);
		bool deleting = false;
		auto old_content = _content;
		_content = "";
		for (auto i: lines) {
			if (i.find(from) != string::npos) {
				deleting = true;
			}
			if (!deleting) {
				_content += i + "\n";
			}
			if (deleting && i.find(to) != string::npos) {
				deleting = false;
			}
		}
		BOOST_REQUIRE (_content != old_content);
	}

private:
	boost::filesystem::path _path;
	std::string _content;
};


static
void
dump_notes (vector<dcp::VerificationNote> const & notes)
{
	for (auto i: notes) {
		std::cout << dcp::note_to_string(i) << "\n";
	}
}


static
void
check_verify_result (vector<boost::filesystem::path> dir, vector<dcp::VerificationNote> test_notes)
{
	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), test_notes.size());
}


static
void
check_verify_result_after_replace (int n, boost::function<boost::filesystem::path (int)> file, string from, string to, vector<dcp::VerificationNote::Code> codes)
{
	auto directories = setup (1, n);

	{
		Editor e (file(n));
		e.replace (from, to);
	}

	auto notes = dcp::verify (directories, &stage, &progress, xsd_test);

	BOOST_REQUIRE_EQUAL (notes.size(), codes.size());
	auto i = notes.begin();
	auto j = codes.begin();
	while (i != notes.end()) {
		BOOST_CHECK_EQUAL (i->code(), *j);
		++i;
		++j;
	}
}


/* Check DCP as-is (should be OK) */
BOOST_AUTO_TEST_CASE (verify_test1)
{
	stages.clear ();
	auto directories = setup (1, 1);
	auto notes = dcp::verify (directories, &stage, &progress, xsd_test);

	boost::filesystem::path const cpl_file = boost::filesystem::path("build") / "test" / "verify_test1" / dcp_test1_cpl;
	boost::filesystem::path const pkl_file = boost::filesystem::path("build") / "test" / "verify_test1" / dcp_test1_pkl;
	boost::filesystem::path const assetmap_file = "build/test/verify_test1/ASSETMAP.xml";

	auto st = stages.begin();
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
	BOOST_CHECK_EQUAL (st->first, "Checking sound asset metadata");
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
	auto directories = setup (1, 2);

	auto mod = fopen("build/test/verify_test2/video.mxf", "r+b");
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

	dcp::ASDCPErrorSuspender sus;
	check_verify_result (
		directories,
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::PICTURE_HASH_INCORRECT },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::SOUND_HASH_INCORRECT }
		});
}

/* Corrupt the hashes in the PKL and check that the disagreement between CPL and PKL is spotted */
BOOST_AUTO_TEST_CASE (verify_test3)
{
	auto directories = setup (1, 3);

	{
		Editor e (boost::filesystem::path("build") / "test" / "verify_test3" / dcp_test1_pkl);
		e.replace ("<Hash>", "<Hash>x");
	}

	check_verify_result (
		directories,
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::CPL_HASH_INCORRECT },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::PKL_CPL_PICTURE_HASHES_DIFFER },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::PKL_CPL_SOUND_HASHES_DIFFER },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR }
		});
}

/* Corrupt the ContentKind in the CPL */
BOOST_AUTO_TEST_CASE (verify_test4)
{
	auto directories = setup (1, 4);

	{
		Editor e (boost::filesystem::path("build") / "test" / "verify_test4" / dcp_test1_cpl);
		e.replace ("<ContentKind>", "<ContentKind>x");
	}

	auto notes = dcp::verify (directories, &stage, &progress, xsd_test);

	BOOST_REQUIRE_EQUAL (notes.size(), 1);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::GENERAL_READ);
	BOOST_CHECK_EQUAL (*notes.front().note(), "Bad content kind 'xtrailer'");
}

static
boost::filesystem::path
cpl (int n)
{
	return dcp::String::compose("build/test/verify_test%1/%2", n, dcp_test1_cpl);
}

static
boost::filesystem::path
pkl (int n)
{
	return dcp::String::compose("build/test/verify_test%1/%2", n, dcp_test1_pkl);
}

static
boost::filesystem::path
asset_map (int n)
{
	return dcp::String::compose("build/test/verify_test%1/ASSETMAP.xml", n);
}


/* FrameRate */
BOOST_AUTO_TEST_CASE (verify_test5)
{
	check_verify_result_after_replace (
			5, &cpl,
			"<FrameRate>24 1", "<FrameRate>99 1",
			{ dcp::VerificationNote::CPL_HASH_INCORRECT,
			  dcp::VerificationNote::INVALID_PICTURE_FRAME_RATE }
			);
}

/* Missing asset */
BOOST_AUTO_TEST_CASE (verify_test6)
{
	auto directories = setup (1, 6);

	boost::filesystem::remove ("build/test/verify_test6/video.mxf");
	check_verify_result (directories, {{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::MISSING_ASSET }});
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
	check_verify_result_after_replace (
			7, &assetmap,
			"<Path>video.mxf</Path>", "<Path></Path>",
			{ dcp::VerificationNote::EMPTY_ASSET_PATH }
			);
}

/* Mismatched standard */
BOOST_AUTO_TEST_CASE (verify_test8)
{
	check_verify_result_after_replace (
			8, &cpl,
			"http://www.smpte-ra.org/schemas/429-7/2006/CPL", "http://www.digicine.com/PROTO-ASDCP-CPL-20040511#",
			{ dcp::VerificationNote::MISMATCHED_STANDARD,
			  dcp::VerificationNote::XML_VALIDATION_ERROR,
			  dcp::VerificationNote::XML_VALIDATION_ERROR,
			  dcp::VerificationNote::XML_VALIDATION_ERROR,
			  dcp::VerificationNote::XML_VALIDATION_ERROR,
			  dcp::VerificationNote::XML_VALIDATION_ERROR,
			  dcp::VerificationNote::CPL_HASH_INCORRECT }
			);
}

/* Badly formatted <Id> in CPL */
BOOST_AUTO_TEST_CASE (verify_test9)
{
	/* There's no CPL_HASH_INCORRECT error here because it can't find the correct hash by ID (since the ID is wrong) */
	check_verify_result_after_replace (
			9, &cpl,
			"<Id>urn:uuid:81fb54df-e1bf-4647-8788-ea7ba154375b", "<Id>urn:uuid:81fb54df-e1bf-4647-8788-ea7ba154375",
			{ dcp::VerificationNote::XML_VALIDATION_ERROR }
			);
}

/* Badly formatted <IssueDate> in CPL */
BOOST_AUTO_TEST_CASE (verify_test10)
{
	check_verify_result_after_replace (
			10, &cpl,
			"<IssueDate>", "<IssueDate>x",
			{ dcp::VerificationNote::XML_VALIDATION_ERROR,
			  dcp::VerificationNote::CPL_HASH_INCORRECT }
			);
}

/* Badly-formatted <Id> in PKL */
BOOST_AUTO_TEST_CASE (verify_test11)
{
	check_verify_result_after_replace (
		11, &pkl,
		"<Id>urn:uuid:2b9", "<Id>urn:uuid:xb9",
		{ dcp::VerificationNote::XML_VALIDATION_ERROR }
		);
}

/* Badly-formatted <Id> in ASSETMAP */
BOOST_AUTO_TEST_CASE (verify_test12)
{
	check_verify_result_after_replace (
		12, &asset_map,
		"<Id>urn:uuid:07e", "<Id>urn:uuid:x7e",
		{ dcp::VerificationNote::XML_VALIDATION_ERROR }
		);
}

/* Basic test of an Interop DCP */
BOOST_AUTO_TEST_CASE (verify_test13)
{
	stages.clear ();
	auto directories = setup (3, 13);
	auto notes = dcp::verify (directories, &stage, &progress, xsd_test);

	boost::filesystem::path const cpl_file = boost::filesystem::path("build") / "test" / "verify_test13" / "cpl_cbfd2bc0-21cf-4a8f-95d8-9cddcbe51296.xml";
	boost::filesystem::path const pkl_file = boost::filesystem::path("build") / "test" / "verify_test13" / "pkl_d87a950c-bd6f-41f6-90cc-56ccd673e131.xml";
	boost::filesystem::path const assetmap_file = "build/test/verify_test13/ASSETMAP";

	auto st = stages.begin();
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
	BOOST_CHECK_EQUAL (st->first, "Checking sound asset metadata");
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
	auto i = notes.begin ();
	BOOST_CHECK_EQUAL (i->type(), dcp::VerificationNote::VERIFY_BV21_ERROR);
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::NOT_SMPTE);
}

/* DCP with a short asset */
BOOST_AUTO_TEST_CASE (verify_test14)
{
	auto directories = setup (8, 14);
	check_verify_result (
		directories,
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::NOT_SMPTE },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::DURATION_TOO_SMALL },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::INTRINSIC_DURATION_TOO_SMALL },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::DURATION_TOO_SMALL },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::INTRINSIC_DURATION_TOO_SMALL }
		});
}


static
void
dcp_from_frame (dcp::ArrayData const& frame, boost::filesystem::path dir)
{
	auto asset = make_shared<dcp::MonoPictureAsset>(dcp::Fraction(24, 1), dcp::SMPTE);
	boost::filesystem::create_directories (dir);
	auto writer = asset->start_write (dir / "pic.mxf", true);
	for (int i = 0; i < 24; ++i) {
		writer->write (frame.data(), frame.size());
	}
	writer->finalize ();

	auto reel_asset = make_shared<dcp::ReelMonoPictureAsset>(asset, 0);
	write_dcp_with_single_asset (dir, reel_asset);
}


/* DCP with an over-sized JPEG2000 frame */
BOOST_AUTO_TEST_CASE (verify_test15)
{
	int const too_big = 1302083 * 2;

	/* Compress a black image */
	auto image = black_image ();
	auto frame = dcp::compress_j2k (image, 100000000, 24, false, false);
	BOOST_REQUIRE (frame.size() < too_big);

	/* Place it in a bigger block with some zero padding at the end */
	dcp::ArrayData oversized_frame(too_big);
	memcpy (oversized_frame.data(), frame.data(), frame.size());
	memset (oversized_frame.data() + frame.size(), 0, too_big - frame.size());

	boost::filesystem::path const dir("build/test/verify_test15");
	boost::filesystem::remove_all (dir);
	dcp_from_frame (oversized_frame, dir);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::PICTURE_FRAME_TOO_LARGE_IN_BYTES },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}


/* DCP with a nearly over-sized JPEG2000 frame */
BOOST_AUTO_TEST_CASE (verify_test16)
{
	int const nearly_too_big = 1302083 * 0.98;

	/* Compress a black image */
	auto image = black_image ();
	auto frame = dcp::compress_j2k (image, 100000000, 24, false, false);
	BOOST_REQUIRE (frame.size() < nearly_too_big);

	/* Place it in a bigger block with some zero padding at the end */
	dcp::ArrayData oversized_frame(nearly_too_big);
	memcpy (oversized_frame.data(), frame.data(), frame.size());
	memset (oversized_frame.data() + frame.size(), 0, nearly_too_big - frame.size());

	boost::filesystem::path const dir("build/test/verify_test16");
	boost::filesystem::remove_all (dir);
	dcp_from_frame (oversized_frame, dir);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::PICTURE_FRAME_NEARLY_TOO_LARGE_IN_BYTES },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}


/* DCP with a within-range JPEG2000 frame */
BOOST_AUTO_TEST_CASE (verify_test17)
{
	/* Compress a black image */
	auto image = black_image ();
	auto frame = dcp::compress_j2k (image, 100000000, 24, false, false);
	BOOST_REQUIRE (frame.size() < 230000000 / (24 * 8));

	boost::filesystem::path const dir("build/test/verify_test17");
	boost::filesystem::remove_all (dir);
	dcp_from_frame (frame, dir);

	check_verify_result ({ dir }, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }});
}


/* DCP with valid Interop subtitles */
BOOST_AUTO_TEST_CASE (verify_test18)
{
	boost::filesystem::path const dir("build/test/verify_test18");
	prepare_directory (dir);
	boost::filesystem::copy_file ("test/data/subs1.xml", dir / "subs.xml");
	auto asset = make_shared<dcp::InteropSubtitleAsset>(dir / "subs.xml");
	auto reel_asset = make_shared<dcp::ReelSubtitleAsset>(asset, dcp::Fraction(24, 1), 16 * 24, 0);
	write_dcp_with_single_asset (dir, reel_asset, dcp::INTEROP);

	check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::NOT_SMPTE }});
}


/* DCP with broken Interop subtitles */
BOOST_AUTO_TEST_CASE (verify_test19)
{
	boost::filesystem::path const dir("build/test/verify_test19");
	prepare_directory (dir);
	boost::filesystem::copy_file ("test/data/subs1.xml", dir / "subs.xml");
	auto asset = make_shared<dcp::InteropSubtitleAsset>(dir / "subs.xml");
	auto reel_asset = make_shared<dcp::ReelSubtitleAsset>(asset, dcp::Fraction(24, 1), 16 * 24, 0);
	write_dcp_with_single_asset (dir, reel_asset, dcp::INTEROP);

	{
		Editor e (dir / "subs.xml");
		e.replace ("</ReelNumber>", "</ReelNumber><Foo></Foo>");
	}

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::NOT_SMPTE },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR }
		});
}


/* DCP with valid SMPTE subtitles */
BOOST_AUTO_TEST_CASE (verify_test20)
{
	boost::filesystem::path const dir("build/test/verify_test20");
	prepare_directory (dir);
	boost::filesystem::copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTESubtitleAsset>(dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSubtitleAsset>(asset, dcp::Fraction(24, 1), 16 * 24, 0);
	write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }});
}


/* DCP with broken SMPTE subtitles */
BOOST_AUTO_TEST_CASE (verify_test21)
{
	boost::filesystem::path const dir("build/test/verify_test21");
	prepare_directory (dir);
	boost::filesystem::copy_file ("test/data/broken_smpte.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTESubtitleAsset>(dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSubtitleAsset>(asset, dcp::Fraction(24, 1), 16 * 24, 0);
	write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_SUBTITLE_START_TIME },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}


/* VF */
BOOST_AUTO_TEST_CASE (verify_test22)
{
	boost::filesystem::path const ov_dir("build/test/verify_test22_ov");
	prepare_directory (ov_dir);

	auto image = black_image ();
	auto frame = dcp::compress_j2k (image, 100000000, 24, false, false);
	BOOST_REQUIRE (frame.size() < 230000000 / (24 * 8));
	dcp_from_frame (frame, ov_dir);

	dcp::DCP ov (ov_dir);
	ov.read ();

	boost::filesystem::path const vf_dir("build/test/verify_test22_vf");
	prepare_directory (vf_dir);

	write_dcp_with_single_asset (vf_dir, ov.cpls().front()->reels().front()->main_picture());

	check_verify_result (
		{ vf_dir },
		{
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::EXTERNAL_ASSET },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}


/* DCP with valid CompositionMetadataAsset */
BOOST_AUTO_TEST_CASE (verify_test23)
{
	boost::filesystem::path const dir("build/test/verify_test23");
	prepare_directory (dir);

	boost::filesystem::copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTESubtitleAsset>(dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSubtitleAsset>(asset, dcp::Fraction(24, 1), 16 * 24, 0);

	auto reel = make_shared<dcp::Reel>();
	reel->add (reel_asset);

	reel->add (simple_markers(16 * 24 - 1));

	auto cpl = make_shared<dcp::CPL>("hello", dcp::TRAILER);
	cpl->add (reel);
	cpl->set_main_sound_configuration ("L,C,R,Lfe,-,-");
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));

	dcp::DCP dcp (dir);
	dcp.add (cpl);
	dcp.write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"hello"
		);

	check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }});
}


boost::filesystem::path find_cpl (boost::filesystem::path dir)
{
	for (auto i: boost::filesystem::directory_iterator(dir)) {
		if (boost::starts_with(i.path().filename().string(), "cpl_")) {
			return i.path();
		}
	}

	BOOST_REQUIRE (false);
	return {};
}


/* DCP with invalid CompositionMetadataAsset */
BOOST_AUTO_TEST_CASE (verify_test24)
{
	boost::filesystem::path const dir("build/test/verify_test24");
	prepare_directory (dir);

	auto reel = make_shared<dcp::Reel>();
	reel->add (black_picture_asset(dir));
	auto cpl = make_shared<dcp::CPL>("hello", dcp::TRAILER);
	cpl->add (reel);
	cpl->set_main_sound_configuration ("L,C,R,Lfe,-,-");
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));
	cpl->set_version_number (1);

	reel->add (simple_markers());

	dcp::DCP dcp (dir);
	dcp.add (cpl);
	dcp.write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"hello"
		);

	{
		Editor e (find_cpl("build/test/verify_test24"));
		e.replace ("MainSound", "MainSoundX");
	}

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::CPL_HASH_INCORRECT }
		});
}


/* DCP with invalid CompositionMetadataAsset */
BOOST_AUTO_TEST_CASE (verify_test25)
{
	boost::filesystem::path const dir("build/test/verify_test25");
	prepare_directory (dir);

	auto reel = make_shared<dcp::Reel>();
	reel->add (black_picture_asset(dir));
	auto cpl = make_shared<dcp::CPL>("hello", dcp::TRAILER);
	cpl->add (reel);
	cpl->set_main_sound_configuration ("L,C,R,Lfe,-,-");
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));

	dcp::DCP dcp (dir);
	dcp.add (cpl);
	dcp.write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"hello"
		);

	{
		Editor e (find_cpl("build/test/verify_test25"));
		e.replace ("meta:Width", "meta:WidthX");
	}

	check_verify_result (
		{ dir },
		{{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::GENERAL_READ }}
		);
}


/* SMPTE DCP with invalid <Language> in the MainSubtitle reel and also in the XML within the MXF */
BOOST_AUTO_TEST_CASE (verify_test26)
{
	boost::filesystem::path const dir("build/test/verify_test26");
	prepare_directory (dir);
	boost::filesystem::copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTESubtitleAsset>(dir / "subs.mxf");
	asset->_language = "wrong-andbad";
	asset->write (dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSubtitleAsset>(asset, dcp::Fraction(24, 1), 16 * 24, 0);
	reel_asset->_language = "badlang";
	write_dcp_with_single_asset (dir, reel_asset);

	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), 3U);
	auto i = notes.begin();
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::BAD_LANGUAGE);
	BOOST_REQUIRE (i->note());
	BOOST_CHECK_EQUAL (*i->note(), "badlang");
	i++;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::BAD_LANGUAGE);
	BOOST_REQUIRE (i->note());
	BOOST_CHECK_EQUAL (*i->note(), "wrong-andbad");
	i++;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::MISSING_CPL_METADATA);
}


/* SMPTE DCP with invalid <Language> in the MainClosedCaption reel and also in the XML within the MXF */
BOOST_AUTO_TEST_CASE (verify_invalid_closed_caption_languages)
{
	boost::filesystem::path const dir("build/test/verify_invalid_closed_caption_languages");
	prepare_directory (dir);
	boost::filesystem::copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTESubtitleAsset>(dir / "subs.mxf");
	asset->_language = "wrong-andbad";
	asset->write (dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelClosedCaptionAsset>(asset, dcp::Fraction(24, 1), 16 * 24, 0);
	reel_asset->_language = "badlang";
	write_dcp_with_single_asset (dir, reel_asset);

	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), 3U);
	auto i = notes.begin ();
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::BAD_LANGUAGE);
	BOOST_REQUIRE (i->note());
	BOOST_CHECK_EQUAL (*i->note(), "badlang");
	i++;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::BAD_LANGUAGE);
	BOOST_REQUIRE (i->note());
	BOOST_CHECK_EQUAL (*i->note(), "wrong-andbad");
	i++;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::MISSING_CPL_METADATA);
}


/* SMPTE DCP with invalid <Language> in the MainSound reel, the CPL additional subtitles languages and
 * the release territory.
 */
BOOST_AUTO_TEST_CASE (verify_various_invalid_languages)
{
	boost::filesystem::path const dir("build/test/verify_various_invalid_languages");
	prepare_directory (dir);

	auto picture = simple_picture (dir, "foo");
	auto reel_picture = make_shared<dcp::ReelMonoPictureAsset>(picture, 0);
	auto reel = make_shared<dcp::Reel>();
	reel->add (reel_picture);
	auto sound = simple_sound (dir, "foo", dcp::MXFMetadata(), "frobozz");
	auto reel_sound = make_shared<dcp::ReelSoundAsset>(sound, 0);
	reel->add (reel_sound);
	reel->add (simple_markers());

	auto cpl = make_shared<dcp::CPL>("hello", dcp::TRAILER);
	cpl->add (reel);
	cpl->_additional_subtitle_languages.push_back("this-is-wrong");
	cpl->_additional_subtitle_languages.push_back("andso-is-this");
	cpl->set_main_sound_configuration ("L,C,R,Lfe,-,-");
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));
	cpl->set_version_number (1);
	cpl->_release_territory = "fred-jim";
	auto dcp = make_shared<dcp::DCP>(dir);
	dcp->add (cpl);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"hello"
		);

	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), 4U);
	auto i = notes.begin ();
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::BAD_LANGUAGE);
	BOOST_REQUIRE (i->note());
	BOOST_CHECK_EQUAL (*i->note(), "this-is-wrong");
	++i;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::BAD_LANGUAGE);
	BOOST_REQUIRE (i->note());
	BOOST_CHECK_EQUAL (*i->note(), "andso-is-this");
	++i;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::BAD_LANGUAGE);
	BOOST_REQUIRE (i->note());
	BOOST_CHECK_EQUAL (*i->note(), "fred-jim");
	++i;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::BAD_LANGUAGE);
	BOOST_REQUIRE (i->note());
	BOOST_CHECK_EQUAL (*i->note(), "frobozz");
}


static
vector<dcp::VerificationNote>
check_picture_size (int width, int height, int frame_rate, bool three_d)
{
	using namespace boost::filesystem;

	path dcp_path = "build/test/verify_picture_test";
	remove_all (dcp_path);
	create_directories (dcp_path);

	shared_ptr<dcp::PictureAsset> mp;
	if (three_d) {
		mp = make_shared<dcp::StereoPictureAsset>(dcp::Fraction(frame_rate, 1), dcp::SMPTE);
	} else {
		mp = make_shared<dcp::MonoPictureAsset>(dcp::Fraction(frame_rate, 1), dcp::SMPTE);
	}
	auto picture_writer = mp->start_write (dcp_path / "video.mxf", false);

	auto image = black_image (dcp::Size(width, height));
	auto j2c = dcp::compress_j2k (image, 100000000, frame_rate, three_d, width > 2048);
	int const length = three_d ? frame_rate * 2 : frame_rate;
	for (int i = 0; i < length; ++i) {
		picture_writer->write (j2c.data(), j2c.size());
	}
	picture_writer->finalize ();

	auto d = make_shared<dcp::DCP>(dcp_path);
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::TRAILER);
	cpl->set_annotation_text ("A Test DCP");
	cpl->set_issue_date ("2012-07-17T04:45:18+00:00");
	cpl->set_main_sound_configuration ("L,C,R,Lfe,-,-");
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1998, 1080));
	cpl->set_version_number (1);

	auto reel = make_shared<dcp::Reel>();

	if (three_d) {
		reel->add (make_shared<dcp::ReelStereoPictureAsset>(std::dynamic_pointer_cast<dcp::StereoPictureAsset>(mp), 0));
	} else {
		reel->add (make_shared<dcp::ReelMonoPictureAsset>(std::dynamic_pointer_cast<dcp::MonoPictureAsset>(mp), 0));
	}

	reel->add (simple_markers(frame_rate));

	cpl->add (reel);

	d->add (cpl);
	d->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	return dcp::verify ({dcp_path}, &stage, &progress, xsd_test);
}


static
void
check_picture_size_ok (int width, int height, int frame_rate, bool three_d)
{
	auto notes = check_picture_size(width, height, frame_rate, three_d);
	dump_notes (notes);
	BOOST_CHECK_EQUAL (notes.size(), 0U);
}


static
void
check_picture_size_bad_frame_size (int width, int height, int frame_rate, bool three_d)
{
	auto notes = check_picture_size(width, height, frame_rate, three_d);
	BOOST_REQUIRE_EQUAL (notes.size(), 1U);
	BOOST_CHECK_EQUAL (notes.front().type(), dcp::VerificationNote::VERIFY_BV21_ERROR);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::PICTURE_ASSET_INVALID_SIZE_IN_PIXELS);
}


static
void
check_picture_size_bad_2k_frame_rate (int width, int height, int frame_rate, bool three_d)
{
	auto notes = check_picture_size(width, height, frame_rate, three_d);
	BOOST_REQUIRE_EQUAL (notes.size(), 2U);
	BOOST_CHECK_EQUAL (notes.back().type(), dcp::VerificationNote::VERIFY_BV21_ERROR);
	BOOST_CHECK_EQUAL (notes.back().code(), dcp::VerificationNote::PICTURE_ASSET_INVALID_FRAME_RATE_FOR_2K);
}


static
void
check_picture_size_bad_4k_frame_rate (int width, int height, int frame_rate, bool three_d)
{
	auto notes = check_picture_size(width, height, frame_rate, three_d);
	BOOST_REQUIRE_EQUAL (notes.size(), 1U);
	BOOST_CHECK_EQUAL (notes.front().type(), dcp::VerificationNote::VERIFY_BV21_ERROR);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::PICTURE_ASSET_INVALID_FRAME_RATE_FOR_4K);
}


BOOST_AUTO_TEST_CASE (verify_picture_size)
{
	using namespace boost::filesystem;

	/* 2K scope */
	check_picture_size_ok (2048, 858, 24, false);
	check_picture_size_ok (2048, 858, 25, false);
	check_picture_size_ok (2048, 858, 48, false);
	check_picture_size_ok (2048, 858, 24, true);
	check_picture_size_ok (2048, 858, 25, true);
	check_picture_size_ok (2048, 858, 48, true);

	/* 2K flat */
	check_picture_size_ok (1998, 1080, 24, false);
	check_picture_size_ok (1998, 1080, 25, false);
	check_picture_size_ok (1998, 1080, 48, false);
	check_picture_size_ok (1998, 1080, 24, true);
	check_picture_size_ok (1998, 1080, 25, true);
	check_picture_size_ok (1998, 1080, 48, true);

	/* 4K scope */
	check_picture_size_ok (4096, 1716, 24, false);

	/* 4K flat */
	check_picture_size_ok (3996, 2160, 24, false);

	/* Bad frame size */
	check_picture_size_bad_frame_size (2050, 858, 24, false);
	check_picture_size_bad_frame_size (2048, 658, 25, false);
	check_picture_size_bad_frame_size (1920, 1080, 48, true);
	check_picture_size_bad_frame_size (4000, 3000, 24, true);

	/* Bad 2K frame rate */
	check_picture_size_bad_2k_frame_rate (2048, 858, 26, false);
	check_picture_size_bad_2k_frame_rate (2048, 858, 31, false);
	check_picture_size_bad_2k_frame_rate (1998, 1080, 50, true);

	/* Bad 4K frame rate */
	check_picture_size_bad_4k_frame_rate (3996, 2160, 25, false);
	check_picture_size_bad_4k_frame_rate (3996, 2160, 48, false);

	/* No 4K 3D */
	auto notes = check_picture_size(3996, 2160, 24, true);
	BOOST_REQUIRE_EQUAL (notes.size(), 1U);
	BOOST_CHECK_EQUAL (notes.front().type(), dcp::VerificationNote::VERIFY_BV21_ERROR);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::PICTURE_ASSET_4K_3D);
}


static
void
add_test_subtitle (shared_ptr<dcp::SubtitleAsset> asset, int start_frame, int end_frame, float v_position = 0, string text = "Hello")
{
	asset->add (
		make_shared<dcp::SubtitleString>(
			optional<string>(),
			false,
			false,
			false,
			dcp::Colour(),
			42,
			1,
			dcp::Time(start_frame, 24, 24),
			dcp::Time(end_frame, 24, 24),
			0,
			dcp::HALIGN_CENTER,
			v_position,
			dcp::VALIGN_CENTER,
			dcp::DIRECTION_LTR,
			text,
			dcp::NONE,
			dcp::Colour(),
			dcp::Time(),
			dcp::Time()
		)
	);
}


BOOST_AUTO_TEST_CASE (verify_closed_caption_xml_too_large)
{
	boost::filesystem::path const dir("build/test/verify_closed_caption_xml_too_large");
	prepare_directory (dir);

	auto asset = make_shared<dcp::SMPTESubtitleAsset>();
	for (int i = 0; i < 2048; ++i) {
		add_test_subtitle (asset, i * 24, i * 24 + 20);
	}
	asset->set_language (dcp::LanguageTag("de-DE"));
	asset->write (dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelClosedCaptionAsset>(asset, dcp::Fraction(24, 1), 16 * 24, 0);
	write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_SUBTITLE_START_TIME },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::CLOSED_CAPTION_XML_TOO_LARGE_IN_BYTES },
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::FIRST_TEXT_TOO_EARLY },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA },
		});
}


static
shared_ptr<dcp::SMPTESubtitleAsset>
make_large_subtitle_asset (boost::filesystem::path font_file)
{
	auto asset = make_shared<dcp::SMPTESubtitleAsset>();
	dcp::ArrayData big_fake_font(1024 * 1024);
	big_fake_font.write (font_file);
	for (int i = 0; i < 116; ++i) {
		asset->add_font (dcp::String::compose("big%1", i), big_fake_font);
	}
	return asset;
}


template <class T>
void
verify_timed_text_asset_too_large (string name)
{
	auto const dir = boost::filesystem::path("build/test") / name;
	prepare_directory (dir);
	auto asset = make_large_subtitle_asset (dir / "font.ttf");
	add_test_subtitle (asset, 0, 20);
	asset->set_language (dcp::LanguageTag("de-DE"));
	asset->write (dir / "subs.mxf");

	auto reel_asset = make_shared<T>(asset, dcp::Fraction(24, 1), 16 * 24, 0);
	write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::TIMED_TEXT_ASSET_TOO_LARGE_IN_BYTES },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::TIMED_TEXT_FONTS_TOO_LARGE_IN_BYTES },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_SUBTITLE_START_TIME },
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::FIRST_TEXT_TOO_EARLY },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA },
		});
}


BOOST_AUTO_TEST_CASE (verify_subtitle_asset_too_large)
{
	verify_timed_text_asset_too_large<dcp::ReelSubtitleAsset>("verify_subtitle_asset_too_large");
	verify_timed_text_asset_too_large<dcp::ReelClosedCaptionAsset>("verify_closed_caption_asset_too_large");
}


BOOST_AUTO_TEST_CASE (verify_missing_language_tag_in_subtitle_xml)
{
	boost::filesystem::path dir = "build/test/verify_missing_language_tag_in_subtitle_xml";
	prepare_directory (dir);
	auto dcp = make_simple (dir, 1, 240);

	string const xml =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SubtitleReel xmlns=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\" xmlns:xs=\"http://www.w3.org/2001/schema\">"
		"<Id>urn:uuid:e6a8ae03-ebbf-41ed-9def-913a87d1493a</Id>"
		"<ContentTitleText>Content</ContentTitleText>"
		"<AnnotationText>Annotation</AnnotationText>"
		"<IssueDate>2018-10-02T12:25:14+02:00</IssueDate>"
		"<ReelNumber>1</ReelNumber>"
		"<EditRate>25 1</EditRate>"
		"<TimeCodeRate>25</TimeCodeRate>"
		"<StartTime>00:00:00:00</StartTime>"
		"<LoadFont ID=\"arial\">urn:uuid:e4f0ff0a-9eba-49e0-92ee-d89a88a575f6</LoadFont>"
		"<SubtitleList>"
		"<Font ID=\"arial\" Color=\"FFFEFEFE\" Weight=\"normal\" Size=\"42\" Effect=\"border\" EffectColor=\"FF181818\" AspectAdjust=\"1.00\">"
		"<Subtitle SpotNumber=\"1\" TimeIn=\"00:00:03:00\" TimeOut=\"00:00:04:10\" FadeUpTime=\"00:00:00:00\" FadeDownTime=\"00:00:00:00\">"
		"<Text Hposition=\"0.0\" Halign=\"center\" Valign=\"bottom\" Vposition=\"13.5\" Direction=\"ltr\">Hello world</Text>"
		"</Subtitle>"
		"</Font>"
		"</SubtitleList>"
		"</SubtitleReel>";

	auto xml_file = dcp::fopen_boost (dir / "subs.xml", "w");
	BOOST_REQUIRE (xml_file);
	fwrite (xml.c_str(), xml.size(), 1, xml_file);
	fclose (xml_file);
	auto subs = make_shared<dcp::SMPTESubtitleAsset>(dir / "subs.xml");
	subs->write (dir / "subs.mxf");

	auto reel_subs = make_shared<dcp::ReelSubtitleAsset>(subs, dcp::Fraction(24, 1), 240, 0);
	dcp->cpls().front()->reels().front()->add(reel_subs);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_SUBTITLE_LANGUAGE },
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::FIRST_TEXT_TOO_EARLY }
		});
}


BOOST_AUTO_TEST_CASE (verify_inconsistent_subtitle_languages)
{
	boost::filesystem::path path ("build/test/verify_inconsistent_subtitle_languages");
	auto dcp = make_simple (path, 2, 240);
	auto cpl = dcp->cpls()[0];

	{
		auto subs = make_shared<dcp::SMPTESubtitleAsset>();
		subs->set_language (dcp::LanguageTag("de-DE"));
		subs->add (simple_subtitle());
		subs->write (path / "subs1.mxf");
		auto reel_subs = make_shared<dcp::ReelSubtitleAsset>(subs, dcp::Fraction(24, 1), 240, 0);
		cpl->reels()[0]->add(reel_subs);
	}

	{
		auto subs = make_shared<dcp::SMPTESubtitleAsset>();
		subs->set_language (dcp::LanguageTag("en-US"));
		subs->add (simple_subtitle());
		subs->write (path / "subs2.mxf");
		auto reel_subs = make_shared<dcp::ReelSubtitleAsset>(subs, dcp::Fraction(24, 1), 240, 0);
		cpl->reels()[1]->add(reel_subs);
	}

	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_verify_result (
		{ path },
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_SUBTITLE_START_TIME },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::SUBTITLE_LANGUAGES_DIFFER },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_SUBTITLE_START_TIME }
		});
}


BOOST_AUTO_TEST_CASE (verify_missing_start_time_tag_in_subtitle_xml)
{
	boost::filesystem::path dir = "build/test/verify_missing_start_time_tag_in_subtitle_xml";
	prepare_directory (dir);
	auto dcp = make_simple (dir, 1, 240);

	string const xml =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SubtitleReel xmlns=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\" xmlns:xs=\"http://www.w3.org/2001/schema\">"
		"<Id>urn:uuid:e6a8ae03-ebbf-41ed-9def-913a87d1493a</Id>"
		"<ContentTitleText>Content</ContentTitleText>"
		"<AnnotationText>Annotation</AnnotationText>"
		"<IssueDate>2018-10-02T12:25:14+02:00</IssueDate>"
		"<ReelNumber>1</ReelNumber>"
		"<Language>de-DE</Language>"
		"<EditRate>25 1</EditRate>"
		"<TimeCodeRate>25</TimeCodeRate>"
		"<LoadFont ID=\"arial\">urn:uuid:e4f0ff0a-9eba-49e0-92ee-d89a88a575f6</LoadFont>"
		"<SubtitleList>"
		"<Font ID=\"arial\" Color=\"FFFEFEFE\" Weight=\"normal\" Size=\"42\" Effect=\"border\" EffectColor=\"FF181818\" AspectAdjust=\"1.00\">"
		"<Subtitle SpotNumber=\"1\" TimeIn=\"00:00:03:00\" TimeOut=\"00:00:04:10\" FadeUpTime=\"00:00:00:00\" FadeDownTime=\"00:00:00:00\">"
		"<Text Hposition=\"0.0\" Halign=\"center\" Valign=\"bottom\" Vposition=\"13.5\" Direction=\"ltr\">Hello world</Text>"
		"</Subtitle>"
		"</Font>"
		"</SubtitleList>"
		"</SubtitleReel>";

	auto xml_file = dcp::fopen_boost (dir / "subs.xml", "w");
	BOOST_REQUIRE (xml_file);
	fwrite (xml.c_str(), xml.size(), 1, xml_file);
	fclose (xml_file);
	auto subs = make_shared<dcp::SMPTESubtitleAsset>(dir / "subs.xml");
	subs->write (dir / "subs.mxf");

	auto reel_subs = make_shared<dcp::ReelSubtitleAsset>(subs, dcp::Fraction(24, 1), 240, 0);
	dcp->cpls().front()->reels().front()->add(reel_subs);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_SUBTITLE_START_TIME },
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::FIRST_TEXT_TOO_EARLY }
		});
}


BOOST_AUTO_TEST_CASE (verify_non_zero_start_time_tag_in_subtitle_xml)
{
	boost::filesystem::path dir = "build/test/verify_non_zero_start_time_tag_in_subtitle_xml";
	prepare_directory (dir);
	auto dcp = make_simple (dir, 1, 240);

	string const xml =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SubtitleReel xmlns=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\" xmlns:xs=\"http://www.w3.org/2001/schema\">"
		"<Id>urn:uuid:e6a8ae03-ebbf-41ed-9def-913a87d1493a</Id>"
		"<ContentTitleText>Content</ContentTitleText>"
		"<AnnotationText>Annotation</AnnotationText>"
		"<IssueDate>2018-10-02T12:25:14+02:00</IssueDate>"
		"<ReelNumber>1</ReelNumber>"
		"<Language>de-DE</Language>"
		"<EditRate>25 1</EditRate>"
		"<TimeCodeRate>25</TimeCodeRate>"
		"<StartTime>00:00:02:00</StartTime>"
		"<LoadFont ID=\"arial\">urn:uuid:e4f0ff0a-9eba-49e0-92ee-d89a88a575f6</LoadFont>"
		"<SubtitleList>"
		"<Font ID=\"arial\" Color=\"FFFEFEFE\" Weight=\"normal\" Size=\"42\" Effect=\"border\" EffectColor=\"FF181818\" AspectAdjust=\"1.00\">"
		"<Subtitle SpotNumber=\"1\" TimeIn=\"00:00:03:00\" TimeOut=\"00:00:04:10\" FadeUpTime=\"00:00:00:00\" FadeDownTime=\"00:00:00:00\">"
		"<Text Hposition=\"0.0\" Halign=\"center\" Valign=\"bottom\" Vposition=\"13.5\" Direction=\"ltr\">Hello world</Text>"
		"</Subtitle>"
		"</Font>"
		"</SubtitleList>"
		"</SubtitleReel>";

	auto xml_file = dcp::fopen_boost (dir / "subs.xml", "w");
	BOOST_REQUIRE (xml_file);
	fwrite (xml.c_str(), xml.size(), 1, xml_file);
	fclose (xml_file);
	auto subs = make_shared<dcp::SMPTESubtitleAsset>(dir / "subs.xml");
	subs->write (dir / "subs.mxf");

	auto reel_subs = make_shared<dcp::ReelSubtitleAsset>(subs, dcp::Fraction(24, 1), 240, 0);
	dcp->cpls().front()->reels().front()->add(reel_subs);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::SUBTITLE_START_TIME_NON_ZERO },
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::FIRST_TEXT_TOO_EARLY }
		});
}


class TestText
{
public:
	TestText (int in_, int out_, float v_position_ = 0, string text_ = "Hello")
		: in(in_)
		, out(out_)
		, v_position(v_position_)
		, text(text_)
	{}

	int in;
	int out;
	float v_position;
	string text;
};


template <class T>
void
dcp_with_text (boost::filesystem::path dir, vector<TestText> subs)
{
	prepare_directory (dir);
	auto asset = make_shared<dcp::SMPTESubtitleAsset>();
	asset->set_start_time (dcp::Time());
	for (auto i: subs) {
		add_test_subtitle (asset, i.in, i.out, i.v_position, i.text);
	}
	asset->set_language (dcp::LanguageTag("de-DE"));
	asset->write (dir / "subs.mxf");

	auto reel_asset = make_shared<T>(asset, dcp::Fraction(24, 1), 16 * 24, 0);
	write_dcp_with_single_asset (dir, reel_asset);
}


BOOST_AUTO_TEST_CASE (verify_text_too_early)
{
	auto const dir = boost::filesystem::path("build/test/verify_text_too_early");
	/* Just too early */
	dcp_with_text<dcp::ReelSubtitleAsset> (dir, {{ 4 * 24 - 1, 5 * 24 }});
	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::FIRST_TEXT_TOO_EARLY },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});

}


BOOST_AUTO_TEST_CASE (verify_text_not_too_early)
{
	auto const dir = boost::filesystem::path("build/test/verify_text_not_too_early");
	/* Just late enough */
	dcp_with_text<dcp::ReelSubtitleAsset> (dir, {{ 4 * 24, 5 * 24 }});
	check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }});
}


BOOST_AUTO_TEST_CASE (verify_text_early_on_second_reel)
{
	auto const dir = boost::filesystem::path("build/test/verify_text_early_on_second_reel");
	prepare_directory (dir);

	auto asset1 = make_shared<dcp::SMPTESubtitleAsset>();
	asset1->set_start_time (dcp::Time());
	/* Just late enough */
	add_test_subtitle (asset1, 4 * 24, 5 * 24);
	asset1->set_language (dcp::LanguageTag("de-DE"));
	asset1->write (dir / "subs1.mxf");
	auto reel_asset1 = make_shared<dcp::ReelSubtitleAsset>(asset1, dcp::Fraction(24, 1), 16 * 24, 0);
	auto reel1 = make_shared<dcp::Reel>();
	reel1->add (reel_asset1);
	auto markers1 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), 16 * 24, 0);
	markers1->set (dcp::Marker::FFOC, dcp::Time(1, 24, 24));
	reel1->add (markers1);

	auto asset2 = make_shared<dcp::SMPTESubtitleAsset>();
	asset2->set_start_time (dcp::Time());
	/* This would be too early on first reel but should be OK on the second */
	add_test_subtitle (asset2, 0, 4 * 24);
	asset2->set_language (dcp::LanguageTag("de-DE"));
	asset2->write (dir / "subs2.mxf");
	auto reel_asset2 = make_shared<dcp::ReelSubtitleAsset>(asset2, dcp::Fraction(24, 1), 16 * 24, 0);
	auto reel2 = make_shared<dcp::Reel>();
	reel2->add (reel_asset2);
	auto markers2 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), 16 * 24, 0);
	markers2->set (dcp::Marker::LFOC, dcp::Time(16 * 24 - 1, 24, 24));
	reel2->add (markers2);

	auto cpl = make_shared<dcp::CPL>("hello", dcp::TRAILER);
	cpl->add (reel1);
	cpl->add (reel2);
	auto dcp = make_shared<dcp::DCP>(dir);
	dcp->add (cpl);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"hello"
		);


	check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }});
}


BOOST_AUTO_TEST_CASE (verify_text_too_close)
{
	auto const dir = boost::filesystem::path("build/test/verify_text_too_close");
	dcp_with_text<dcp::ReelSubtitleAsset> (
		dir,
		{
			{ 4 * 24,     5 * 24 },
			{ 5 * 24 + 1, 6 * 24 },
		});
	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::SUBTITLE_TOO_CLOSE },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}


BOOST_AUTO_TEST_CASE (verify_text_not_too_close)
{
	auto const dir = boost::filesystem::path("build/test/verify_text_not_too_close");
	dcp_with_text<dcp::ReelSubtitleAsset> (
		dir,
		{
			{ 4 * 24,      5 * 24 },
			{ 5 * 24 + 16, 8 * 24 },
		});
	check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }});
}


BOOST_AUTO_TEST_CASE (verify_text_too_short)
{
	auto const dir = boost::filesystem::path("build/test/verify_text_too_short");
	dcp_with_text<dcp::ReelSubtitleAsset> (dir, {{ 4 * 24, 4 * 24 + 1 }});
	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::SUBTITLE_TOO_SHORT },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}


BOOST_AUTO_TEST_CASE (verify_text_not_too_short)
{
	auto const dir = boost::filesystem::path("build/test/verify_text_not_too_short");
	dcp_with_text<dcp::ReelSubtitleAsset> (dir, {{ 4 * 24, 4 * 24 + 17 }});
	check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }});
}


BOOST_AUTO_TEST_CASE (verify_too_many_subtitle_lines1)
{
	auto const dir = boost::filesystem::path ("build/test/verify_too_many_subtitle_lines1");
	dcp_with_text<dcp::ReelSubtitleAsset> (
		dir,
		{
			{ 96, 200, 0.0, "We" },
			{ 96, 200, 0.1, "have" },
			{ 96, 200, 0.2, "four" },
			{ 96, 200, 0.3, "lines" }
		});
	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::TOO_MANY_SUBTITLE_LINES },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}


BOOST_AUTO_TEST_CASE (verify_not_too_many_subtitle_lines1)
{
	auto const dir = boost::filesystem::path ("build/test/verify_not_too_many_subtitle_lines1");
	dcp_with_text<dcp::ReelSubtitleAsset> (
		dir,
		{
			{ 96, 200, 0.0, "We" },
			{ 96, 200, 0.1, "have" },
			{ 96, 200, 0.2, "four" },
		});
	check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }});
}


BOOST_AUTO_TEST_CASE (verify_too_many_subtitle_lines2)
{
	auto const dir = boost::filesystem::path ("build/test/verify_too_many_subtitle_lines2");
	dcp_with_text<dcp::ReelSubtitleAsset> (
		dir,
		{
			{ 96, 300, 0.0, "We" },
			{ 96, 300, 0.1, "have" },
			{ 150, 180, 0.2, "four" },
			{ 150, 180, 0.3, "lines" }
		});
	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::TOO_MANY_SUBTITLE_LINES },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}


BOOST_AUTO_TEST_CASE (verify_not_too_many_subtitle_lines2)
{
	auto const dir = boost::filesystem::path ("build/test/verify_not_too_many_subtitle_lines2");
	dcp_with_text<dcp::ReelSubtitleAsset> (
		dir,
		{
			{ 96, 300, 0.0, "We" },
			{ 96, 300, 0.1, "have" },
			{ 150, 180, 0.2, "four" },
			{ 190, 250, 0.3, "lines" }
		});
	check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }});
}


BOOST_AUTO_TEST_CASE (verify_subtitle_lines_too_long1)
{
	auto const dir = boost::filesystem::path ("build/test/verify_subtitle_lines_too_long1");
	dcp_with_text<dcp::ReelSubtitleAsset> (
		dir,
		{
			{ 96, 300, 0.0, "012345678901234567890123456789012345678901234567890123" }
		});
	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::SUBTITLE_LINE_LONGER_THAN_RECOMMENDED },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}


BOOST_AUTO_TEST_CASE (verify_subtitle_lines_too_long2)
{
	auto const dir = boost::filesystem::path ("build/test/verify_subtitle_lines_too_long2");
	dcp_with_text<dcp::ReelSubtitleAsset> (
		dir,
		{
			{ 96, 300, 0.0, "012345678901234567890123456789012345678901234567890123456789012345678901234567890" }
		});
	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::SUBTITLE_LINE_TOO_LONG },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}


BOOST_AUTO_TEST_CASE (verify_too_many_closed_caption_lines1)
{
	auto const dir = boost::filesystem::path ("build/test/verify_too_many_closed_caption_lines1");
	dcp_with_text<dcp::ReelClosedCaptionAsset> (
		dir,
		{
			{ 96, 200, 0.0, "We" },
			{ 96, 200, 0.1, "have" },
			{ 96, 200, 0.2, "four" },
			{ 96, 200, 0.3, "lines" }
		});
	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::TOO_MANY_CLOSED_CAPTION_LINES},
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}


BOOST_AUTO_TEST_CASE (verify_not_too_many_closed_caption_lines1)
{
	auto const dir = boost::filesystem::path ("build/test/verify_not_too_many_closed_caption_lines1");
	dcp_with_text<dcp::ReelClosedCaptionAsset> (
		dir,
		{
			{ 96, 200, 0.0, "We" },
			{ 96, 200, 0.1, "have" },
			{ 96, 200, 0.2, "four" },
		});
	check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }});
}


BOOST_AUTO_TEST_CASE (verify_too_many_closed_caption_lines2)
{
	auto const dir = boost::filesystem::path ("build/test/verify_too_many_closed_caption_lines2");
	dcp_with_text<dcp::ReelClosedCaptionAsset> (
		dir,
		{
			{ 96, 300, 0.0, "We" },
			{ 96, 300, 0.1, "have" },
			{ 150, 180, 0.2, "four" },
			{ 150, 180, 0.3, "lines" }
		});
	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::TOO_MANY_CLOSED_CAPTION_LINES},
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}


BOOST_AUTO_TEST_CASE (verify_not_too_many_closed_caption_lines2)
{
	auto const dir = boost::filesystem::path ("build/test/verify_not_too_many_closed_caption_lines2");
	dcp_with_text<dcp::ReelClosedCaptionAsset> (
		dir,
		{
			{ 96, 300, 0.0, "We" },
			{ 96, 300, 0.1, "have" },
			{ 150, 180, 0.2, "four" },
			{ 190, 250, 0.3, "lines" }
		});
	check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }});
}


BOOST_AUTO_TEST_CASE (verify_closed_caption_lines_too_long1)
{
	auto const dir = boost::filesystem::path ("build/test/verify_closed_caption_lines_too_long1");
	dcp_with_text<dcp::ReelClosedCaptionAsset> (
		dir,
		{
			{ 96, 300, 0.0, "0123456789012345678901234567890123" }
		});
	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::CLOSED_CAPTION_LINE_TOO_LONG },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}


BOOST_AUTO_TEST_CASE (verify_sound_sampling_rate_must_be_48k)
{
	boost::filesystem::path const dir("build/test/verify_sound_sampling_rate_must_be_48k");
	prepare_directory (dir);

	auto picture = simple_picture (dir, "foo");
	auto reel_picture = make_shared<dcp::ReelMonoPictureAsset>(picture, 0);
	auto reel = make_shared<dcp::Reel>();
	reel->add (reel_picture);
	auto sound = simple_sound (dir, "foo", dcp::MXFMetadata(), "de-DE", 24, 96000);
	auto reel_sound = make_shared<dcp::ReelSoundAsset>(sound, 0);
	reel->add (reel_sound);
	reel->add (simple_markers());
	auto cpl = make_shared<dcp::CPL>("hello", dcp::TRAILER);
	cpl->add (reel);
	auto dcp = make_shared<dcp::DCP>(dir);
	dcp->add (cpl);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"hello"
		);

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::INVALID_SOUND_FRAME_RATE },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}


BOOST_AUTO_TEST_CASE (verify_cpl_must_have_annotation_text)
{
	boost::filesystem::path const dir("build/test/verify_cpl_must_have_annotation_text");
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	BOOST_REQUIRE_EQUAL (dcp->cpls().size(), 1U);

	{
		BOOST_REQUIRE (dcp->cpls()[0]->file());
		Editor e(dcp->cpls()[0]->file().get());
		e.replace("<AnnotationText>A Test DCP</AnnotationText>", "");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_ANNOTATION_TEXT_IN_CPL },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::CPL_HASH_INCORRECT }
		});
}


BOOST_AUTO_TEST_CASE (verify_cpl_annotation_text_should_be_same_as_content_title_text)
{
	boost::filesystem::path const dir("build/test/verify_cpl_annotation_text_should_be_same_as_content_title_text");
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	BOOST_REQUIRE_EQUAL (dcp->cpls().size(), 1U);

	{
		BOOST_REQUIRE (dcp->cpls()[0]->file());
		Editor e(dcp->cpls()[0]->file().get());
		e.replace("<AnnotationText>A Test DCP</AnnotationText>", "<AnnotationText>A Test DCP 1</AnnotationText>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::CPL_ANNOTATION_TEXT_DIFFERS_FROM_CONTENT_TITLE_TEXT },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::CPL_HASH_INCORRECT }
		});
}


BOOST_AUTO_TEST_CASE (verify_reel_assets_durations_must_match)
{
	boost::filesystem::path const dir("build/test/verify_reel_assets_durations_must_match");
	boost::filesystem::remove_all (dir);
	boost::filesystem::create_directories (dir);
	shared_ptr<dcp::DCP> dcp (new dcp::DCP(dir));
	shared_ptr<dcp::CPL> cpl (new dcp::CPL("A Test DCP", dcp::TRAILER));

	shared_ptr<dcp::MonoPictureAsset> mp = simple_picture (dir, "", 24);
	shared_ptr<dcp::SoundAsset> ms = simple_sound (dir, "", dcp::MXFMetadata(), "en-US", 25);

	auto reel = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(mp, 0),
		make_shared<dcp::ReelSoundAsset>(ms, 0)
		);

	reel->add (simple_markers());
	cpl->add (reel);

	dcp->add (cpl);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);


	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISMATCHED_ASSET_DURATION },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}



static
void
verify_subtitles_must_be_in_all_reels_check (boost::filesystem::path dir, bool add_to_reel1, bool add_to_reel2)
{
	boost::filesystem::remove_all (dir);
	boost::filesystem::create_directories (dir);
	auto dcp = make_shared<dcp::DCP>(dir);
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::TRAILER);

	auto subs = make_shared<dcp::SMPTESubtitleAsset>();
	subs->set_language (dcp::LanguageTag("de-DE"));
	subs->set_start_time (dcp::Time());
	subs->add (simple_subtitle());
	subs->write (dir / "subs.mxf");
	auto reel_subs = make_shared<dcp::ReelSubtitleAsset>(subs, dcp::Fraction(24, 1), 240, 0);

	auto reel1 = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "", 240), 0),
		make_shared<dcp::ReelSoundAsset>(simple_sound(dir, "", dcp::MXFMetadata(), "en-US", 240), 0)
		);

	if (add_to_reel1) {
		reel1->add (make_shared<dcp::ReelSubtitleAsset>(subs, dcp::Fraction(24, 1), 240, 0));
	}

	auto markers1 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), 240, 0);
	markers1->set (dcp::Marker::FFOC, dcp::Time(1, 24, 24));
	reel1->add (markers1);

	cpl->add (reel1);

	auto reel2 = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "", 240), 0),
		make_shared<dcp::ReelSoundAsset>(simple_sound(dir, "", dcp::MXFMetadata(), "en-US", 240), 0)
		);

	if (add_to_reel2) {
		reel2->add (make_shared<dcp::ReelSubtitleAsset>(subs, dcp::Fraction(24, 1), 240, 0));
	}

	auto markers2 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), 240, 0);
	markers2->set (dcp::Marker::LFOC, dcp::Time(239, 24, 24));
	reel2->add (markers2);

	cpl->add (reel2);

	dcp->add (cpl);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);
}


BOOST_AUTO_TEST_CASE (verify_subtitles_must_be_in_all_reels)
{
	{
		boost::filesystem::path dir ("build/test/verify_subtitles_must_be_in_all_reels1");
		verify_subtitles_must_be_in_all_reels_check (dir, true, false);
		check_verify_result (
			{ dir },
			{
				{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MAIN_SUBTITLE_NOT_IN_ALL_REELS },
				{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
			});

	}

	{
		boost::filesystem::path dir ("build/test/verify_subtitles_must_be_in_all_reels2");
		verify_subtitles_must_be_in_all_reels_check (dir, true, true);
		check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }});
	}

	{
		boost::filesystem::path dir ("build/test/verify_subtitles_must_be_in_all_reels1");
		verify_subtitles_must_be_in_all_reels_check (dir, false, false);
		check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }});
	}
}


static
void
verify_closed_captions_must_be_in_all_reels_check (boost::filesystem::path dir, int caps_in_reel1, int caps_in_reel2)
{
	boost::filesystem::remove_all (dir);
	boost::filesystem::create_directories (dir);
	auto dcp = make_shared<dcp::DCP>(dir);
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::TRAILER);

	auto subs = make_shared<dcp::SMPTESubtitleAsset>();
	subs->set_language (dcp::LanguageTag("de-DE"));
	subs->set_start_time (dcp::Time());
	subs->add (simple_subtitle());
	subs->write (dir / "subs.mxf");

	auto reel1 = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "", 240), 0),
		make_shared<dcp::ReelSoundAsset>(simple_sound(dir, "", dcp::MXFMetadata(), "en-US", 240), 0)
		);

	for (int i = 0; i < caps_in_reel1; ++i) {
		reel1->add (make_shared<dcp::ReelClosedCaptionAsset>(subs, dcp::Fraction(24, 1), 240, 0));
	}

	auto markers1 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), 240, 0);
	markers1->set (dcp::Marker::FFOC, dcp::Time(1, 24, 24));
	reel1->add (markers1);

	cpl->add (reel1);

	auto reel2 = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "", 240), 0),
		make_shared<dcp::ReelSoundAsset>(simple_sound(dir, "", dcp::MXFMetadata(), "en-US", 240), 0)
		);

	for (int i = 0; i < caps_in_reel2; ++i) {
		reel2->add (make_shared<dcp::ReelClosedCaptionAsset>(subs, dcp::Fraction(24, 1), 240, 0));
	}

	auto markers2 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), 240, 0);
	markers2->set (dcp::Marker::LFOC, dcp::Time(239, 24, 24));
	reel2->add (markers2);

	cpl->add (reel2);

	dcp->add (cpl);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);
}


BOOST_AUTO_TEST_CASE (verify_closed_captions_must_be_in_all_reels)
{
	{
		boost::filesystem::path dir ("build/test/verify_closed_captions_must_be_in_all_reels1");
		verify_closed_captions_must_be_in_all_reels_check (dir, 3, 4);
		check_verify_result (
			{dir},
			{
				{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::CLOSED_CAPTION_ASSET_COUNTS_DIFFER },
				{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
			});
	}

	{
		boost::filesystem::path dir ("build/test/verify_closed_captions_must_be_in_all_reels2");
		verify_closed_captions_must_be_in_all_reels_check (dir, 4, 4);
		check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }});
	}

	{
		boost::filesystem::path dir ("build/test/verify_closed_captions_must_be_in_all_reels3");
		verify_closed_captions_must_be_in_all_reels_check (dir, 0, 0);
		check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }});
	}
}


template <class T>
void
verify_text_entry_point_check (boost::filesystem::path dir, dcp::VerificationNote::Code code, boost::function<void (shared_ptr<T>)> adjust)
{
	boost::filesystem::remove_all (dir);
	boost::filesystem::create_directories (dir);
	auto dcp = make_shared<dcp::DCP>(dir);
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::TRAILER);

	auto subs = make_shared<dcp::SMPTESubtitleAsset>();
	subs->set_language (dcp::LanguageTag("de-DE"));
	subs->set_start_time (dcp::Time());
	subs->add (simple_subtitle());
	subs->write (dir / "subs.mxf");
	auto reel_text = make_shared<T>(subs, dcp::Fraction(24, 1), 240, 0);
	adjust (reel_text);

	auto reel = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "", 240), 0),
		make_shared<dcp::ReelSoundAsset>(simple_sound(dir, "", dcp::MXFMetadata(), "en-US", 240), 0)
		);

	reel->add (reel_text);

	reel->add (simple_markers(240));

	cpl->add (reel);

	dcp->add (cpl);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, code },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA }
		});
}


BOOST_AUTO_TEST_CASE (verify_text_entry_point)
{
	verify_text_entry_point_check<dcp::ReelSubtitleAsset> (
		"build/test/verify_subtitle_entry_point_must_be_present",
		dcp::VerificationNote::MISSING_SUBTITLE_ENTRY_POINT,
		[](shared_ptr<dcp::ReelSubtitleAsset> asset) {
			asset->unset_entry_point ();
			}
		);

	verify_text_entry_point_check<dcp::ReelSubtitleAsset> (
		"build/test/verify_subtitle_entry_point_must_be_zero",
		dcp::VerificationNote::SUBTITLE_ENTRY_POINT_NON_ZERO,
		[](shared_ptr<dcp::ReelSubtitleAsset> asset) {
			asset->set_entry_point (4);
			}
		);

	verify_text_entry_point_check<dcp::ReelClosedCaptionAsset> (
		"build/test/verify_closed_caption_entry_point_must_be_present",
		dcp::VerificationNote::MISSING_CLOSED_CAPTION_ENTRY_POINT,
		[](shared_ptr<dcp::ReelClosedCaptionAsset> asset) {
			asset->unset_entry_point ();
			}
		);

	verify_text_entry_point_check<dcp::ReelClosedCaptionAsset> (
		"build/test/verify_closed_caption_entry_point_must_be_zero",
		dcp::VerificationNote::CLOSED_CAPTION_ENTRY_POINT_NON_ZERO,
		[](shared_ptr<dcp::ReelClosedCaptionAsset> asset) {
			asset->set_entry_point (9);
			}
		);
}


BOOST_AUTO_TEST_CASE (verify_assets_must_have_hashes)
{
	RNGFixer fix;

	boost::filesystem::path const dir("build/test/verify_assets_must_have_hashes");
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	BOOST_REQUIRE_EQUAL (dcp->cpls().size(), 1U);

	{
		BOOST_REQUIRE (dcp->cpls()[0]->file());
		Editor e(dcp->cpls()[0]->file().get());
		e.replace("<Hash>XGhFVrqZqapOJx5Fh2SLjj48Yjg=</Hash>", "");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::CPL_HASH_INCORRECT },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_HASH }
		});
}


static
void
verify_markers_test (
	boost::filesystem::path dir,
	vector<pair<dcp::Marker, dcp::Time>> markers,
	vector<dcp::VerificationNote> test_notes
	)
{
	auto dcp = make_simple (dir);
	dcp->cpls()[0]->set_content_kind (dcp::FEATURE);
	auto markers_asset = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), 24, 0);
	for (auto const& i: markers) {
		markers_asset->set (i.first, i.second);
	}
	dcp->cpls()[0]->reels()[0]->add(markers_asset);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_verify_result ({dir}, test_notes);
}


BOOST_AUTO_TEST_CASE (verify_markers)
{
	verify_markers_test (
		"build/test/verify_markers_all_correct",
		{
			{ dcp::Marker::FFEC, dcp::Time(12, 24, 24) },
			{ dcp::Marker::FFMC, dcp::Time(13, 24, 24) },
			{ dcp::Marker::FFOC, dcp::Time(1, 24, 24) },
			{ dcp::Marker::LFOC, dcp::Time(23, 24, 24) }
		},
		{}
		);

	verify_markers_test (
		"build/test/verify_markers_missing_ffec",
		{
			{ dcp::Marker::FFMC, dcp::Time(13, 24, 24) },
			{ dcp::Marker::FFOC, dcp::Time(1, 24, 24) },
			{ dcp::Marker::LFOC, dcp::Time(23, 24, 24) }
		},
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_FFEC_IN_FEATURE }
		});

	verify_markers_test (
		"build/test/verify_markers_missing_ffmc",
		{
			{ dcp::Marker::FFEC, dcp::Time(12, 24, 24) },
			{ dcp::Marker::FFOC, dcp::Time(1, 24, 24) },
			{ dcp::Marker::LFOC, dcp::Time(23, 24, 24) }
		},
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_FFMC_IN_FEATURE }
		});

	verify_markers_test (
		"build/test/verify_markers_missing_ffoc",
		{
			{ dcp::Marker::FFEC, dcp::Time(12, 24, 24) },
			{ dcp::Marker::FFMC, dcp::Time(13, 24, 24) },
			{ dcp::Marker::LFOC, dcp::Time(23, 24, 24) }
		},
		{
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::MISSING_FFOC}
		});

	verify_markers_test (
		"build/test/verify_markers_missing_lfoc",
		{
			{ dcp::Marker::FFEC, dcp::Time(12, 24, 24) },
			{ dcp::Marker::FFMC, dcp::Time(13, 24, 24) },
			{ dcp::Marker::FFOC, dcp::Time(1, 24, 24) }
		},
		{
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::MISSING_LFOC }
		});

	verify_markers_test (
		"build/test/verify_markers_incorrect_ffoc",
		{
			{ dcp::Marker::FFEC, dcp::Time(12, 24, 24) },
			{ dcp::Marker::FFMC, dcp::Time(13, 24, 24) },
			{ dcp::Marker::FFOC, dcp::Time(3, 24, 24) },
			{ dcp::Marker::LFOC, dcp::Time(23, 24, 24) }
		},
		{
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::INCORRECT_FFOC }
		});

	verify_markers_test (
		"build/test/verify_markers_incorrect_lfoc",
		{
			{ dcp::Marker::FFEC, dcp::Time(12, 24, 24) },
			{ dcp::Marker::FFMC, dcp::Time(13, 24, 24) },
			{ dcp::Marker::FFOC, dcp::Time(1, 24, 24) },
			{ dcp::Marker::LFOC, dcp::Time(18, 24, 24) }
		},
		{
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::INCORRECT_LFOC }
		});
}


BOOST_AUTO_TEST_CASE (verify_cpl_metadata_version)
{
	boost::filesystem::path dir = "build/test/verify_cpl_metadata_version";
	prepare_directory (dir);
	auto dcp = make_simple (dir);
	dcp->cpls()[0]->unset_version_number();
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_CPL_METADATA_VERSION_NUMBER }});
}


BOOST_AUTO_TEST_CASE (verify_cpl_extension_metadata1)
{
	boost::filesystem::path dir = "build/test/verify_cpl_extension_metadata1";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	{
		Editor e (dcp->cpls()[0]->file().get());
		e.delete_lines ("<meta:ExtensionMetadataList>", "</meta:ExtensionMetadataList>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::CPL_HASH_INCORRECT },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_EXTENSION_METADATA }
		});
}


BOOST_AUTO_TEST_CASE (verify_cpl_extension_metadata2)
{
	boost::filesystem::path dir = "build/test/verify_cpl_extension_metadata2";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	{
		Editor e (dcp->cpls()[0]->file().get());
		e.delete_lines ("<meta:ExtensionMetadata scope=\"http://isdcf.com/ns/cplmd/app\">", "</meta:ExtensionMetadata>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::CPL_HASH_INCORRECT },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_EXTENSION_METADATA }
		});
}


BOOST_AUTO_TEST_CASE (verify_cpl_extension_metadata3)
{
	boost::filesystem::path dir = "build/test/verify_cpl_extension_metadata3";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	{
		Editor e (dcp->cpls()[0]->file().get());
		e.replace ("<meta:Name>A", "<meta:NameX>A");
		e.replace ("n</meta:Name>", "n</meta:NameX>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::CPL_HASH_INCORRECT },
		});
}


BOOST_AUTO_TEST_CASE (verify_cpl_extension_metadata4)
{
	boost::filesystem::path dir = "build/test/verify_cpl_extension_metadata4";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	{
		Editor e (dcp->cpls()[0]->file().get());
		e.replace ("Application", "Fred");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::CPL_HASH_INCORRECT },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::INVALID_EXTENSION_METADATA, string("<Name> property should be 'Application'") },
		});
}


BOOST_AUTO_TEST_CASE (verify_cpl_extension_metadata5)
{
	boost::filesystem::path dir = "build/test/verify_cpl_extension_metadata5";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);
	{
		Editor e (dcp->cpls()[0]->file().get());
		e.replace ("DCP Constraints Profile", "Fred");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::CPL_HASH_INCORRECT },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::INVALID_EXTENSION_METADATA, string("<Name> property should be 'DCP Constraints Profile'") },
		});
}


BOOST_AUTO_TEST_CASE (verify_cpl_extension_metadata6)
{
	boost::filesystem::path dir = "build/test/verify_cpl_extension_metadata6";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	{
		Editor e (dcp->cpls()[0]->file().get());
		e.replace ("<meta:Value>", "<meta:ValueX>");
		e.replace ("</meta:Value>", "</meta:ValueX>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::CPL_HASH_INCORRECT },
		});
}


BOOST_AUTO_TEST_CASE (verify_cpl_extension_metadata7)
{
	boost::filesystem::path dir = "build/test/verify_cpl_extension_metadata7";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);
	{
		Editor e (dcp->cpls()[0]->file().get());
		e.replace ("SMPTE-RDD-52:2020-Bv2.1", "Fred");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::CPL_HASH_INCORRECT },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::INVALID_EXTENSION_METADATA, string("<Value> property should be 'SMPTE-RDD-52:2020-Bv2.1'") },
		});
}


BOOST_AUTO_TEST_CASE (verify_cpl_extension_metadata8)
{
	boost::filesystem::path dir = "build/test/verify_cpl_extension_metadata8";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);
	{
		Editor e (dcp->cpls()[0]->file().get());
		e.replace ("<meta:Property>", "<meta:PropertyX>");
		e.replace ("</meta:Property>", "</meta:PropertyX>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::CPL_HASH_INCORRECT },
		});
}


BOOST_AUTO_TEST_CASE (verify_cpl_extension_metadata9)
{
	boost::filesystem::path dir = "build/test/verify_cpl_extension_metadata9";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::SMPTE,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);
	{
		Editor e (dcp->cpls()[0]->file().get());
		e.replace ("<meta:PropertyList>", "<meta:PropertyListX>");
		e.replace ("</meta:PropertyList>", "</meta:PropertyListX>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::XML_VALIDATION_ERROR },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::CPL_HASH_INCORRECT },
		});
}



BOOST_AUTO_TEST_CASE (verify_encrypted_cpl_is_signed)
{
	boost::filesystem::path dir = "build/test/verify_encrypted_cpl_is_signed";
	prepare_directory (dir);
	for (auto i: boost::filesystem::directory_iterator("test/ref/DCP/encryption_test")) {
		boost::filesystem::copy_file (i.path(), dir / i.path().filename());
	}

	{
		Editor e (dir / "cpl_81fb54df-e1bf-4647-8788-ea7ba154375b.xml");
		e.delete_lines ("<dsig:Signature", "</dsig:Signature>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::CPL_HASH_INCORRECT },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::PKL_ANNOTATION_TEXT_DOES_NOT_MATCH_CPL_CONTENT_TITLE_TEXT },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::MISSING_FFEC_IN_FEATURE },
			{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::MISSING_FFMC_IN_FEATURE },
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::MISSING_FFOC },
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::MISSING_LFOC },
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::MISSING_CPL_METADATA },
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::CPL_WITH_ENCRYPTED_CONTENT_NOT_SIGNED }
		});
}

