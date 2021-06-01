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

#include "compose.hpp"
#include "cpl.h"
#include "dcp.h"
#include "interop_subtitle_asset.h"
#include "j2k_transcode.h"
#include "mono_picture_asset.h"
#include "mono_picture_asset_writer.h"
#include "openjpeg_image.h"
#include "raw_convert.h"
#include "reel.h"
#include "reel_interop_closed_caption_asset.h"
#include "reel_interop_subtitle_asset.h"
#include "reel_markers_asset.h"
#include "reel_mono_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_stereo_picture_asset.h"
#include "reel_smpte_closed_caption_asset.h"
#include "reel_smpte_subtitle_asset.h"
#include "smpte_subtitle_asset.h"
#include "stereo_picture_asset.h"
#include "stream_operators.h"
#include "test.h"
#include "util.h"
#include "verify.h"
#include "verify_j2k.h"
#include <boost/test/unit_test.hpp>
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
using namespace boost::filesystem;
using std::shared_ptr;


static list<pair<string, optional<path>>> stages;

static string filename_to_id(boost::filesystem::path path)
{
	return path.string().substr(4, path.string().length() - 8);
}

static boost::filesystem::path const dcp_test1_pkl = find_file("test/ref/DCP/dcp_test1", "pkl_").filename();
static string const dcp_test1_pkl_id = filename_to_id(dcp_test1_pkl);

static boost::filesystem::path const dcp_test1_cpl = find_file("test/ref/DCP/dcp_test1", "cpl_").filename();
static string const dcp_test1_cpl_id = filename_to_id(dcp_test1_cpl);

static string const dcp_test1_asset_map_id = "5d51e8a1-b2a5-4da6-9b66-4615c3609440";

static boost::filesystem::path const encryption_test_cpl = find_file("test/ref/DCP/encryption_test", "cpl_").filename();
static string const encryption_test_cpl_id = filename_to_id(encryption_test_cpl);

static boost::filesystem::path const encryption_test_pkl = find_file("test/ref/DCP/encryption_test", "pkl_").filename();
static string const encryption_test_pkl_id = filename_to_id(encryption_test_pkl);

static void
stage (string s, optional<path> p)
{
	stages.push_back (make_pair (s, p));
}

static void
progress (float)
{

}

static void
prepare_directory (path path)
{
	using namespace boost::filesystem;
	remove_all (path);
	create_directories (path);
}


static path
setup (int reference_number, string verify_test_suffix)
{
	auto const dir = dcp::String::compose("build/test/verify_test%1", verify_test_suffix);
	prepare_directory (dir);
	for (auto i: directory_iterator(dcp::String::compose("test/ref/DCP/dcp_test%1", reference_number))) {
		copy_file (i.path(), dir / i.path().filename());
	}

	return dir;
}


static
shared_ptr<dcp::CPL>
write_dcp_with_single_asset (path dir, shared_ptr<dcp::ReelAsset> reel_asset, dcp::Standard standard = dcp::Standard::SMPTE)
{
	auto reel = make_shared<dcp::Reel>();
	reel->add (reel_asset);
	reel->add (simple_markers());

	auto cpl = make_shared<dcp::CPL>("hello", dcp::ContentKind::TRAILER, standard);
	cpl->add (reel);
	auto dcp = make_shared<dcp::DCP>(dir);
	dcp->add (cpl);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"hello"
		);

	return cpl;
}


/** Class that can alter a file by searching and replacing strings within it.
 *  On destruction modifies the file whose name was given to the constructor.
 */
class Editor
{
public:
	Editor (path path)
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

	void delete_first_line_containing (string s)
	{
		vector<string> lines;
		boost::algorithm::split (lines, _content, boost::is_any_of("\r\n"), boost::token_compress_on);
		auto old_content = _content;
		_content = "";
		bool done = false;
		for (auto i: lines) {
			if (i.find(s) == string::npos || done) {
				_content += i + "\n";
			} else {
				done = true;
			}
		}
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
	path _path;
	std::string _content;
};


LIBDCP_DISABLE_WARNINGS
static
void
dump_notes (vector<dcp::VerificationNote> const & notes)
{
	for (auto i: notes) {
		std::cout << dcp::note_to_string(i) << "\n";
	}
}
LIBDCP_ENABLE_WARNINGS


static
void
check_verify_result (vector<path> dir, vector<dcp::VerificationNote> test_notes)
{
	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);
	BOOST_REQUIRE_EQUAL (notes.size(), test_notes.size());
	std::sort (notes.begin(), notes.end());
	std::sort (test_notes.begin(), test_notes.end());
	for (auto i = 0U; i < notes.size(); ++i) {
		BOOST_REQUIRE_MESSAGE (notes[i] == test_notes[i], "Note from verify:\n" << notes[i] << "\ndoes not match the expected:\n" << test_notes[i]);
	}
}


static
void
check_verify_result_after_replace (string suffix, boost::function<path (string)> file, string from, string to, vector<dcp::VerificationNote::Code> codes)
{
	auto dir = setup (1, suffix);

	{
		Editor e (file(suffix));
		e.replace (from, to);
	}

	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);

	BOOST_REQUIRE_EQUAL (notes.size(), codes.size());
	auto i = notes.begin();
	auto j = codes.begin();
	while (i != notes.end()) {
		BOOST_CHECK_EQUAL (i->code(), *j);
		++i;
		++j;
	}
}


BOOST_AUTO_TEST_CASE (verify_no_error)
{
	stages.clear ();
	auto dir = setup (1, "no_error");
	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);

	path const cpl_file = dir / dcp_test1_cpl;
	path const pkl_file = dir / dcp_test1_pkl;
	path const assetmap_file = dir / "ASSETMAP.xml";

	auto st = stages.begin();
	BOOST_CHECK_EQUAL (st->first, "Checking DCP");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(dir));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking CPL");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(cpl_file));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking reel");
	BOOST_REQUIRE (!st->second);
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking picture asset hash");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(dir / "video.mxf"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking picture frame sizes");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(dir / "video.mxf"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking sound asset hash");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(dir / "audio.mxf"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking sound asset metadata");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(dir / "audio.mxf"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking PKL");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(pkl_file));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking ASSETMAP");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(assetmap_file));
	++st;
	BOOST_REQUIRE (st == stages.end());

	BOOST_CHECK_EQUAL (notes.size(), 0);
}


BOOST_AUTO_TEST_CASE (verify_incorrect_picture_sound_hash)
{
	using namespace boost::filesystem;

	auto dir = setup (1, "incorrect_picture_sound_hash");

	auto video_path = path(dir / "video.mxf");
	auto mod = fopen(video_path.string().c_str(), "r+b");
	BOOST_REQUIRE (mod);
	fseek (mod, 4096, SEEK_SET);
	int x = 42;
	fwrite (&x, sizeof(x), 1, mod);
	fclose (mod);

	auto audio_path = path(dir / "audio.mxf");
	mod = fopen(audio_path.string().c_str(), "r+b");
	BOOST_REQUIRE (mod);
	BOOST_REQUIRE_EQUAL (fseek(mod, -64, SEEK_END), 0);
	BOOST_REQUIRE (fwrite (&x, sizeof(x), 1, mod) == 1);
	fclose (mod);

	dcp::ASDCPErrorSuspender sus;
	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INCORRECT_PICTURE_HASH, canonical(video_path) },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INCORRECT_SOUND_HASH, canonical(audio_path) },
		});
}


BOOST_AUTO_TEST_CASE (verify_mismatched_picture_sound_hashes)
{
	using namespace boost::filesystem;

	auto dir = setup (1, "mismatched_picture_sound_hashes");

	{
		Editor e (dir / dcp_test1_pkl);
		e.replace ("<Hash>", "<Hash>x");
	}

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, dcp_test1_cpl_id, canonical(dir / dcp_test1_cpl) },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_PICTURE_HASHES, canonical(dir / "video.mxf") },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_SOUND_HASHES, canonical(dir / "audio.mxf") },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, "value 'xD9HpNAjKsECGtJEWtwV2/T5ndG8=' is invalid Base64-encoded binary", canonical(dir / dcp_test1_pkl), 26 },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, "value 'xgVKhC9IkWyzQbgzpFcJ1bpqbtwk=' is invalid Base64-encoded binary", canonical(dir / dcp_test1_pkl), 19 },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, "value 'xz/y4fI+ocWzVB7cr8pVt2OYpOHA=' is invalid Base64-encoded binary", canonical(dir / dcp_test1_pkl), 12 }
		});
}


BOOST_AUTO_TEST_CASE (verify_failed_read_content_kind)
{
	auto dir = setup (1, "failed_read_content_kind");

	{
		Editor e (dir / dcp_test1_cpl);
		e.replace ("<ContentKind>", "<ContentKind>x");
	}

	check_verify_result (
		{ dir },
		{{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::FAILED_READ, string("Bad content kind 'xtrailer'")}}
		);
}


static
path
cpl (string suffix)
{
	return dcp::String::compose("build/test/verify_test%1/%2", suffix, dcp_test1_cpl);
}


static
path
pkl (string suffix)
{
	return dcp::String::compose("build/test/verify_test%1/%2", suffix, dcp_test1_pkl);
}


static
path
asset_map (string suffix)
{
	return dcp::String::compose("build/test/verify_test%1/ASSETMAP.xml", suffix);
}


BOOST_AUTO_TEST_CASE (verify_invalid_picture_frame_rate)
{
	check_verify_result_after_replace (
			"invalid_picture_frame_rate", &cpl,
			"<FrameRate>24 1", "<FrameRate>99 1",
			{ dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES,
			  dcp::VerificationNote::Code::INVALID_PICTURE_FRAME_RATE }
			);
}

BOOST_AUTO_TEST_CASE (verify_missing_asset)
{
	auto dir = setup (1, "missing_asset");
	remove (dir / "video.mxf");
	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISSING_ASSET, canonical(dir) / "video.mxf" }
		});
}


