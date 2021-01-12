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
	auto cpl = make_shared<dcp::CPL>("hello", dcp::FEATURE);
	cpl->add (reel);
	auto dcp = make_shared<dcp::DCP>(dir);
	dcp->add (cpl);
	dcp->write_xml (standard);
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
		boost::algorithm::replace_all (_content, a, b);
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
check_verify_result (vector<boost::filesystem::path> dir, vector<std::pair<dcp::VerificationNote::Type, dcp::VerificationNote::Code>> types_and_codes)
{
	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);
	dump_notes (notes);
	BOOST_REQUIRE_EQUAL (notes.size(), types_and_codes.size());
	auto i = notes.begin();
	auto j = types_and_codes.begin();
	while (i != notes.end()) {
		BOOST_CHECK_EQUAL (i->type(), j->first);
		BOOST_CHECK_EQUAL (i->code(), j->second);
		++i;
		++j;
	}
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

	boost::filesystem::path const cpl_file = "build/test/verify_test1/cpl_81fb54df-e1bf-4647-8788-ea7ba154375b.xml";
	boost::filesystem::path const pkl_file = "build/test/verify_test1/pkl_cd49971e-bf4c-4594-8474-54ebef09a40c.xml";
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
		Editor e ("build/test/verify_test3/pkl_cd49971e-bf4c-4594-8474-54ebef09a40c.xml");
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
		Editor e ("build/test/verify_test4/cpl_81fb54df-e1bf-4647-8788-ea7ba154375b.xml");
		e.replace ("<ContentKind>", "<ContentKind>x");
	}

	auto notes = dcp::verify (directories, &stage, &progress, xsd_test);

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
		"<Id>urn:uuid:cd4", "<Id>urn:uuid:xd4",
		{ dcp::VerificationNote::XML_VALIDATION_ERROR }
		);
}

/* Badly-formatted <Id> in ASSETMAP */
BOOST_AUTO_TEST_CASE (verify_test12)
{
	check_verify_result_after_replace (
		12, &asset_map,
		"<Id>urn:uuid:63c", "<Id>urn:uuid:x3c",
		{ dcp::VerificationNote::XML_VALIDATION_ERROR }
		);
}

/* Basic test of an Interop DCP */
BOOST_AUTO_TEST_CASE (verify_test13)
{
	stages.clear ();
	auto directories = setup (3, 13);
	auto notes = dcp::verify (directories, &stage, &progress, xsd_test);

	boost::filesystem::path const cpl_file = "build/test/verify_test13/cpl_cbfd2bc0-21cf-4a8f-95d8-9cddcbe51296.xml";
	boost::filesystem::path const pkl_file = "build/test/verify_test13/pkl_d87a950c-bd6f-41f6-90cc-56ccd673e131.xml";
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
		{{ dcp::VerificationNote::VERIFY_ERROR, dcp::VerificationNote::PICTURE_FRAME_TOO_LARGE_IN_BYTES }}
		);
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
		{{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::PICTURE_FRAME_NEARLY_TOO_LARGE_IN_BYTES }}
		);
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

	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), 0);
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

	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), 0);
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
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::MISSING_SUBTITLE_START_TIME }
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
		{{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::EXTERNAL_ASSET }});
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
	auto cpl = make_shared<dcp::CPL>("hello", dcp::FEATURE);
	cpl->add (reel);
	cpl->set_main_sound_configuration ("L,C,R,Lfe,-,-");
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));

	dcp::DCP dcp (dir);
	dcp.add (cpl);
	dcp.write_xml (dcp::SMPTE);

	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);
	BOOST_CHECK (notes.empty());
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
	auto cpl = make_shared<dcp::CPL>("hello", dcp::FEATURE);
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
	auto cpl = make_shared<dcp::CPL>("hello", dcp::FEATURE);
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
	BOOST_REQUIRE_EQUAL (notes.size(), 2U);
	auto i = notes.begin();
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::BAD_LANGUAGE);
	BOOST_REQUIRE (i->note());
	BOOST_CHECK_EQUAL (*i->note(), "badlang");
	i++;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::BAD_LANGUAGE);
	BOOST_REQUIRE (i->note());
	BOOST_CHECK_EQUAL (*i->note(), "wrong-andbad");
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
	BOOST_REQUIRE_EQUAL (notes.size(), 2U);
	auto i = notes.begin ();
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::BAD_LANGUAGE);
	BOOST_REQUIRE (i->note());
	BOOST_CHECK_EQUAL (*i->note(), "badlang");
	i++;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::BAD_LANGUAGE);
	BOOST_REQUIRE (i->note());
	BOOST_CHECK_EQUAL (*i->note(), "wrong-andbad");
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
	auto cpl = make_shared<dcp::CPL>("hello", dcp::FEATURE);
	cpl->add (reel);
	cpl->_additional_subtitle_languages.push_back("this-is-wrong");
	cpl->_additional_subtitle_languages.push_back("andso-is-this");
	cpl->set_main_sound_configuration ("L,C,R,Lfe,-,-");
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));
	cpl->_release_territory = "fred-jim";
	auto dcp = make_shared<dcp::DCP>(dir);
	dcp->add (cpl);
	dcp->write_xml (dcp::SMPTE);

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
	++i;
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
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::FEATURE);
	cpl->set_annotation_text ("A Test DCP");
	cpl->set_issue_date ("2012-07-17T04:45:18+00:00");

	auto reel = make_shared<dcp::Reel>();

	if (three_d) {
		reel->add (make_shared<dcp::ReelStereoPictureAsset>(std::dynamic_pointer_cast<dcp::StereoPictureAsset>(mp), 0));
	} else {
		reel->add (make_shared<dcp::ReelMonoPictureAsset>(std::dynamic_pointer_cast<dcp::MonoPictureAsset>(mp), 0));
	}

	cpl->add (reel);

	d->add (cpl);
	d->write_xml (dcp::SMPTE);

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
add_test_subtitle (shared_ptr<dcp::SubtitleAsset> asset, int start_frame, int end_frame)
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
			0,
			dcp::VALIGN_CENTER,
			dcp::DIRECTION_LTR,
			"Hello",
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
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::FIRST_TEXT_TOO_EARLY }
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
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::FIRST_TEXT_TOO_EARLY }
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
	auto dcp = make_simple (dir, 1);

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

	auto reel_subs = make_shared<dcp::ReelSubtitleAsset>(subs, dcp::Fraction(24, 1), 100, 0);
	dcp->cpls().front()->reels().front()->add(reel_subs);
	dcp->write_xml (dcp::SMPTE);

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

	dcp->write_xml (dcp::SMPTE);

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
	auto dcp = make_simple (dir, 1);

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

	auto reel_subs = make_shared<dcp::ReelSubtitleAsset>(subs, dcp::Fraction(24, 1), 100, 0);
	dcp->cpls().front()->reels().front()->add(reel_subs);
	dcp->write_xml (dcp::SMPTE);

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
	auto dcp = make_simple (dir, 1);

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

	auto reel_subs = make_shared<dcp::ReelSubtitleAsset>(subs, dcp::Fraction(24, 1), 100, 0);
	dcp->cpls().front()->reels().front()->add(reel_subs);
	dcp->write_xml (dcp::SMPTE);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::VERIFY_BV21_ERROR, dcp::VerificationNote::SUBTITLE_START_TIME_NON_ZERO },
			{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::FIRST_TEXT_TOO_EARLY }
		});
}


