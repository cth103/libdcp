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
using boost::shared_ptr;


static list<pair<string, optional<boost::filesystem::path> > > stages;
static int next_verify_test_number = 1;

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
	stages.clear ();
	vector<boost::filesystem::path> directories = setup (1, next_verify_test_number);
	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, "xsd");

	boost::filesystem::path const cpl_file = dcp::String::compose("build/test/verify_test%1/cpl_81fb54df-e1bf-4647-8788-ea7ba154375b.xml", next_verify_test_number);
	boost::filesystem::path const pkl_file = dcp::String::compose("build/test/verify_test1/pkl_ae8a9818-872a-4f86-8493-11dfdea03e09.xml", next_verify_test_number);
	boost::filesystem::path const assetmap_file = dcp::String::compose("build/test/verify_test1/ASSETMAP.xml", next_verify_test_number);

	list<pair<string, optional<boost::filesystem::path> > >::const_iterator st = stages.begin();
	BOOST_CHECK_EQUAL (st->first, "Checking DCP");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical(dcp::String::compose("build/test/verify_test%1", next_verify_test_number)));
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
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical(dcp::String::compose("build/test/verify_test%1/video.mxf", next_verify_test_number)));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking picture frame sizes");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical(dcp::String::compose("build/test/verify_test%1/video.mxf", next_verify_test_number)));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking sound asset hash");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical(dcp::String::compose("build/test/verify_test%1/audio.mxf", next_verify_test_number)));
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

	dump_notes (notes);

	BOOST_CHECK_EQUAL (notes.size(), 0);

	next_verify_test_number++;
}

/* Corrupt the MXFs and check that this is spotted */
BOOST_AUTO_TEST_CASE (verify_test2)
{
	vector<boost::filesystem::path> directories = setup (1, next_verify_test_number++);

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
	vector<boost::filesystem::path> directories = setup (1, next_verify_test_number++);

	{
		Editor e ("build/test/verify_test3/pkl_ae8a9818-872a-4f86-8493-11dfdea03e09.xml");
		e.replace ("<Hash>", "<Hash>x");
	}

	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, "xsd");

	dump_notes (notes);

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
	vector<boost::filesystem::path> directories = setup (1, next_verify_test_number++);

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
boost::filesystem::path
pkl (int n)
{
	return dcp::String::compose("build/test/verify_test%1/pkl_ae8a9818-872a-4f86-8493-11dfdea03e09.xml", n);
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

	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, "xsd");

	dump_notes (notes);

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
	vector<boost::filesystem::path> directories = setup (1, n);

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
			next_verify_test_number++, &cpl,
			"<FrameRate>24 1", "<FrameRate>99 1",
			dcp::VerificationNote::CPL_HASH_INCORRECT,
			dcp::VerificationNote::INVALID_PICTURE_FRAME_RATE
			);
}

/* Missing asset */
BOOST_AUTO_TEST_CASE (verify_test6)
{
	vector<boost::filesystem::path> directories = setup (1, next_verify_test_number++);

	boost::filesystem::remove ("build/test/verify_test6/video.mxf");
	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, "xsd");

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
			next_verify_test_number++, &assetmap,
			"<Path>video.mxf</Path>", "<Path></Path>",
			dcp::VerificationNote::EMPTY_ASSET_PATH
			);
}

/* Mismatched standard */
BOOST_AUTO_TEST_CASE (verify_test8)
{
	check_after_replace (
			next_verify_test_number++, &cpl,
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
			next_verify_test_number++, &cpl,
			"<Id>urn:uuid:81fb54df-e1bf-4647-8788-ea7ba154375b", "<Id>urn:uuid:81fb54df-e1bf-4647-8788-ea7ba154375",
			dcp::VerificationNote::XML_VALIDATION_ERROR
			);
}

/* Badly formatted <IssueDate> in CPL */
BOOST_AUTO_TEST_CASE (verify_test10)
{
	check_after_replace (
			next_verify_test_number++, &cpl,
			"<IssueDate>", "<IssueDate>x",
			dcp::VerificationNote::XML_VALIDATION_ERROR,
			dcp::VerificationNote::CPL_HASH_INCORRECT
			);
}

/* Badly-formatted <Id> in PKL */
BOOST_AUTO_TEST_CASE (verify_test11)
{
	check_after_replace (
		next_verify_test_number++, &pkl,
		"<Id>urn:uuid:ae8", "<Id>urn:uuid:xe8",
		dcp::VerificationNote::XML_VALIDATION_ERROR
		);
}

/* Badly-formatted <Id> in ASSETMAP */
BOOST_AUTO_TEST_CASE (verify_test12)
{
	check_after_replace (
		next_verify_test_number++, &asset_map,
		"<Id>urn:uuid:74e", "<Id>urn:uuid:x4e",
		dcp::VerificationNote::XML_VALIDATION_ERROR
		);
}