BOOST_AUTO_TEST_CASE (verify_empty_asset_path)
{
	check_verify_result_after_replace (
			"empty_asset_path", &asset_map,
			"<Path>video.mxf</Path>", "<Path></Path>",
			{ dcp::VerificationNote::Code::EMPTY_ASSET_PATH }
			);
}


BOOST_AUTO_TEST_CASE (verify_mismatched_standard)
{
	check_verify_result_after_replace (
			"mismatched_standard", &cpl,
			"http://www.smpte-ra.org/schemas/429-7/2006/CPL", "http://www.digicine.com/PROTO-ASDCP-CPL-20040511#",
			{ dcp::VerificationNote::Code::MISMATCHED_STANDARD,
			  dcp::VerificationNote::Code::INVALID_XML,
			  dcp::VerificationNote::Code::INVALID_XML,
			  dcp::VerificationNote::Code::INVALID_XML,
			  dcp::VerificationNote::Code::INVALID_XML,
			  dcp::VerificationNote::Code::INVALID_XML,
			  dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES }
			);
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_cpl_id)
{
	/* There's no MISMATCHED_CPL_HASHES error here because it can't find the correct hash by ID (since the ID is wrong) */
	check_verify_result_after_replace (
			"invalid_xml_cpl_id", &cpl,
			"<Id>urn:uuid:81fb54df-e1bf-4647-8788-ea7ba154375b", "<Id>urn:uuid:81fb54df-e1bf-4647-8788-ea7ba154375",
			{ dcp::VerificationNote::Code::INVALID_XML }
			);
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_issue_date)
{
	check_verify_result_after_replace (
			"invalid_xml_issue_date", &cpl,
			"<IssueDate>", "<IssueDate>x",
			{ dcp::VerificationNote::Code::INVALID_XML,
			  dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES }
			);
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_pkl_id)
{
	check_verify_result_after_replace (
		"invalid_xml_pkl_id", &pkl,
		"<Id>urn:uuid:" + dcp_test1_pkl_id.substr(0, 3),
		"<Id>urn:uuid:x" + dcp_test1_pkl_id.substr(1, 2),
		{ dcp::VerificationNote::Code::INVALID_XML }
		);
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_asset_map_id)
{
	check_verify_result_after_replace (
		"invalix_xml_asset_map_id", &asset_map,
		"<Id>urn:uuid:" + dcp_test1_asset_map_id.substr(0, 3),
		"<Id>urn:uuid:x" + dcp_test1_asset_map_id.substr(1, 2),
		{ dcp::VerificationNote::Code::INVALID_XML }
		);
}


BOOST_AUTO_TEST_CASE (verify_invalid_standard)
{
	stages.clear ();
	auto dir = setup (3, "verify_invalid_standard");
	auto notes = dcp::verify ({dir}, &stage, &progress, xsd_test);

	path const cpl_file = dir / "cpl_cbfd2bc0-21cf-4a8f-95d8-9cddcbe51296.xml";
	path const pkl_file = dir / "pkl_d87a950c-bd6f-41f6-90cc-56ccd673e131.xml";
	path const assetmap_file = dir / "ASSETMAP";

	auto st = stages.begin();
	BOOST_CHECK_EQUAL (st->first, "Checking DCP");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(dir));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking CPL");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(cpl_file));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking reel");
	BOOST_REQUIRE (!st->second);
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking picture asset hash");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(dir / "j2c_c6035f97-b07d-4e1c-944d-603fc2ddc242.mxf"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking picture frame sizes");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(dir / "j2c_c6035f97-b07d-4e1c-944d-603fc2ddc242.mxf"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking sound asset hash");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(dir / "pcm_69cf9eaf-9a99-4776-b022-6902208626c3.mxf"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking sound asset metadata");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(dir / "pcm_69cf9eaf-9a99-4776-b022-6902208626c3.mxf"));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking PKL");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(pkl_file));
	++st;
	BOOST_CHECK_EQUAL (st->first, "Checking ASSETMAP");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(assetmap_file));
	++st;
	BOOST_REQUIRE (st == stages.end());

	BOOST_REQUIRE_EQUAL (notes.size(), 2U);
	auto i = notes.begin ();
	BOOST_CHECK_EQUAL (i->type(), dcp::VerificationNote::Type::BV21_ERROR);
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::Code::INVALID_STANDARD);
	++i;
	BOOST_CHECK_EQUAL (i->type(), dcp::VerificationNote::Type::BV21_ERROR);
	BOOST_CHECK_EQUAL (i->code(), dcp::VerificationNote::Code::INVALID_JPEG2000_GUARD_BITS_FOR_2K);
}

/* DCP with a short asset */
BOOST_AUTO_TEST_CASE (verify_invalid_duration)
{
	auto dir = setup (8, "invalid_duration");
	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_STANDARD },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_DURATION, string("d7576dcb-a361-4139-96b8-267f5f8d7f91") },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_INTRINSIC_DURATION, string("d7576dcb-a361-4139-96b8-267f5f8d7f91") },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_DURATION, string("a2a87f5d-b749-4a7e-8d0c-9d48a4abf626") },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_INTRINSIC_DURATION, string("a2a87f5d-b749-4a7e-8d0c-9d48a4abf626") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_JPEG2000_GUARD_BITS_FOR_2K, string("2") }
		});
}


static
shared_ptr<dcp::CPL>
dcp_from_frame (dcp::ArrayData const& frame, path dir)
{
	auto asset = make_shared<dcp::MonoPictureAsset>(dcp::Fraction(24, 1), dcp::Standard::SMPTE);
	create_directories (dir);
	auto writer = asset->start_write (dir / "pic.mxf", true);
	for (int i = 0; i < 24; ++i) {
		writer->write (frame.data(), frame.size());
	}
	writer->finalize ();

	auto reel_asset = make_shared<dcp::ReelMonoPictureAsset>(asset, 0);
	return write_dcp_with_single_asset (dir, reel_asset);
}


BOOST_AUTO_TEST_CASE (verify_invalid_picture_frame_size_in_bytes)
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

	path const dir("build/test/verify_invalid_picture_frame_size_in_bytes");
	prepare_directory (dir);
	auto cpl = dcp_from_frame (oversized_frame, dir);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_JPEG2000_CODESTREAM, string("missing marker start byte") },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_PICTURE_FRAME_SIZE_IN_BYTES, canonical(dir / "pic.mxf") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


BOOST_AUTO_TEST_CASE (verify_nearly_invalid_picture_frame_size_in_bytes)
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

	path const dir("build/test/verify_nearly_invalid_picture_frame_size_in_bytes");
	prepare_directory (dir);
	auto cpl = dcp_from_frame (oversized_frame, dir);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_JPEG2000_CODESTREAM, string("missing marker start byte") },
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::NEARLY_INVALID_PICTURE_FRAME_SIZE_IN_BYTES, canonical(dir / "pic.mxf") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_picture_frame_size_in_bytes)
{
	/* Compress a black image */
	auto image = black_image ();
	auto frame = dcp::compress_j2k (image, 100000000, 24, false, false);
	BOOST_REQUIRE (frame.size() < 230000000 / (24 * 8));

	path const dir("build/test/verify_valid_picture_frame_size_in_bytes");
	prepare_directory (dir);
	auto cpl = dcp_from_frame (frame, dir);

	check_verify_result ({ dir }, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }});
}


BOOST_AUTO_TEST_CASE (verify_valid_interop_subtitles)
{
	path const dir("build/test/verify_valid_interop_subtitles");
	prepare_directory (dir);
	copy_file ("test/data/subs1.xml", dir / "subs.xml");
	auto asset = make_shared<dcp::InteropSubtitleAsset>(dir / "subs.xml");
	auto reel_asset = make_shared<dcp::ReelInteropSubtitleAsset>(asset, dcp::Fraction(24, 1), 16 * 24, 0);
	write_dcp_with_single_asset (dir, reel_asset, dcp::Standard::INTEROP);

	check_verify_result ({dir}, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_STANDARD }});
}


BOOST_AUTO_TEST_CASE (verify_invalid_interop_subtitles)
{
	using namespace boost::filesystem;

	path const dir("build/test/verify_invalid_interop_subtitles");
	prepare_directory (dir);
	copy_file ("test/data/subs1.xml", dir / "subs.xml");
	auto asset = make_shared<dcp::InteropSubtitleAsset>(dir / "subs.xml");
	auto reel_asset = make_shared<dcp::ReelInteropSubtitleAsset>(asset, dcp::Fraction(24, 1), 16 * 24, 0);
	write_dcp_with_single_asset (dir, reel_asset, dcp::Standard::INTEROP);

	{
		Editor e (dir / "subs.xml");
		e.replace ("</ReelNumber>", "</ReelNumber><Foo></Foo>");
	}

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_STANDARD },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'Foo'"), path(), 5 },
			{
				dcp::VerificationNote::Type::ERROR,
				dcp::VerificationNote::Code::INVALID_XML,
				string("element 'Foo' is not allowed for content model '(SubtitleID,MovieTitle,ReelNumber,Language,LoadFont*,Font*,Subtitle*)'"),
				path(),
				29
			}
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_smpte_subtitles)
{
	path const dir("build/test/verify_valid_smpte_subtitles");
	prepare_directory (dir);
	copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTESubtitleAsset>(dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSMPTESubtitleAsset>(asset, dcp::Fraction(24, 1), 6046, 0);
	auto cpl = write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result ({dir}, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }});
}


BOOST_AUTO_TEST_CASE (verify_invalid_smpte_subtitles)
{
	using namespace boost::filesystem;

	path const dir("build/test/verify_invalid_smpte_subtitles");
	prepare_directory (dir);
	/* This broken_smpte.mxf does not use urn:uuid: for its subtitle ID, which we tolerate (rightly or wrongly) */
	copy_file ("test/data/broken_smpte.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTESubtitleAsset>(dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSMPTESubtitleAsset>(asset, dcp::Fraction(24, 1), 6046, 0);
	auto cpl = write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'Foo'"), path(), 2 },
			{
				dcp::VerificationNote::Type::ERROR,
				dcp::VerificationNote::Code::INVALID_XML,
				string("element 'Foo' is not allowed for content model '(Id,ContentTitleText,AnnotationText?,IssueDate,ReelNumber?,Language?,EditRate,TimeCodeRate,StartTime?,DisplayType?,LoadFont*,SubtitleList)'"),
				path(),
				2
			},
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(dir / "subs.mxf") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() },
		});
}