static
void
dcp_with_subtitles (boost::filesystem::path dir, vector<int> timings)
{
	prepare_directory (dir);
	auto asset = make_shared<dcp::SMPTESubtitleAsset>();
	asset->set_start_time (dcp::Time());
	for (auto i = 0U; i < timings.size(); i += 2) {
		add_test_subtitle (asset, timings[i], timings[i + 1]);
	}
	asset->set_language (dcp::LanguageTag("de-DE"));
	asset->write (dir / "subs.mxf");

	auto reel_asset = make_shared<dcp::ReelSubtitleAsset>(asset, dcp::Fraction(24, 1), 16 * 24, 0);
	write_dcp_with_single_asset (dir, reel_asset);
}


BOOST_AUTO_TEST_CASE (verify_text_too_early)
{
	auto const dir = boost::filesystem::path("build/test/verify_text_too_early");
	/* Just too early */
	dcp_with_subtitles (dir, { 4 * 24 - 1, 5 * 24 });
	check_verify_result (
		{ dir },
		{{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::FIRST_TEXT_TOO_EARLY }});
}


BOOST_AUTO_TEST_CASE (verify_text_not_too_early)
{
	auto const dir = boost::filesystem::path("build/test/verify_text_not_too_early");
	/* Just late enough */
	dcp_with_subtitles (dir, { 4 * 24, 5 * 24 });
	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);
	BOOST_REQUIRE (notes.empty());
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

	auto asset2 = make_shared<dcp::SMPTESubtitleAsset>();
	asset2->set_start_time (dcp::Time());
	/* This would be too early on first reel but should be OK on the second */
	add_test_subtitle (asset2, 0, 4 * 24);
	asset2->set_language (dcp::LanguageTag("de-DE"));
	asset2->write (dir / "subs2.mxf");
	auto reel_asset2 = make_shared<dcp::ReelSubtitleAsset>(asset2, dcp::Fraction(24, 1), 16 * 24, 0);
	auto reel2 = make_shared<dcp::Reel>();
	reel2->add (reel_asset2);

	auto cpl = make_shared<dcp::CPL>("hello", dcp::FEATURE);
	cpl->add (reel1);
	cpl->add (reel2);
	auto dcp = make_shared<dcp::DCP>(dir);
	dcp->add (cpl);
	dcp->write_xml (dcp::SMPTE);

	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);
	BOOST_REQUIRE (notes.empty());
}


BOOST_AUTO_TEST_CASE (verify_text_too_close)
{
	auto const dir = boost::filesystem::path("build/test/verify_text_too_close");
	dcp_with_subtitles (
		dir,
		{
		  4 * 24,     5 * 24,
		  5 * 24 + 1, 6 * 24,
		});
	check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::SUBTITLE_TOO_CLOSE }});
}


BOOST_AUTO_TEST_CASE (verify_text_not_too_close)
{
	auto const dir = boost::filesystem::path("build/test/verify_text_not_too_close");
	dcp_with_subtitles (
		dir,
		{
		  4 * 24,     5 * 24,
		  5 * 24 + 16, 8 * 24,
		});
	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);
	BOOST_REQUIRE (notes.empty());
}


BOOST_AUTO_TEST_CASE (verify_text_too_short)
{
	auto const dir = boost::filesystem::path("build/test/verify_text_too_short");
	dcp_with_subtitles (
		dir,
		{
		  4 * 24,     4 * 24 + 1,
		});
	check_verify_result ({dir}, {{ dcp::VerificationNote::VERIFY_WARNING, dcp::VerificationNote::SUBTITLE_TOO_SHORT }});
}


BOOST_AUTO_TEST_CASE (verify_text_not_too_short)
{
	auto const dir = boost::filesystem::path("build/test/verify_text_not_too_short");
	dcp_with_subtitles (
		dir,
		{
		  4 * 24,     4 * 24 + 17,
		});
	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);
	BOOST_REQUIRE (notes.empty());
}