/* Basic test of an Interop DCP */
BOOST_AUTO_TEST_CASE (verify_test13)
{
	stages.clear ();
	vector<boost::filesystem::path> directories = setup (3, next_verify_test_number);
	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, "xsd");

	boost::filesystem::path const cpl_file = dcp::String::compose("build/test/verify_test%1/cpl_cbfd2bc0-21cf-4a8f-95d8-9cddcbe51296.xml", next_verify_test_number);
	boost::filesystem::path const pkl_file = dcp::String::compose("build/test/verify_test%1/pkl_d87a950c-bd6f-41f6-90cc-56ccd673e131.xml", next_verify_test_number);
	boost::filesystem::path const assetmap_file = dcp::String::compose("build/test/verify_test%1/ASSETMAP", next_verify_test_number);

	list<pair<string, optional<boost::filesystem::path> > >::const_iterator st = stages.begin();
	BOOST_CHECK_EQUAL (st->first, "Checking DCP");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical(dcp::String::compose("build/test/verify_test%1", next_verify_test_number)));
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
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical(dcp::String::compose("build/test/verify_test%1/j2c_c6035f97-b07d-4e1c-944d-603fc2ddc242.mxf", next_verify_test_number)));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking picture frame sizes");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical(dcp::String::compose("build/test/verify_test%1/j2c_c6035f97-b07d-4e1c-944d-603fc2ddc242.mxf", next_verify_test_number)));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking sound asset hash");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), boost::filesystem::canonical(dcp::String::compose("build/test/verify_test%1/pcm_69cf9eaf-9a99-4776-b022-6902208626c3.mxf", next_verify_test_number)));
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

	dump_notes (notes);

	BOOST_CHECK_EQUAL (notes.size(), 0);

	next_verify_test_number++;
}

/* DCP with a short asset */
BOOST_AUTO_TEST_CASE (verify_test14)
{
	vector<boost::filesystem::path> directories = setup (8, next_verify_test_number);
	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, "xsd");

	dump_notes (notes);

	BOOST_REQUIRE_EQUAL (notes.size(), 4);
	list<dcp::VerificationNote>::const_iterator i = notes.begin ();
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::DURATION_TOO_SMALL);
	++i;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::INTRINSIC_DURATION_TOO_SMALL);
	++i;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::DURATION_TOO_SMALL);
	++i;
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::INTRINSIC_DURATION_TOO_SMALL);
	++i;
	next_verify_test_number++;
}


static
shared_ptr<dcp::OpenJPEGImage>
random_image ()
{
	shared_ptr<dcp::OpenJPEGImage> image(new dcp::OpenJPEGImage(dcp::Size(1998, 1080)));
	int const pixels = 1998 * 1080;
	for (int i = 0; i < 3; ++i) {
		int* p = image->data(i);
		for (int j = 0; j < pixels; ++j) {
			*p++ = rand();
		}
	}
	return image;
}


static
void
dcp_from_frame (dcp::Data const& frame, boost::filesystem::path dir)
{
	shared_ptr<dcp::MonoPictureAsset> asset(new dcp::MonoPictureAsset(dcp::Fraction(24, 1), dcp::SMPTE));
	boost::filesystem::create_directories (dir);
	shared_ptr<dcp::PictureAssetWriter> writer = asset->start_write (dir / "pic.mxf", true);
	for (int i = 0; i < 24; ++i) {
		writer->write (frame.data().get(), frame.size());
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
	/* Compress a random image with a bandwidth of 500Mbit/s */
	shared_ptr<dcp::OpenJPEGImage> image = random_image ();
	dcp::Data frame = dcp::compress_j2k (image, 500000000, 24, false, false);

	boost::filesystem::path const dir("build/test/verify_test15");
	dcp_from_frame (frame, dir);

	vector<boost::filesystem::path> dirs;
	dirs.push_back (dir);
	list<dcp::VerificationNote> notes = dcp::verify (dirs, &stage, &progress, "xsd");
	BOOST_REQUIRE_EQUAL (notes.size(), 1);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::PICTURE_FRAME_TOO_LARGE);
}


/* DCP with a nearly over-sized JPEG2000 frame */
BOOST_AUTO_TEST_CASE (verify_test16)
{
	/* Compress a random image with a bandwidth of 500Mbit/s */
	shared_ptr<dcp::OpenJPEGImage> image = random_image ();
	dcp::Data frame = dcp::compress_j2k (image, 240000000, 24, false, false);

	boost::filesystem::path const dir("build/test/verify_test16");
	dcp_from_frame (frame, dir);

	vector<boost::filesystem::path> dirs;
	dirs.push_back (dir);
	list<dcp::VerificationNote> notes = dcp::verify (dirs, &stage, &progress, "xsd");
	BOOST_REQUIRE_EQUAL (notes.size(), 1);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::PICTURE_FRAME_NEARLY_TOO_LARGE);
}


/* DCP with a within-range JPEG2000 frame */
BOOST_AUTO_TEST_CASE (verify_test17)
{
	/* Compress a random image with a bandwidth of 500Mbit/s */
	shared_ptr<dcp::OpenJPEGImage> image = random_image ();
	dcp::Data frame = dcp::compress_j2k (image, 100000000, 24, false, false);

	boost::filesystem::path const dir("build/test/verify_test17");
	dcp_from_frame (frame, dir);

	vector<boost::filesystem::path> dirs;
	dirs.push_back (dir);
	list<dcp::VerificationNote> notes = dcp::verify (dirs, &stage, &progress, "xsd");
	BOOST_REQUIRE_EQUAL (notes.size(), 0);
}