BOOST_AUTO_TEST_CASE (verify_external_asset)
{
	path const ov_dir("build/test/verify_external_asset");
	prepare_directory (ov_dir);

	auto image = black_image ();
	auto frame = dcp::compress_j2k (image, 100000000, 24, false, false);
	BOOST_REQUIRE (frame.size() < 230000000 / (24 * 8));
	dcp_from_frame (frame, ov_dir);

	dcp::DCP ov (ov_dir);
	ov.read ();

	path const vf_dir("build/test/verify_external_asset_vf");
	prepare_directory (vf_dir);

	auto picture = ov.cpls()[0]->reels()[0]->main_picture();
	auto cpl = write_dcp_with_single_asset (vf_dir, picture);

	check_verify_result (
		{ vf_dir },
		{
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::EXTERNAL_ASSET, picture->asset()->id() },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_cpl_metadata)
{
	path const dir("build/test/verify_valid_cpl_metadata");
	prepare_directory (dir);

	copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTESubtitleAsset>(dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSMPTESubtitleAsset>(asset, dcp::Fraction(24, 1), 16 * 24, 0);

	auto reel = make_shared<dcp::Reel>();
	reel->add (reel_asset);

	reel->add (make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "", 16 * 24), 0));
	reel->add (simple_markers(16 * 24));

	auto cpl = make_shared<dcp::CPL>("hello", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
	cpl->add (reel);
	cpl->set_main_sound_configuration ("L,C,R,Lfe,-,-");
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));
	cpl->set_version_number (1);

	dcp::DCP dcp (dir);
	dcp.add (cpl);
	dcp.write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"hello"
		);
}


path find_cpl (path dir)
{
	for (auto i: directory_iterator(dir)) {
		if (boost::starts_with(i.path().filename().string(), "cpl_")) {
			return i.path();
		}
	}

	BOOST_REQUIRE (false);
	return {};
}


/* DCP with invalid CompositionMetadataAsset */
BOOST_AUTO_TEST_CASE (verify_invalid_cpl_metadata_bad_tag)
{
	using namespace boost::filesystem;

	path const dir("build/test/verify_invalid_cpl_metadata_bad_tag");
	prepare_directory (dir);

	auto reel = make_shared<dcp::Reel>();
	reel->add (black_picture_asset(dir));
	auto cpl = make_shared<dcp::CPL>("hello", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
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
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"hello"
		);

	{
		Editor e (find_cpl(dir));
		e.replace ("MainSound", "MainSoundX");
	}

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'meta:MainSoundXConfiguration'"), canonical(cpl->file().get()), 54 },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'meta:MainSoundXSampleRate'"), canonical(cpl->file().get()), 55 },
			{
				dcp::VerificationNote::Type::ERROR,
				dcp::VerificationNote::Code::INVALID_XML,
				string("element 'meta:MainSoundXConfiguration' is not allowed for content model "
				       "'(Id,AnnotationText?,EditRate,IntrinsicDuration,EntryPoint?,Duration?,"
				       "FullContentTitleText,ReleaseTerritory?,VersionNumber?,Chain?,Distributor?,"
				       "Facility?,AlternateContentVersionList?,Luminance?,MainSoundConfiguration,"
				       "MainSoundSampleRate,MainPictureStoredArea,MainPictureActiveArea,MainSubtitleLanguageList?,"
				       "ExtensionMetadataList?,)'"),
				canonical(cpl->file().get()),
				75
			},
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->id(), canonical(cpl->file().get()) },
		});
}


/* DCP with invalid CompositionMetadataAsset */
BOOST_AUTO_TEST_CASE (verify_invalid_cpl_metadata_missing_tag)
{
	path const dir("build/test/verify_invalid_cpl_metadata_missing_tag");
	prepare_directory (dir);

	auto reel = make_shared<dcp::Reel>();
	reel->add (black_picture_asset(dir));
	auto cpl = make_shared<dcp::CPL>("hello", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
	cpl->add (reel);
	cpl->set_main_sound_configuration ("L,C,R,Lfe,-,-");
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));

	dcp::DCP dcp (dir);
	dcp.add (cpl);
	dcp.write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"hello"
		);

	{
		Editor e (find_cpl(dir));
		e.replace ("meta:Width", "meta:WidthX");
	}

	check_verify_result (
		{ dir },
		{{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::FAILED_READ, string("missing XML tag Width in MainPictureStoredArea") }}
		);
}


BOOST_AUTO_TEST_CASE (verify_invalid_language1)
{
	path const dir("build/test/verify_invalid_language1");
	prepare_directory (dir);
	copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTESubtitleAsset>(dir / "subs.mxf");
	asset->_language = "wrong-andbad";
	asset->write (dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSMPTESubtitleAsset>(asset, dcp::Fraction(24, 1), 6046, 0);
	reel_asset->_language = "badlang";
	auto cpl = write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("badlang") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("wrong-andbad") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() },
		});
}


/* SMPTE DCP with invalid <Language> in the MainClosedCaption reel and also in the XML within the MXF */
BOOST_AUTO_TEST_CASE (verify_invalid_language2)
{
	path const dir("build/test/verify_invalid_language2");
	prepare_directory (dir);
	copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTESubtitleAsset>(dir / "subs.mxf");
	asset->_language = "wrong-andbad";
	asset->write (dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSMPTEClosedCaptionAsset>(asset, dcp::Fraction(24, 1), 6046, 0);
	reel_asset->_language = "badlang";
	auto cpl = write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("badlang") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("wrong-andbad") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


/* SMPTE DCP with invalid <Language> in the MainSound reel, the CPL additional subtitles languages and
 * the release territory.
 */
BOOST_AUTO_TEST_CASE (verify_invalid_language3)
{
	path const dir("build/test/verify_invalid_language3");
	prepare_directory (dir);

	auto picture = simple_picture (dir, "foo");
	auto reel_picture = make_shared<dcp::ReelMonoPictureAsset>(picture, 0);
	auto reel = make_shared<dcp::Reel>();
	reel->add (reel_picture);
	auto sound = simple_sound (dir, "foo", dcp::MXFMetadata(), "frobozz");
	auto reel_sound = make_shared<dcp::ReelSoundAsset>(sound, 0);
	reel->add (reel_sound);
	reel->add (simple_markers());

	auto cpl = make_shared<dcp::CPL>("hello", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
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
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"hello"
		);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("this-is-wrong") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("andso-is-this") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("fred-jim") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("frobozz") },
		});
}


static
vector<dcp::VerificationNote>
check_picture_size (int width, int height, int frame_rate, bool three_d)
{
	using namespace boost::filesystem;

	path dcp_path = "build/test/verify_picture_test";
	prepare_directory (dcp_path);

	shared_ptr<dcp::PictureAsset> mp;
	if (three_d) {
		mp = make_shared<dcp::StereoPictureAsset>(dcp::Fraction(frame_rate, 1), dcp::Standard::SMPTE);
	} else {
		mp = make_shared<dcp::MonoPictureAsset>(dcp::Fraction(frame_rate, 1), dcp::Standard::SMPTE);
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
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
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
	BOOST_CHECK_EQUAL (notes.size(), 0U);
}


static
void
check_picture_size_bad_frame_size (int width, int height, int frame_rate, bool three_d)
{
	auto notes = check_picture_size(width, height, frame_rate, three_d);
	BOOST_REQUIRE_EQUAL (notes.size(), 1U);
	BOOST_CHECK_EQUAL (notes.front().type(), dcp::VerificationNote::Type::BV21_ERROR);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::Code::INVALID_PICTURE_SIZE_IN_PIXELS);
}


static
void
check_picture_size_bad_2k_frame_rate (int width, int height, int frame_rate, bool three_d)
{
	auto notes = check_picture_size(width, height, frame_rate, three_d);
	BOOST_REQUIRE_EQUAL (notes.size(), 2U);
	BOOST_CHECK_EQUAL (notes.back().type(), dcp::VerificationNote::Type::BV21_ERROR);
	BOOST_CHECK_EQUAL (notes.back().code(), dcp::VerificationNote::Code::INVALID_PICTURE_FRAME_RATE_FOR_2K);
}


static
void
check_picture_size_bad_4k_frame_rate (int width, int height, int frame_rate, bool three_d)
{
	auto notes = check_picture_size(width, height, frame_rate, three_d);
	BOOST_REQUIRE_EQUAL (notes.size(), 1U);
	BOOST_CHECK_EQUAL (notes.front().type(), dcp::VerificationNote::Type::BV21_ERROR);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::Code::INVALID_PICTURE_FRAME_RATE_FOR_4K);
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
	check_picture_size_bad_frame_size (4000, 2000, 24, true);

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
	BOOST_CHECK_EQUAL (notes.front().type(), dcp::VerificationNote::Type::BV21_ERROR);
	BOOST_CHECK_EQUAL (notes.front().code(), dcp::VerificationNote::Code::INVALID_PICTURE_ASSET_RESOLUTION_FOR_3D);
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
			dcp::HAlign::CENTER,
			v_position,
			dcp::VAlign::CENTER,
			dcp::Direction::LTR,
			text,
			dcp::Effect::NONE,
			dcp::Colour(),
			dcp::Time(),
			dcp::Time()
		)
	);
}


BOOST_AUTO_TEST_CASE (verify_invalid_closed_caption_xml_size_in_bytes)
{
	path const dir("build/test/verify_invalid_closed_caption_xml_size_in_bytes");
	prepare_directory (dir);

	auto asset = make_shared<dcp::SMPTESubtitleAsset>();
	for (int i = 0; i < 2048; ++i) {
		add_test_subtitle (asset, i * 24, i * 24 + 20);
	}
	asset->set_language (dcp::LanguageTag("de-DE"));
	asset->write (dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSMPTEClosedCaptionAsset>(asset, dcp::Fraction(24, 1), 49148, 0);
	auto cpl = write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(dir / "subs.mxf") },
			{
				dcp::VerificationNote::Type::BV21_ERROR,
				dcp::VerificationNote::Code::INVALID_CLOSED_CAPTION_XML_SIZE_IN_BYTES,
				string("413262"),
				canonical(dir / "subs.mxf")
			},
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() },
		});
}


static
shared_ptr<dcp::SMPTESubtitleAsset>
make_large_subtitle_asset (path font_file)
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
	auto const dir = path("build/test") / name;
	prepare_directory (dir);
	auto asset = make_large_subtitle_asset (dir / "font.ttf");
	add_test_subtitle (asset, 0, 240);
	asset->set_language (dcp::LanguageTag("de-DE"));
	asset->write (dir / "subs.mxf");

	auto reel_asset = make_shared<T>(asset, dcp::Fraction(24, 1), 240, 0);
	auto cpl = write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_TIMED_TEXT_SIZE_IN_BYTES, string("121696411"), canonical(dir / "subs.mxf") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_TIMED_TEXT_FONT_SIZE_IN_BYTES, string("121634816"), canonical(dir / "subs.mxf") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(dir / "subs.mxf") },
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() },
		});
}


BOOST_AUTO_TEST_CASE (verify_subtitle_asset_too_large)
{
	verify_timed_text_asset_too_large<dcp::ReelSMPTESubtitleAsset>("verify_subtitle_asset_too_large");
	verify_timed_text_asset_too_large<dcp::ReelSMPTEClosedCaptionAsset>("verify_closed_caption_asset_too_large");
}


BOOST_AUTO_TEST_CASE (verify_missing_subtitle_language)
{
	path dir = "build/test/verify_missing_subtitle_language";
	prepare_directory (dir);
	auto dcp = make_simple (dir, 1, 106);

	string const xml =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SubtitleReel xmlns=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\" xmlns:xs=\"http://www.w3.org/2001/schema\">"
		"<Id>urn:uuid:e6a8ae03-ebbf-41ed-9def-913a87d1493a</Id>"
		"<ContentTitleText>Content</ContentTitleText>"
		"<AnnotationText>Annotation</AnnotationText>"
		"<IssueDate>2018-10-02T12:25:14+02:00</IssueDate>"
		"<ReelNumber>1</ReelNumber>"
		"<EditRate>24 1</EditRate>"
		"<TimeCodeRate>24</TimeCodeRate>"
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

	auto reel_subs = make_shared<dcp::ReelSMPTESubtitleAsset>(subs, dcp::Fraction(24, 1), 106, 0);
	dcp->cpls()[0]->reels()[0]->add(reel_subs);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_LANGUAGE, canonical(dir / "subs.mxf") },
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME }
		});
}


BOOST_AUTO_TEST_CASE (verify_mismatched_subtitle_languages)
{
	path path ("build/test/verify_mismatched_subtitle_languages");
	auto constexpr reel_length = 192;
	auto dcp = make_simple (path, 2, reel_length);
	auto cpl = dcp->cpls()[0];

	{
		auto subs = make_shared<dcp::SMPTESubtitleAsset>();
		subs->set_language (dcp::LanguageTag("de-DE"));
		subs->add (simple_subtitle());
		subs->write (path / "subs1.mxf");
		auto reel_subs = make_shared<dcp::ReelSMPTESubtitleAsset>(subs, dcp::Fraction(24, 1), reel_length, 0);
		cpl->reels()[0]->add(reel_subs);
	}

	{
		auto subs = make_shared<dcp::SMPTESubtitleAsset>();
		subs->set_language (dcp::LanguageTag("en-US"));
		subs->add (simple_subtitle());
		subs->write (path / "subs2.mxf");
		auto reel_subs = make_shared<dcp::ReelSMPTESubtitleAsset>(subs, dcp::Fraction(24, 1), reel_length, 0);
		cpl->reels()[1]->add(reel_subs);
	}

	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_verify_result (
		{ path },
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(path / "subs1.mxf") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(path / "subs2.mxf") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_SUBTITLE_LANGUAGES }
		});
}


BOOST_AUTO_TEST_CASE (verify_multiple_closed_caption_languages_allowed)
{
	path path ("build/test/verify_multiple_closed_caption_languages_allowed");
	auto constexpr reel_length = 192;
	auto dcp = make_simple (path, 2, reel_length);
	auto cpl = dcp->cpls()[0];

	{
		auto ccaps = make_shared<dcp::SMPTESubtitleAsset>();
		ccaps->set_language (dcp::LanguageTag("de-DE"));
		ccaps->add (simple_subtitle());
		ccaps->write (path / "subs1.mxf");
		auto reel_ccaps = make_shared<dcp::ReelSMPTEClosedCaptionAsset>(ccaps, dcp::Fraction(24, 1), reel_length, 0);
		cpl->reels()[0]->add(reel_ccaps);
	}

	{
		auto ccaps = make_shared<dcp::SMPTESubtitleAsset>();
		ccaps->set_language (dcp::LanguageTag("en-US"));
		ccaps->add (simple_subtitle());
		ccaps->write (path / "subs2.mxf");
		auto reel_ccaps = make_shared<dcp::ReelSMPTEClosedCaptionAsset>(ccaps, dcp::Fraction(24, 1), reel_length, 0);
		cpl->reels()[1]->add(reel_ccaps);
	}

	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_verify_result (
		{ path },
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(path / "subs1.mxf") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(path / "subs2.mxf") }
		});
}


BOOST_AUTO_TEST_CASE (verify_missing_subtitle_start_time)
{
	path dir = "build/test/verify_missing_subtitle_start_time";
	prepare_directory (dir);
	auto dcp = make_simple (dir, 1, 106);

	string const xml =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SubtitleReel xmlns=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\" xmlns:xs=\"http://www.w3.org/2001/schema\">"
		"<Id>urn:uuid:e6a8ae03-ebbf-41ed-9def-913a87d1493a</Id>"
		"<ContentTitleText>Content</ContentTitleText>"
		"<AnnotationText>Annotation</AnnotationText>"
		"<IssueDate>2018-10-02T12:25:14+02:00</IssueDate>"
		"<ReelNumber>1</ReelNumber>"
		"<Language>de-DE</Language>"
		"<EditRate>24 1</EditRate>"
		"<TimeCodeRate>24</TimeCodeRate>"
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

	auto reel_subs = make_shared<dcp::ReelSMPTESubtitleAsset>(subs, dcp::Fraction(24, 1), 106, 0);
	dcp->cpls()[0]->reels()[0]->add(reel_subs);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(dir / "subs.mxf") },
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME }
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_subtitle_start_time)
{
	path dir = "build/test/verify_invalid_subtitle_start_time";
	prepare_directory (dir);
	auto dcp = make_simple (dir, 1, 106);

	string const xml =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SubtitleReel xmlns=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\" xmlns:xs=\"http://www.w3.org/2001/schema\">"
		"<Id>urn:uuid:e6a8ae03-ebbf-41ed-9def-913a87d1493a</Id>"
		"<ContentTitleText>Content</ContentTitleText>"
		"<AnnotationText>Annotation</AnnotationText>"
		"<IssueDate>2018-10-02T12:25:14+02:00</IssueDate>"
		"<ReelNumber>1</ReelNumber>"
		"<Language>de-DE</Language>"
		"<EditRate>24 1</EditRate>"
		"<TimeCodeRate>24</TimeCodeRate>"
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

	auto reel_subs = make_shared<dcp::ReelSMPTESubtitleAsset>(subs, dcp::Fraction(24, 1), 106, 0);
	dcp->cpls().front()->reels().front()->add(reel_subs);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_SUBTITLE_START_TIME, canonical(dir / "subs.mxf") },
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME }
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
shared_ptr<dcp::CPL>
dcp_with_text (path dir, vector<TestText> subs)
{
	prepare_directory (dir);
	auto asset = make_shared<dcp::SMPTESubtitleAsset>();
	asset->set_start_time (dcp::Time());
	for (auto i: subs) {
		add_test_subtitle (asset, i.in, i.out, i.v_position, i.text);
	}
	asset->set_language (dcp::LanguageTag("de-DE"));
	asset->write (dir / "subs.mxf");

	auto reel_asset = make_shared<T>(asset, dcp::Fraction(24, 1), asset->intrinsic_duration(), 0);
	return write_dcp_with_single_asset (dir, reel_asset);
}


BOOST_AUTO_TEST_CASE (verify_invalid_subtitle_first_text_time)
{
	auto const dir = path("build/test/verify_invalid_subtitle_first_text_time");
	/* Just too early */
	auto cpl = dcp_with_text<dcp::ReelSMPTESubtitleAsset> (dir, {{ 4 * 24 - 1, 5 * 24 }});
	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});

}


BOOST_AUTO_TEST_CASE (verify_valid_subtitle_first_text_time)
{
	auto const dir = path("build/test/verify_valid_subtitle_first_text_time");
	/* Just late enough */
	auto cpl = dcp_with_text<dcp::ReelSMPTESubtitleAsset> (dir, {{ 4 * 24, 5 * 24 }});
	check_verify_result ({dir}, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }});
}


BOOST_AUTO_TEST_CASE (verify_valid_subtitle_first_text_time_on_second_reel)
{
	auto const dir = path("build/test/verify_valid_subtitle_first_text_time_on_second_reel");
	prepare_directory (dir);

	auto asset1 = make_shared<dcp::SMPTESubtitleAsset>();
	asset1->set_start_time (dcp::Time());
	/* Just late enough */
	add_test_subtitle (asset1, 4 * 24, 5 * 24);
	asset1->set_language (dcp::LanguageTag("de-DE"));
	asset1->write (dir / "subs1.mxf");
	auto reel_asset1 = make_shared<dcp::ReelSMPTESubtitleAsset>(asset1, dcp::Fraction(24, 1), 5 * 24, 0);
	auto reel1 = make_shared<dcp::Reel>();
	reel1->add (reel_asset1);
	auto markers1 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), 5 * 24, 0);
	markers1->set (dcp::Marker::FFOC, dcp::Time(1, 24, 24));
	reel1->add (markers1);

	auto asset2 = make_shared<dcp::SMPTESubtitleAsset>();
	asset2->set_start_time (dcp::Time());
	/* This would be too early on first reel but should be OK on the second */
	add_test_subtitle (asset2, 3, 4 * 24);
	asset2->set_language (dcp::LanguageTag("de-DE"));
	asset2->write (dir / "subs2.mxf");
	auto reel_asset2 = make_shared<dcp::ReelSMPTESubtitleAsset>(asset2, dcp::Fraction(24, 1), 4 * 24, 0);
	auto reel2 = make_shared<dcp::Reel>();
	reel2->add (reel_asset2);
	auto markers2 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), 4 * 24, 0);
	markers2->set (dcp::Marker::LFOC, dcp::Time(4 * 24 - 1, 24, 24));
	reel2->add (markers2);

	auto cpl = make_shared<dcp::CPL>("hello", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
	cpl->add (reel1);
	cpl->add (reel2);
	auto dcp = make_shared<dcp::DCP>(dir);
	dcp->add (cpl);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"hello"
		);


	check_verify_result ({dir}, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }});
}


BOOST_AUTO_TEST_CASE (verify_invalid_subtitle_spacing)
{
	auto const dir = path("build/test/verify_invalid_subtitle_spacing");
	auto cpl = dcp_with_text<dcp::ReelSMPTESubtitleAsset> (
		dir,
		{
			{ 4 * 24,     5 * 24 },
			{ 5 * 24 + 1, 6 * 24 },
		});
	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_SPACING },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_subtitle_spacing)
{
	auto const dir = path("build/test/verify_valid_subtitle_spacing");
	auto cpl = dcp_with_text<dcp::ReelSMPTESubtitleAsset> (
		dir,
		{
			{ 4 * 24,      5 * 24 },
			{ 5 * 24 + 16, 8 * 24 },
		});
	check_verify_result ({dir}, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }});
}


BOOST_AUTO_TEST_CASE (verify_invalid_subtitle_duration)
{
	auto const dir = path("build/test/verify_invalid_subtitle_duration");
	auto cpl = dcp_with_text<dcp::ReelSMPTESubtitleAsset> (dir, {{ 4 * 24, 4 * 24 + 1 }});
	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_DURATION },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_subtitle_duration)
{
	auto const dir = path("build/test/verify_valid_subtitle_duration");
	auto cpl = dcp_with_text<dcp::ReelSMPTESubtitleAsset> (dir, {{ 4 * 24, 4 * 24 + 17 }});
	check_verify_result ({dir}, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }});
}


BOOST_AUTO_TEST_CASE (verify_subtitle_overlapping_reel_boundary)
{
	auto const dir = path("build/test/verify_subtitle_overlapping_reel_boundary");
	prepare_directory (dir);
	auto asset = make_shared<dcp::SMPTESubtitleAsset>();
	asset->set_start_time (dcp::Time());
	add_test_subtitle (asset, 0, 4 * 24);
	asset->set_language (dcp::LanguageTag("de-DE"));
	asset->write (dir / "subs.mxf");

	auto reel_asset = make_shared<dcp::ReelSMPTESubtitleAsset>(asset, dcp::Fraction(24, 1), 3 * 24, 0);
	auto cpl = write_dcp_with_single_asset (dir, reel_asset);
	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_TIMED_TEXT_DURATION , "72 96", boost::filesystem::canonical(asset->file().get()) },
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::SUBTITLE_OVERLAPS_REEL_BOUNDARY },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});

}


BOOST_AUTO_TEST_CASE (verify_invalid_subtitle_line_count1)
{
	auto const dir = path ("build/test/invalid_subtitle_line_count1");
	auto cpl = dcp_with_text<dcp::ReelSMPTESubtitleAsset> (
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
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_LINE_COUNT },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_subtitle_line_count1)
{
	auto const dir = path ("build/test/verify_valid_subtitle_line_count1");
	auto cpl = dcp_with_text<dcp::ReelSMPTESubtitleAsset> (
		dir,
		{
			{ 96, 200, 0.0, "We" },
			{ 96, 200, 0.1, "have" },
			{ 96, 200, 0.2, "four" },
		});
	check_verify_result ({dir}, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }});
}


BOOST_AUTO_TEST_CASE (verify_invalid_subtitle_line_count2)
{
	auto const dir = path ("build/test/verify_invalid_subtitle_line_count2");
	auto cpl = dcp_with_text<dcp::ReelSMPTESubtitleAsset> (
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
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_LINE_COUNT },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_subtitle_line_count2)
{
	auto const dir = path ("build/test/verify_valid_subtitle_line_count2");
	auto cpl = dcp_with_text<dcp::ReelSMPTESubtitleAsset> (
		dir,
		{
			{ 96, 300, 0.0, "We" },
			{ 96, 300, 0.1, "have" },
			{ 150, 180, 0.2, "four" },
			{ 190, 250, 0.3, "lines" }
		});
	check_verify_result ({dir}, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }});
}


BOOST_AUTO_TEST_CASE (verify_invalid_subtitle_line_length1)
{
	auto const dir = path ("build/test/verify_invalid_subtitle_line_length1");
	auto cpl = dcp_with_text<dcp::ReelSMPTESubtitleAsset> (
		dir,
		{
			{ 96, 300, 0.0, "012345678901234567890123456789012345678901234567890123" }
		});
	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::NEARLY_INVALID_SUBTITLE_LINE_LENGTH },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_subtitle_line_length2)
{
	auto const dir = path ("build/test/verify_invalid_subtitle_line_length2");
	auto cpl = dcp_with_text<dcp::ReelSMPTESubtitleAsset> (
		dir,
		{
			{ 96, 300, 0.0, "012345678901234567890123456789012345678901234567890123456789012345678901234567890" }
		});
	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_LINE_LENGTH },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_closed_caption_line_count1)
{
	auto const dir = path ("build/test/verify_valid_closed_caption_line_count1");
	auto cpl = dcp_with_text<dcp::ReelSMPTEClosedCaptionAsset> (
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
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_CLOSED_CAPTION_LINE_COUNT},
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_closed_caption_line_count2)
{
	auto const dir = path ("build/test/verify_valid_closed_caption_line_count2");
	auto cpl = dcp_with_text<dcp::ReelSMPTEClosedCaptionAsset> (
		dir,
		{
			{ 96, 200, 0.0, "We" },
			{ 96, 200, 0.1, "have" },
			{ 96, 200, 0.2, "four" },
		});
	check_verify_result ({dir}, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }});
}


BOOST_AUTO_TEST_CASE (verify_invalid_closed_caption_line_count3)
{
	auto const dir = path ("build/test/verify_invalid_closed_caption_line_count3");
	auto cpl = dcp_with_text<dcp::ReelSMPTEClosedCaptionAsset> (
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
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_CLOSED_CAPTION_LINE_COUNT},
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_closed_caption_line_count4)
{
	auto const dir = path ("build/test/verify_valid_closed_caption_line_count4");
	auto cpl = dcp_with_text<dcp::ReelSMPTEClosedCaptionAsset> (
		dir,
		{
			{ 96, 300, 0.0, "We" },
			{ 96, 300, 0.1, "have" },
			{ 150, 180, 0.2, "four" },
			{ 190, 250, 0.3, "lines" }
		});
	check_verify_result ({dir}, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }});
}


BOOST_AUTO_TEST_CASE (verify_invalid_closed_caption_line_length)
{
	auto const dir = path ("build/test/verify_invalid_closed_caption_line_length");
	auto cpl = dcp_with_text<dcp::ReelSMPTEClosedCaptionAsset> (
		dir,
		{
			{ 96, 300, 0.0, "0123456789012345678901234567890123" }
		});
	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_CLOSED_CAPTION_LINE_LENGTH },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_sound_frame_rate)
{
	path const dir("build/test/verify_invalid_sound_frame_rate");
	prepare_directory (dir);

	auto picture = simple_picture (dir, "foo");
	auto reel_picture = make_shared<dcp::ReelMonoPictureAsset>(picture, 0);
	auto reel = make_shared<dcp::Reel>();
	reel->add (reel_picture);
	auto sound = simple_sound (dir, "foo", dcp::MXFMetadata(), "de-DE", 24, 96000);
	auto reel_sound = make_shared<dcp::ReelSoundAsset>(sound, 0);
	reel->add (reel_sound);
	reel->add (simple_markers());
	auto cpl = make_shared<dcp::CPL>("hello", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
	cpl->add (reel);
	auto dcp = make_shared<dcp::DCP>(dir);
	dcp->add (cpl);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"hello"
		);

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_SOUND_FRAME_RATE, string("96000"), canonical(dir / "audiofoo.mxf") },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() },
		});
}


BOOST_AUTO_TEST_CASE (verify_missing_cpl_annotation_text)
{
	path const dir("build/test/verify_missing_cpl_annotation_text");
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	BOOST_REQUIRE_EQUAL (dcp->cpls().size(), 1U);

	auto const cpl = dcp->cpls()[0];

	{
		BOOST_REQUIRE (cpl->file());
		Editor e(cpl->file().get());
		e.replace("<AnnotationText>A Test DCP</AnnotationText>", "");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_ANNOTATION_TEXT, cpl->id(), canonical(cpl->file().get()) },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->id(), canonical(cpl->file().get()) }
		});
}


BOOST_AUTO_TEST_CASE (verify_mismatched_cpl_annotation_text)
{
	path const dir("build/test/verify_mismatched_cpl_annotation_text");
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	BOOST_REQUIRE_EQUAL (dcp->cpls().size(), 1U);
	auto const cpl = dcp->cpls()[0];

	{
		BOOST_REQUIRE (cpl->file());
		Editor e(cpl->file().get());
		e.replace("<AnnotationText>A Test DCP</AnnotationText>", "<AnnotationText>A Test DCP 1</AnnotationText>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISMATCHED_CPL_ANNOTATION_TEXT, cpl->id(), canonical(cpl->file().get()) },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->id(), canonical(cpl->file().get()) }
		});
}


BOOST_AUTO_TEST_CASE (verify_mismatched_asset_duration)
{
	path const dir("build/test/verify_mismatched_asset_duration");
	prepare_directory (dir);
	shared_ptr<dcp::DCP> dcp (new dcp::DCP(dir));
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);

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
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_ASSET_DURATION },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), canonical(cpl->file().get()) }
		});
}



static
shared_ptr<dcp::CPL>
verify_subtitles_must_be_in_all_reels_check (path dir, bool add_to_reel1, bool add_to_reel2)
{
	prepare_directory (dir);
	auto dcp = make_shared<dcp::DCP>(dir);
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);

	auto constexpr reel_length = 192;

	auto subs = make_shared<dcp::SMPTESubtitleAsset>();
	subs->set_language (dcp::LanguageTag("de-DE"));
	subs->set_start_time (dcp::Time());
	subs->add (simple_subtitle());
	subs->write (dir / "subs.mxf");
	auto reel_subs = make_shared<dcp::ReelSMPTESubtitleAsset>(subs, dcp::Fraction(24, 1), reel_length, 0);

	auto reel1 = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "", reel_length), 0),
		make_shared<dcp::ReelSoundAsset>(simple_sound(dir, "", dcp::MXFMetadata(), "en-US", reel_length), 0)
		);

	if (add_to_reel1) {
		reel1->add (make_shared<dcp::ReelSMPTESubtitleAsset>(subs, dcp::Fraction(24, 1), reel_length, 0));
	}

	auto markers1 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), reel_length, 0);
	markers1->set (dcp::Marker::FFOC, dcp::Time(1, 24, 24));
	reel1->add (markers1);

	cpl->add (reel1);

	auto reel2 = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "", reel_length), 0),
		make_shared<dcp::ReelSoundAsset>(simple_sound(dir, "", dcp::MXFMetadata(), "en-US", reel_length), 0)
		);

	if (add_to_reel2) {
		reel2->add (make_shared<dcp::ReelSMPTESubtitleAsset>(subs, dcp::Fraction(24, 1), reel_length, 0));
	}

	auto markers2 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), reel_length, 0);
	markers2->set (dcp::Marker::LFOC, dcp::Time(reel_length - 1, 24, 24));
	reel2->add (markers2);

	cpl->add (reel2);

	dcp->add (cpl);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	return cpl;
}


BOOST_AUTO_TEST_CASE (verify_missing_main_subtitle_from_some_reels)
{
	{
		path dir ("build/test/missing_main_subtitle_from_some_reels");
		auto cpl = verify_subtitles_must_be_in_all_reels_check (dir, true, false);
		check_verify_result (
			{ dir },
			{
				{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_MAIN_SUBTITLE_FROM_SOME_REELS },
				{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
			});

	}

	{
		path dir ("build/test/verify_subtitles_must_be_in_all_reels2");
		auto cpl = verify_subtitles_must_be_in_all_reels_check (dir, true, true);
		check_verify_result ({dir}, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }});
	}

	{
		path dir ("build/test/verify_subtitles_must_be_in_all_reels1");
		auto cpl = verify_subtitles_must_be_in_all_reels_check (dir, false, false);
		check_verify_result ({dir}, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }});
	}
}


static
shared_ptr<dcp::CPL>
verify_closed_captions_must_be_in_all_reels_check (path dir, int caps_in_reel1, int caps_in_reel2)
{
	prepare_directory (dir);
	auto dcp = make_shared<dcp::DCP>(dir);
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);

	auto constexpr reel_length = 192;

	auto subs = make_shared<dcp::SMPTESubtitleAsset>();
	subs->set_language (dcp::LanguageTag("de-DE"));
	subs->set_start_time (dcp::Time());
	subs->add (simple_subtitle());
	subs->write (dir / "subs.mxf");

	auto reel1 = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "", reel_length), 0),
		make_shared<dcp::ReelSoundAsset>(simple_sound(dir, "", dcp::MXFMetadata(), "en-US", reel_length), 0)
		);

	for (int i = 0; i < caps_in_reel1; ++i) {
		reel1->add (make_shared<dcp::ReelSMPTEClosedCaptionAsset>(subs, dcp::Fraction(24, 1), reel_length, 0));
	}

	auto markers1 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), reel_length, 0);
	markers1->set (dcp::Marker::FFOC, dcp::Time(1, 24, 24));
	reel1->add (markers1);

	cpl->add (reel1);

	auto reel2 = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "", reel_length), 0),
		make_shared<dcp::ReelSoundAsset>(simple_sound(dir, "", dcp::MXFMetadata(), "en-US", reel_length), 0)
		);

	for (int i = 0; i < caps_in_reel2; ++i) {
		reel2->add (make_shared<dcp::ReelSMPTEClosedCaptionAsset>(subs, dcp::Fraction(24, 1), reel_length, 0));
	}

	auto markers2 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), reel_length, 0);
	markers2->set (dcp::Marker::LFOC, dcp::Time(reel_length - 1, 24, 24));
	reel2->add (markers2);

	cpl->add (reel2);

	dcp->add (cpl);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	return cpl;
}


BOOST_AUTO_TEST_CASE (verify_mismatched_closed_caption_asset_counts)
{
	{
		path dir ("build/test/mismatched_closed_caption_asset_counts");
		auto cpl = verify_closed_captions_must_be_in_all_reels_check (dir, 3, 4);
		check_verify_result (
			{dir},
			{
				{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_CLOSED_CAPTION_ASSET_COUNTS },
				{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
			});
	}

	{
		path dir ("build/test/verify_closed_captions_must_be_in_all_reels2");
		auto cpl = verify_closed_captions_must_be_in_all_reels_check (dir, 4, 4);
		check_verify_result ({dir}, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }});
	}

	{
		path dir ("build/test/verify_closed_captions_must_be_in_all_reels3");
		auto cpl = verify_closed_captions_must_be_in_all_reels_check (dir, 0, 0);
		check_verify_result ({dir}, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }});
	}
}


template <class T>
void
verify_text_entry_point_check (path dir, dcp::VerificationNote::Code code, boost::function<void (shared_ptr<T>)> adjust)
{
	prepare_directory (dir);
	auto dcp = make_shared<dcp::DCP>(dir);
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);

	auto constexpr reel_length = 192;

	auto subs = make_shared<dcp::SMPTESubtitleAsset>();
	subs->set_language (dcp::LanguageTag("de-DE"));
	subs->set_start_time (dcp::Time());
	subs->add (simple_subtitle());
	subs->write (dir / "subs.mxf");
	auto reel_text = make_shared<T>(subs, dcp::Fraction(24, 1), reel_length, 0);
	adjust (reel_text);

	auto reel = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "", reel_length), 0),
		make_shared<dcp::ReelSoundAsset>(simple_sound(dir, "", dcp::MXFMetadata(), "en-US", reel_length), 0)
		);

	reel->add (reel_text);

	reel->add (simple_markers(reel_length));

	cpl->add (reel);

	dcp->add (cpl);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, code, subs->id() },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


BOOST_AUTO_TEST_CASE (verify_text_entry_point)
{
	verify_text_entry_point_check<dcp::ReelSMPTESubtitleAsset> (
		"build/test/verify_subtitle_entry_point_must_be_present",
		dcp::VerificationNote::Code::MISSING_SUBTITLE_ENTRY_POINT,
		[](shared_ptr<dcp::ReelSMPTESubtitleAsset> asset) {
			asset->unset_entry_point ();
			}
		);

	verify_text_entry_point_check<dcp::ReelSMPTESubtitleAsset> (
		"build/test/verify_subtitle_entry_point_must_be_zero",
		dcp::VerificationNote::Code::INCORRECT_SUBTITLE_ENTRY_POINT,
		[](shared_ptr<dcp::ReelSMPTESubtitleAsset> asset) {
			asset->set_entry_point (4);
			}
		);

	verify_text_entry_point_check<dcp::ReelSMPTEClosedCaptionAsset> (
		"build/test/verify_closed_caption_entry_point_must_be_present",
		dcp::VerificationNote::Code::MISSING_CLOSED_CAPTION_ENTRY_POINT,
		[](shared_ptr<dcp::ReelSMPTEClosedCaptionAsset> asset) {
			asset->unset_entry_point ();
			}
		);

	verify_text_entry_point_check<dcp::ReelSMPTEClosedCaptionAsset> (
		"build/test/verify_closed_caption_entry_point_must_be_zero",
		dcp::VerificationNote::Code::INCORRECT_CLOSED_CAPTION_ENTRY_POINT,
		[](shared_ptr<dcp::ReelSMPTEClosedCaptionAsset> asset) {
			asset->set_entry_point (9);
			}
		);
}


BOOST_AUTO_TEST_CASE (verify_missing_hash)
{
	RNGFixer fix;

	path const dir("build/test/verify_missing_hash");
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	BOOST_REQUIRE_EQUAL (dcp->cpls().size(), 1U);
	auto const cpl = dcp->cpls()[0];
	BOOST_REQUIRE_EQUAL (cpl->reels().size(), 1U);
	BOOST_REQUIRE (cpl->reels()[0]->main_picture());
	auto asset_id = cpl->reels()[0]->main_picture()->id();

	{
		BOOST_REQUIRE (cpl->file());
		Editor e(cpl->file().get());
		e.delete_first_line_containing("<Hash>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->id(), cpl->file().get() },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_HASH, asset_id }
		});
}


static
void
verify_markers_test (
	path dir,
	vector<pair<dcp::Marker, dcp::Time>> markers,
	vector<dcp::VerificationNote> test_notes
	)
{
	auto dcp = make_simple (dir);
	dcp->cpls()[0]->set_content_kind (dcp::ContentKind::FEATURE);
	auto markers_asset = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), 24, 0);
	for (auto const& i: markers) {
		markers_asset->set (i.first, i.second);
	}
	dcp->cpls()[0]->reels()[0]->add(markers_asset);
	dcp->write_xml (
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
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_FFEC_IN_FEATURE }
		});

	verify_markers_test (
		"build/test/verify_markers_missing_ffmc",
		{
			{ dcp::Marker::FFEC, dcp::Time(12, 24, 24) },
			{ dcp::Marker::FFOC, dcp::Time(1, 24, 24) },
			{ dcp::Marker::LFOC, dcp::Time(23, 24, 24) }
		},
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_FFMC_IN_FEATURE }
		});

	verify_markers_test (
		"build/test/verify_markers_missing_ffoc",
		{
			{ dcp::Marker::FFEC, dcp::Time(12, 24, 24) },
			{ dcp::Marker::FFMC, dcp::Time(13, 24, 24) },
			{ dcp::Marker::LFOC, dcp::Time(23, 24, 24) }
		},
		{
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSING_FFOC}
		});

	verify_markers_test (
		"build/test/verify_markers_missing_lfoc",
		{
			{ dcp::Marker::FFEC, dcp::Time(12, 24, 24) },
			{ dcp::Marker::FFMC, dcp::Time(13, 24, 24) },
			{ dcp::Marker::FFOC, dcp::Time(1, 24, 24) }
		},
		{
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSING_LFOC }
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
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INCORRECT_FFOC, string("3") }
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
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INCORRECT_LFOC, string("18") }
		});
}


BOOST_AUTO_TEST_CASE (verify_missing_cpl_metadata_version_number)
{
	path dir = "build/test/verify_missing_cpl_metadata_version_number";
	prepare_directory (dir);
	auto dcp = make_simple (dir);
	auto cpl = dcp->cpls()[0];
	cpl->unset_version_number();
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_verify_result ({dir}, {{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA_VERSION_NUMBER, cpl->id(), cpl->file().get() }});
}


BOOST_AUTO_TEST_CASE (verify_missing_extension_metadata1)
{
	path dir = "build/test/verify_missing_extension_metadata1";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	auto cpl = dcp->cpls()[0];

	{
		Editor e (cpl->file().get());
		e.delete_lines ("<meta:ExtensionMetadataList>", "</meta:ExtensionMetadataList>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->id(), cpl->file().get() },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_EXTENSION_METADATA, cpl->id(), cpl->file().get() }
		});
}


BOOST_AUTO_TEST_CASE (verify_missing_extension_metadata2)
{
	path dir = "build/test/verify_missing_extension_metadata2";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	auto cpl = dcp->cpls()[0];

	{
		Editor e (cpl->file().get());
		e.delete_lines ("<meta:ExtensionMetadata scope=\"http://isdcf.com/ns/cplmd/app\">", "</meta:ExtensionMetadata>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->id(), cpl->file().get() },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_EXTENSION_METADATA, cpl->id(), cpl->file().get() }
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_cpl_extension_metadata3)
{
	path dir = "build/test/verify_invalid_xml_cpl_extension_metadata3";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	auto const cpl = dcp->cpls()[0];

	{
		Editor e (cpl->file().get());
		e.replace ("<meta:Name>A", "<meta:NameX>A");
		e.replace ("n</meta:Name>", "n</meta:NameX>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'meta:NameX'"), cpl->file().get(), 75 },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("element 'meta:NameX' is not allowed for content model '(Name,PropertyList?,)'"), cpl->file().get(), 82 },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->id(), cpl->file().get() },
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_extension_metadata1)
{
	path dir = "build/test/verify_invalid_extension_metadata1";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	auto cpl = dcp->cpls()[0];

	{
		Editor e (cpl->file().get());
		e.replace ("Application", "Fred");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->id(), cpl->file().get() },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_EXTENSION_METADATA, string("<Name> should be 'Application'"), cpl->file().get() },
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_extension_metadata2)
{
	path dir = "build/test/verify_invalid_extension_metadata2";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	auto cpl = dcp->cpls()[0];

	{
		Editor e (cpl->file().get());
		e.replace ("DCP Constraints Profile", "Fred");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->id(), cpl->file().get() },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_EXTENSION_METADATA, string("<Name> property should be 'DCP Constraints Profile'"), cpl->file().get() },
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_cpl_extension_metadata6)
{
	path dir = "build/test/verify_invalid_xml_cpl_extension_metadata6";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	auto const cpl = dcp->cpls()[0];

	{
		Editor e (cpl->file().get());
		e.replace ("<meta:Value>", "<meta:ValueX>");
		e.replace ("</meta:Value>", "</meta:ValueX>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'meta:ValueX'"), cpl->file().get(), 79 },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("element 'meta:ValueX' is not allowed for content model '(Name,Value)'"), cpl->file().get(), 80 },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->id(), cpl->file().get() },
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_cpl_extension_metadata7)
{
	path dir = "build/test/verify_invalid_xml_cpl_extension_metadata7";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	auto const cpl = dcp->cpls()[0];

	{
		Editor e (cpl->file().get());
		e.replace ("SMPTE-RDD-52:2020-Bv2.1", "Fred");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->id(), cpl->file().get() },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_EXTENSION_METADATA, string("<Value> property should be 'SMPTE-RDD-52:2020-Bv2.1'"), cpl->file().get() },
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_cpl_extension_metadata8)
{
	path dir = "build/test/verify_invalid_xml_cpl_extension_metadata8";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	auto const cpl = dcp->cpls()[0];

	{
		Editor e (cpl->file().get());
		e.replace ("<meta:Property>", "<meta:PropertyX>");
		e.replace ("</meta:Property>", "</meta:PropertyX>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'meta:PropertyX'"), cpl->file().get(), 77 },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("element 'meta:PropertyX' is not allowed for content model '(Property+)'"), cpl->file().get(), 81 },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->id(), cpl->file().get() },
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_cpl_extension_metadata9)
{
	path dir = "build/test/verify_invalid_xml_cpl_extension_metadata9";
	auto dcp = make_simple (dir);
	dcp->write_xml (
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	auto const cpl = dcp->cpls()[0];

	{
		Editor e (cpl->file().get());
		e.replace ("<meta:PropertyList>", "<meta:PropertyListX>");
		e.replace ("</meta:PropertyList>", "</meta:PropertyListX>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'meta:PropertyListX'"), cpl->file().get(), 76 },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("element 'meta:PropertyListX' is not allowed for content model '(Name,PropertyList?,)'"), cpl->file().get(), 82 },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->id(), cpl->file().get() },
		});
}



BOOST_AUTO_TEST_CASE (verify_unsigned_cpl_with_encrypted_content)
{
	path dir = "build/test/verify_unsigned_cpl_with_encrypted_content";
	prepare_directory (dir);
	for (auto i: directory_iterator("test/ref/DCP/encryption_test")) {
		copy_file (i.path(), dir / i.path().filename());
	}

	path const pkl = dir / ( "pkl_" + encryption_test_pkl_id + ".xml" );
	path const cpl = dir / ( "cpl_" + encryption_test_cpl_id + ".xml");

	{
		Editor e (cpl);
		e.delete_lines ("<dsig:Signature", "</dsig:Signature>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, encryption_test_cpl_id, canonical(cpl) },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_PKL_ANNOTATION_TEXT_WITH_CPL, encryption_test_pkl_id, canonical(pkl), },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_FFEC_IN_FEATURE },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_FFMC_IN_FEATURE },
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSING_FFOC },
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSING_LFOC },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, encryption_test_cpl_id, canonical(cpl) },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::UNSIGNED_CPL_WITH_ENCRYPTED_CONTENT, encryption_test_cpl_id, canonical(cpl) }
		});
}


BOOST_AUTO_TEST_CASE (verify_unsigned_pkl_with_encrypted_content)
{
	path dir = "build/test/unsigned_pkl_with_encrypted_content";
	prepare_directory (dir);
	for (auto i: directory_iterator("test/ref/DCP/encryption_test")) {
		copy_file (i.path(), dir / i.path().filename());
	}

	path const cpl = dir / ("cpl_" + encryption_test_cpl_id + ".xml");
	path const pkl = dir / ("pkl_" + encryption_test_pkl_id + ".xml");
	{
		Editor e (pkl);
		e.delete_lines ("<dsig:Signature", "</dsig:Signature>");
	}

	check_verify_result (
		{dir},
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_PKL_ANNOTATION_TEXT_WITH_CPL, encryption_test_pkl_id, canonical(pkl) },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_FFEC_IN_FEATURE },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_FFMC_IN_FEATURE },
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSING_FFOC },
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSING_LFOC },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, encryption_test_cpl_id, canonical(cpl) },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::UNSIGNED_PKL_WITH_ENCRYPTED_CONTENT, encryption_test_pkl_id, canonical(pkl) },
		});
}


BOOST_AUTO_TEST_CASE (verify_unsigned_pkl_with_unencrypted_content)
{
	path dir = "build/test/verify_unsigned_pkl_with_unencrypted_content";
	prepare_directory (dir);
	for (auto i: directory_iterator("test/ref/DCP/dcp_test1")) {
		copy_file (i.path(), dir / i.path().filename());
	}

	{
		Editor e (dir / dcp_test1_pkl);
		e.delete_lines ("<dsig:Signature", "</dsig:Signature>");
	}

	check_verify_result ({dir}, {});
}


BOOST_AUTO_TEST_CASE (verify_partially_encrypted)
{
	path dir ("build/test/verify_must_not_be_partially_encrypted");
	prepare_directory (dir);

	dcp::DCP d (dir);

	auto signer = make_shared<dcp::CertificateChain>();
	signer->add (dcp::Certificate(dcp::file_to_string("test/ref/crypt/ca.self-signed.pem")));
	signer->add (dcp::Certificate(dcp::file_to_string("test/ref/crypt/intermediate.signed.pem")));
	signer->add (dcp::Certificate(dcp::file_to_string("test/ref/crypt/leaf.signed.pem")));
	signer->set_key (dcp::file_to_string("test/ref/crypt/leaf.key"));

	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);

	dcp::Key key;

	auto mp = make_shared<dcp::MonoPictureAsset>(dcp::Fraction (24, 1), dcp::Standard::SMPTE);
	mp->set_key (key);

	auto writer = mp->start_write (dir / "video.mxf", false);
	dcp::ArrayData j2c ("test/data/flat_red.j2c");
	for (int i = 0; i < 24; ++i) {
		writer->write (j2c.data(), j2c.size());
	}
	writer->finalize ();

	auto ms = simple_sound (dir, "", dcp::MXFMetadata(), "de-DE");

	auto reel = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(mp, 0),
		make_shared<dcp::ReelSoundAsset>(ms, 0)
		);

	reel->add (simple_markers());

	cpl->add (reel);

	cpl->set_content_version (
		{"urn:uri:81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00", "81fb54df-e1bf-4647-8788-ea7ba154375b_2012-07-17T04:45:18+00:00"}
		);
	cpl->set_annotation_text ("A Test DCP");
	cpl->set_issuer ("OpenDCP 0.0.25");
	cpl->set_creator ("OpenDCP 0.0.25");
	cpl->set_issue_date ("2012-07-17T04:45:18+00:00");
	cpl->set_main_sound_configuration ("L,C,R,Lfe,-,-");
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));
	cpl->set_version_number (1);

	d.add (cpl);

	d.write_xml ("OpenDCP 0.0.25", "OpenDCP 0.0.25", "2012-07-17T04:45:18+00:00", "A Test DCP", signer);

	check_verify_result (
		{dir},
		{
			{dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::PARTIALLY_ENCRYPTED},
		});
}


BOOST_AUTO_TEST_CASE (verify_jpeg2000_codestream_2k)
{
	vector<dcp::VerificationNote> notes;
	dcp::MonoPictureAsset picture (find_file(private_test / "data" / "JourneyToJah_TLR-1_F_EN-DE-FR_CH_51_2K_LOK_20140225_DGL_SMPTE_OV", "j2c.mxf"));
	auto reader = picture.start_read ();
	auto frame = reader->get_frame (0);
	verify_j2k (frame, notes);
	BOOST_REQUIRE_EQUAL (notes.size(), 0U);
}


BOOST_AUTO_TEST_CASE (verify_jpeg2000_codestream_4k)
{
	vector<dcp::VerificationNote> notes;
	dcp::MonoPictureAsset picture (find_file(private_test / "data" / "sul", "TLR"));
	auto reader = picture.start_read ();
	auto frame = reader->get_frame (0);
	verify_j2k (frame, notes);
	BOOST_REQUIRE_EQUAL (notes.size(), 0U);
}


BOOST_AUTO_TEST_CASE (verify_jpeg2000_codestream_libdcp)
{
	boost::filesystem::path dir = "build/test/verify_jpeg2000_codestream_libdcp";
	prepare_directory (dir);
	auto dcp = make_simple (dir);
	dcp->write_xml ();
	vector<dcp::VerificationNote> notes;
	dcp::MonoPictureAsset picture (find_file(dir, "video"));
	auto reader = picture.start_read ();
	auto frame = reader->get_frame (0);
	verify_j2k (frame, notes);
	BOOST_REQUIRE_EQUAL (notes.size(), 0U);
}


/** Check that ResourceID and the XML ID being different is spotted */
BOOST_AUTO_TEST_CASE (verify_mismatched_subtitle_resource_id)
{
	boost::filesystem::path const dir = "build/test/verify_mismatched_subtitle_resource_id";
	prepare_directory (dir);

	ASDCP::WriterInfo writer_info;
	writer_info.LabelSetType = ASDCP::LS_MXF_SMPTE;

	unsigned int c;
	auto mxf_id = dcp::make_uuid ();
	Kumu::hex2bin (mxf_id.c_str(), writer_info.AssetUUID, Kumu::UUID_Length, &c);
	BOOST_REQUIRE (c == Kumu::UUID_Length);

	auto resource_id = dcp::make_uuid ();
	ASDCP::TimedText::TimedTextDescriptor descriptor;
	Kumu::hex2bin (resource_id.c_str(), descriptor.AssetID, Kumu::UUID_Length, &c);
	DCP_ASSERT (c == Kumu::UUID_Length);

	auto xml_id = dcp::make_uuid ();
	ASDCP::TimedText::MXFWriter writer;
	auto subs_mxf = dir / "subs.mxf";
	auto r = writer.OpenWrite(subs_mxf.c_str(), writer_info, descriptor, 4096);
	BOOST_REQUIRE (ASDCP_SUCCESS(r));
	writer.WriteTimedTextResource (dcp::String::compose(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SubtitleReel xmlns=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\" xmlns:xs=\"http://www.w3.org/2001/schema\">"
		"<Id>urn:uuid:%1</Id>"
		"<ContentTitleText>Content</ContentTitleText>"
		"<AnnotationText>Annotation</AnnotationText>"
		"<IssueDate>2018-10-02T12:25:14+02:00</IssueDate>"
		"<ReelNumber>1</ReelNumber>"
		"<Language>en-US</Language>"
		"<EditRate>25 1</EditRate>"
		"<TimeCodeRate>25</TimeCodeRate>"
		"<StartTime>00:00:00:00</StartTime>"
		"<SubtitleList>"
		"<Font ID=\"arial\" Color=\"FFFEFEFE\" Weight=\"normal\" Size=\"42\" Effect=\"border\" EffectColor=\"FF181818\" AspectAdjust=\"1.00\">"
		"<Subtitle SpotNumber=\"1\" TimeIn=\"00:00:03:00\" TimeOut=\"00:00:04:10\" FadeUpTime=\"00:00:00:00\" FadeDownTime=\"00:00:00:00\">"
		"<Text Hposition=\"0.0\" Halign=\"center\" Valign=\"bottom\" Vposition=\"13.5\" Direction=\"ltr\">Hello world</Text>"
		"</Subtitle>"
		"</Font>"
		"</SubtitleList>"
		"</SubtitleReel>",
		xml_id).c_str());

	writer.Finalize();

	auto subs_asset = make_shared<dcp::SMPTESubtitleAsset>(subs_mxf);
	auto subs_reel = make_shared<dcp::ReelSMPTESubtitleAsset>(subs_asset, dcp::Fraction(24, 1), 240, 0);

	auto cpl = write_dcp_with_single_asset (dir, subs_reel);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_TIMED_TEXT_DURATION , "240 0", boost::filesystem::canonical(subs_mxf) },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_TIMED_TEXT_RESOURCE_ID },
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


/** Check that ResourceID and the MXF ID being the same is spotted */
BOOST_AUTO_TEST_CASE (verify_incorrect_timed_text_id)
{
	boost::filesystem::path const dir = "build/test/verify_incorrect_timed_text_id";
	prepare_directory (dir);

	ASDCP::WriterInfo writer_info;
	writer_info.LabelSetType = ASDCP::LS_MXF_SMPTE;

	unsigned int c;
	auto mxf_id = dcp::make_uuid ();
	Kumu::hex2bin (mxf_id.c_str(), writer_info.AssetUUID, Kumu::UUID_Length, &c);
	BOOST_REQUIRE (c == Kumu::UUID_Length);

	auto resource_id = mxf_id;
	ASDCP::TimedText::TimedTextDescriptor descriptor;
	Kumu::hex2bin (resource_id.c_str(), descriptor.AssetID, Kumu::UUID_Length, &c);
	DCP_ASSERT (c == Kumu::UUID_Length);

	auto xml_id = resource_id;
	ASDCP::TimedText::MXFWriter writer;
	auto subs_mxf = dir / "subs.mxf";
	auto r = writer.OpenWrite(subs_mxf.c_str(), writer_info, descriptor, 4096);
	BOOST_REQUIRE (ASDCP_SUCCESS(r));
	writer.WriteTimedTextResource (dcp::String::compose(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SubtitleReel xmlns=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\" xmlns:xs=\"http://www.w3.org/2001/schema\">"
		"<Id>urn:uuid:%1</Id>"
		"<ContentTitleText>Content</ContentTitleText>"
		"<AnnotationText>Annotation</AnnotationText>"
		"<IssueDate>2018-10-02T12:25:14+02:00</IssueDate>"
		"<ReelNumber>1</ReelNumber>"
		"<Language>en-US</Language>"
		"<EditRate>25 1</EditRate>"
		"<TimeCodeRate>25</TimeCodeRate>"
		"<StartTime>00:00:00:00</StartTime>"
		"<SubtitleList>"
		"<Font ID=\"arial\" Color=\"FFFEFEFE\" Weight=\"normal\" Size=\"42\" Effect=\"border\" EffectColor=\"FF181818\" AspectAdjust=\"1.00\">"
		"<Subtitle SpotNumber=\"1\" TimeIn=\"00:00:03:00\" TimeOut=\"00:00:04:10\" FadeUpTime=\"00:00:00:00\" FadeDownTime=\"00:00:00:00\">"
		"<Text Hposition=\"0.0\" Halign=\"center\" Valign=\"bottom\" Vposition=\"13.5\" Direction=\"ltr\">Hello world</Text>"
		"</Subtitle>"
		"</Font>"
		"</SubtitleList>"
		"</SubtitleReel>",
		xml_id).c_str());

	writer.Finalize();

	auto subs_asset = make_shared<dcp::SMPTESubtitleAsset>(subs_mxf);
	auto subs_reel = make_shared<dcp::ReelSMPTESubtitleAsset>(subs_asset, dcp::Fraction(24, 1), 240, 0);

	auto cpl = write_dcp_with_single_asset (dir, subs_reel);

	check_verify_result (
		{ dir },
		{
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_TIMED_TEXT_DURATION , "240 0", boost::filesystem::canonical(subs_mxf) },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INCORRECT_TIMED_TEXT_ASSET_ID },
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME },
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->id(), cpl->file().get() }
		});
}


/** Check a DCP with a 3D asset marked as 2D */
BOOST_AUTO_TEST_CASE (verify_threed_marked_as_twod)
{
	check_verify_result (
		{ private_test / "data" / "xm" },
		{
			{
				dcp::VerificationNote::Type::WARNING,
				dcp::VerificationNote::Code::THREED_ASSET_MARKED_AS_TWOD, boost::filesystem::canonical(find_file(private_test / "data" / "xm", "j2c"))
			},
			{
				dcp::VerificationNote::Type::BV21_ERROR,
				dcp::VerificationNote::Code::INVALID_STANDARD
			},
		});

}

