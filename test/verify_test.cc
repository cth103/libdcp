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
#include "file.h"
#include "interop_text_asset.h"
#include "j2k_transcode.h"
#include "mono_j2k_picture_asset.h"
#include "mono_j2k_picture_asset_writer.h"
#include "openjpeg_image.h"
#include "raw_convert.h"
#include "reel.h"
#include "reel_interop_text_asset.h"
#include "reel_markers_asset.h"
#include "reel_mono_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_stereo_picture_asset.h"
#include "reel_smpte_text_asset.h"
#include "smpte_text_asset.h"
#include "stereo_j2k_picture_asset.h"
#include "stream_operators.h"
#include "test.h"
#include "util.h"
#include "verify.h"
#include "verify_internal.h"
#include "verify_j2k.h"
#include <boost/algorithm/string.hpp>
#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <cstdio>
#include <iostream>
#include <tuple>


using std::list;
using std::make_pair;
using std::make_shared;
using std::pair;
using std::shared_ptr;
using std::string;
using std::vector;
using boost::optional;
using namespace boost::filesystem;


static list<pair<string, optional<path>>> stages;

static string filename_to_id(boost::filesystem::path path)
{
	return path.string().substr(4, path.string().length() - 8);
}

static
boost::filesystem::path
dcp_test1_pkl()
{
	return find_file("test/ref/DCP/dcp_test1", "pkl_").filename();
}

static
string
dcp_test1_pkl_id()
{
	return filename_to_id(dcp_test1_pkl());
}

static
boost::filesystem::path
dcp_test1_cpl()
{
	return find_file("test/ref/DCP/dcp_test1", "cpl_").filename();
}

static
string
dcp_test1_cpl_id()
{
	return filename_to_id(dcp_test1_cpl());
}

static string const dcp_test1_asset_map_id = "017b3de4-6dda-408d-b19b-6711354b0bc3";

static
string
encryption_test_cpl_id()
{
	return filename_to_id(find_file("test/ref/DCP/encryption_test", "cpl_").filename());
}

static
string
encryption_test_pkl_id()
{
	return filename_to_id(find_file("test/ref/DCP/encryption_test", "pkl_").filename());
}

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


static
path
find_prefix(path dir, string prefix)
{
	auto iter = std::find_if(directory_iterator(dir), directory_iterator(), [prefix](path const& p) {
		return boost::starts_with(p.filename().string(), prefix);
	});

	BOOST_REQUIRE(iter != directory_iterator());
	return iter->path();
}


static
path
find_cpl(path dir)
{
	return find_prefix(dir, "cpl_");
}


static
path
find_pkl(path dir)
{
	return find_prefix(dir, "pkl_");
}


static
path
find_asset_map(path dir)
{
	return find_prefix(dir, "ASSETMAP");
}


/** Copy dcp_test{reference_number} to build/test/verify_test{verify_test_suffix}
 *  to make a new sacrificial test DCP.
 */
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
	dcp->set_annotation_text("hello");

	auto signer = make_shared<dcp::CertificateChain>(dcp::file_to_string("test/data/certificate_chain"));
	signer->set_key(dcp::file_to_string("test/data/private.key"));

	dcp->write_xml(signer);

	return cpl;
}


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
string
to_string(dcp::VerificationNote const& note)
{
	string s = note_to_string(note) + dcp::String::compose(
		"\n  [%1 %2 %3 %4 %5 %6 ",
		static_cast<int>(note.type()),
		static_cast<int>(note.code()),
		note.note().get_value_or("<none>"),
		note.file().get_value_or("<none>"),
		note.line().get_value_or(0),
		note.frame().get_value_or(0)
		);

	s += dcp::String::compose(
		"%1 %2 %3 %4 %5]\n",
		note.id().get_value_or("<none>"),
		note.other_id().get_value_or("<none>"),
		note.cpl_id().get_value_or("<none>"),
		note.reference_hash().get_value_or("<none>"),
		note.calculated_hash().get_value_or("<none>")
		);

	return s;
}


static
void
check_verify_result(vector<dcp::VerificationNote> notes, vector<dcp::VerificationNote> test_notes)
{
	std::sort(notes.begin(), notes.end());
	std::sort(test_notes.begin(), test_notes.end());

	string message = "\n";

	vector<dcp::VerificationNote> not_expected;
	for (auto note: notes) {
		auto iter = std::find_if(test_notes.begin(), test_notes.end(), [note](dcp::VerificationNote const& n) { return note.type() == n.type() && note.code() == n.code(); });
		if (iter != test_notes.end() && *iter != note) {
			message += "Wrong details:\n --seen     " + to_string(note) + " --expected " + to_string(*iter) + "\n";
		} else if (iter == test_notes.end()) {
			not_expected.push_back(note);
		}
	}

	vector<dcp::VerificationNote> not_seen;
	for (auto note: test_notes) {
		auto iter = std::find_if(notes.begin(), notes.end(), [note](dcp::VerificationNote const& n) { return note.type() == n.type() && note.code() == n.code(); });
		if (iter == notes.end()) {
			not_seen.push_back(note);
		}
	}

	for (auto note: not_expected) {
		message += "Not expected:\n" + to_string(note) + "\n";
	}

	for (auto note: not_seen) {
		message += "Not seen:\n" + to_string(note) + "\n";
	}

	BOOST_REQUIRE_MESSAGE(notes == test_notes, message);
}


static
void
check_verify_result(vector<path> dir, vector<dcp::DecryptedKDM> kdm, vector<dcp::VerificationNote> test_notes)
{
	check_verify_result(dcp::verify({dir}, kdm, &stage, &progress, {}, xsd_test).notes, test_notes);
}


/* Copy dcp_test1 to build/test/verify_test{suffix} then edit a file found by the functor 'file',
 * replacing from with to.
 */
static
void
replace(string suffix, boost::function<path (string)> file, string from, string to)
{
	auto dir = setup (1, suffix);

	{
		Editor e (file(suffix));
		e.replace (from, to);
	}
}


static
void
add_font(shared_ptr<dcp::TextAsset> asset)
{
	dcp::ArrayData fake_font(1024);
	asset->add_font("font", fake_font);
}


class HashCalculator
{
public:
	HashCalculator(boost::filesystem::path path)
		: _path(path)
		, _old_hash(dcp::make_digest(path, [](int64_t, int64_t) {}))
	{}

	std::string old_hash() const {
		return _old_hash;
	}

	std::string new_hash() const {
		return dcp::make_digest(_path, [](int64_t, int64_t) {});
	}

private:
	boost::filesystem::path _path;
	std::string _old_hash;
};


static
dcp::VerificationNote
ok(dcp::VerificationNote::Code code, shared_ptr<const dcp::CPL> cpl)
{
	return dcp::VerificationNote(dcp::VerificationNote::Type::OK, code).set_cpl_id(cpl->id());
}


static
dcp::VerificationNote
ok(dcp::VerificationNote::Code code, string note, shared_ptr<const dcp::CPL> cpl)
{
	return dcp::VerificationNote(dcp::VerificationNote::Type::OK, code, note).set_cpl_id(cpl->id());
}


static
dcp::VerificationNote
ok(dcp::VerificationNote::Code code, boost::filesystem::path path, shared_ptr<const dcp::CPL> cpl)
{
	return dcp::VerificationNote(dcp::VerificationNote::Type::OK, code, path).set_cpl_id(cpl->id());
}


void
add(vector<dcp::VerificationNote>& notes, vector<dcp::VerificationNote> const& add)
{
	for (auto i: add) {
		notes.push_back(i);
	}
}


BOOST_AUTO_TEST_CASE (verify_no_error)
{
	stages.clear ();
	auto dir = setup (1, "no_error");
	auto notes = dcp::verify({dir}, {}, &stage, &progress, {}, xsd_test).notes;

	path const cpl_file = dir / dcp_test1_cpl();
	path const pkl_file = dir / dcp_test1_pkl();
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
	BOOST_CHECK_EQUAL (st->first, "Checking picture asset details");
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
	++st; BOOST_CHECK_EQUAL (st->first, "Checking ASSETMAP");
	BOOST_REQUIRE (st->second);
	BOOST_CHECK_EQUAL (st->second.get(), canonical(assetmap_file));
	++st;
	BOOST_REQUIRE (st == stages.end());

	for (auto note: notes) {
		BOOST_CHECK(note.type() == dcp::VerificationNote::Type::OK);
	}
}


BOOST_AUTO_TEST_CASE (verify_incorrect_picture_sound_hash)
{
	using namespace boost::filesystem;

	auto dir = setup (1, "incorrect_picture_sound_hash");
	auto cpl = make_shared<dcp::CPL>(find_cpl(dir));

	auto video_path = path(dir / "video.mxf");
	HashCalculator video_calc(video_path);
	auto mod = fopen(video_path.string().c_str(), "r+b");
	BOOST_REQUIRE (mod);
	BOOST_REQUIRE_EQUAL(fseek(mod, -16, SEEK_END), 0);
	int x = 42;
	BOOST_REQUIRE(fwrite(&x, sizeof(x), 1, mod) == 1);
	fclose (mod);

	auto audio_path = path(dir / "audio.mxf");
	HashCalculator audio_calc(audio_path);
	mod = fopen(audio_path.string().c_str(), "r+b");
	BOOST_REQUIRE (mod);
	BOOST_REQUIRE_EQUAL(fseek(mod, 0, SEEK_END), 0);
	BOOST_REQUIRE (fwrite (&x, sizeof(x), 1, mod) == 1);
	fclose (mod);

	dcp::ASDCPErrorSuspender sus;
	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(dcp_test1_cpl_id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INCORRECT_PICTURE_HASH, canonical(video_path)
				).set_cpl_id(dcp_test1_cpl_id()).set_reference_hash(video_calc.old_hash()).set_calculated_hash(video_calc.new_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INCORRECT_SOUND_HASH, canonical(audio_path)
				).set_cpl_id(dcp_test1_cpl_id()).set_reference_hash(audio_calc.old_hash()).set_calculated_hash(audio_calc.new_hash()),
		});
}


BOOST_AUTO_TEST_CASE (verify_mismatched_picture_sound_hashes)
{
	using namespace boost::filesystem;

	auto dir = setup (1, "mismatched_picture_sound_hashes");
	auto cpl = make_shared<dcp::CPL>(find_cpl(dir));

	HashCalculator calc(dir / dcp_test1_cpl());

	{
		Editor e (dir / dcp_test1_pkl());
		e.replace ("<Hash>", "<Hash>x");
	}

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, canonical(dir / dcp_test1_cpl())
				).set_cpl_id(dcp_test1_cpl_id()).set_reference_hash("x" + calc.old_hash()).set_calculated_hash(calc.old_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_PICTURE_HASHES, canonical(dir / "video.mxf")
				).set_cpl_id(dcp_test1_cpl_id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_SOUND_HASHES, canonical(dir / "audio.mxf")
				).set_cpl_id(dcp_test1_cpl_id()),
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, "value 'x3M7YTgvFKXXMEGLkIbV4miC90FE=' is invalid Base64-encoded binary", canonical(dir / dcp_test1_pkl()), 28 },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, "value 'xskI+5b/9LA/y6h0mcyxysJYanxI=' is invalid Base64-encoded binary", canonical(dir / dcp_test1_pkl()), 12 },
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, "value 'xvsVjRV9vhTBPUWfE/TT1o2vdQsI=' is invalid Base64-encoded binary", canonical(dir / dcp_test1_pkl()), 20 },
		});
}


BOOST_AUTO_TEST_CASE (verify_failed_read_content_kind)
{
	auto dir = setup (1, "failed_read_content_kind");

	HashCalculator calc(dir / dcp_test1_cpl());

	{
		Editor e (dir / dcp_test1_cpl());
		e.replace ("<ContentKind>", "<ContentKind>x");
	}

	auto cpl = std::make_shared<dcp::CPL>(find_cpl(dir));

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, canonical(dir / dcp_test1_cpl())
				).set_cpl_id(dcp_test1_cpl_id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_CONTENT_KIND, string("xtrailer")
				).set_cpl_id(dcp_test1_cpl_id())
		});
}


static
path
dcp_test1_cpl_path(string suffix)
{
	return dcp::String::compose("build/test/verify_test%1/%2", suffix, dcp_test1_cpl());
}


static
path
dcp_test1_pkl_path(string suffix)
{
	return dcp::String::compose("build/test/verify_test%1/%2", suffix, dcp_test1_pkl());
}


static
path
asset_map (string suffix)
{
	return dcp::String::compose("build/test/verify_test%1/ASSETMAP.xml", suffix);
}


BOOST_AUTO_TEST_CASE (verify_invalid_picture_frame_rate)
{
	auto const suffix = "invalid_picture_frame_rate";

	replace(suffix, &dcp_test1_cpl_path, "<FrameRate>24 1", "<FrameRate>99 1");

	auto const dir = dcp::String::compose("build/test/verify_test%1", suffix);
	auto const cpl_path = find_cpl(dir);
	auto cpl = std::make_shared<dcp::CPL>(cpl_path);

	std::vector<dcp::VerificationNote> expected =
		{
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, canonical(cpl_path)
				).set_cpl_id(cpl->id()).set_calculated_hash("7n7GQ2TbxQbmHYuAR8ml7XDOep8=").set_reference_hash("skI+5b/9LA/y6h0mcyxysJYanxI="),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_PICTURE_FRAME_RATE, string{"99/1"}
				).set_cpl_id(cpl->id())
		};

	check_verify_result(dcp::verify({dir}, {}, &stage, &progress, {}, xsd_test).notes, expected);
}


BOOST_AUTO_TEST_CASE (verify_missing_asset)
{
	auto dir = setup (1, "missing_asset");
	remove (dir / "video.mxf");

	auto cpl = std::make_shared<dcp::CPL>(find_cpl(dir));

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISSING_ASSET, canonical(dir) / "video.mxf" }
		});
}


BOOST_AUTO_TEST_CASE (verify_empty_asset_path)
{
	auto const suffix = "empty_asset_path";

	replace("empty_asset_path", &asset_map, "<Path>video.mxf</Path>", "<Path></Path>");

	auto const dir = dcp::String::compose("build/test/verify_test%1", suffix);
	auto const cpl_path = find_cpl(dir);
	auto cpl = std::make_shared<dcp::CPL>(cpl_path);

	std::vector<dcp::VerificationNote> expected = {
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::EMPTY_ASSET_PATH }
		};

	check_verify_result(dcp::verify({dir}, {}, &stage, &progress, {}, xsd_test).notes, expected);
}


BOOST_AUTO_TEST_CASE (verify_mismatched_standard)
{
	auto const suffix = "mismatched_standard";

	replace(suffix, &dcp_test1_cpl_path, "http://www.smpte-ra.org/schemas/429-7/2006/CPL", "http://www.digicine.com/PROTO-ASDCP-CPL-20040511#");

	auto const dir = dcp::String::compose("build/test/verify_test%1", suffix);
	auto const cpl_path = find_cpl(dir);
	auto cpl = std::make_shared<dcp::CPL>(cpl_path);

	std::vector<dcp::VerificationNote> expected = {
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_STANDARD },
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, "invalid character encountered", canonical(cpl_path), 42
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, "no declaration found for element 'Id'", canonical(cpl_path), 53
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, "no declaration found for element 'EditRate'", canonical(cpl_path), 54
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, "no declaration found for element 'IntrinsicDuration'", canonical(cpl_path), 55
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML,
				"element 'Id' is not allowed for content model '(Id,AnnotationText?,EditRate,IntrinsicDuration,"
				"EntryPoint?,Duration?,FullContentTitleText,ReleaseTerritory?,VersionNumber?,Chain?,Distributor?,"
				"Facility?,AlternateContentVersionList?,Luminance?,MainSoundConfiguration,MainSoundSampleRate,"
				"MainPictureStoredArea,MainPictureActiveArea,MainSubtitleLanguageList?,ExtensionMetadataList?,)'",
				canonical(cpl_path), 149
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, canonical(cpl_path)
				).set_cpl_id(cpl->id()).set_reference_hash("skI+5b/9LA/y6h0mcyxysJYanxI=").set_calculated_hash("FZ9E7L/pOuJ6aZfbiaANTv8BFOo=")
		};

	check_verify_result(dcp::verify({dir}, {}, &stage, &progress, {}, xsd_test).notes, expected);
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_cpl_id)
{
	auto const suffix = "invalid_xml_cpl_id";

	/* There's no MISMATCHED_CPL_HASHES error here because it can't find the correct hash by ID (since the ID is wrong) */
	replace("invalid_xml_cpl_id", &dcp_test1_cpl_path, "<Id>urn:uuid:6affb8ee-0020-4dff-a53c-17652f6358ab", "<Id>urn:uuid:6affb8ee-0020-4dff-a53c-17652f6358a");

	auto const dir = dcp::String::compose("build/test/verify_test%1", suffix);
	auto const cpl_path = find_cpl(dir);
	auto cpl = std::make_shared<dcp::CPL>(cpl_path);

	std::vector<dcp::VerificationNote> expected = {
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML,
				"value 'urn:uuid:6affb8ee-0020-4dff-a53c-17652f6358a' does not match regular expression "
				"facet 'urn:uuid:[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}'", canonical(cpl_path), 3
				).set_cpl_id(cpl->id())
		};

	check_verify_result(dcp::verify({dir}, {}, &stage, &progress, {}, xsd_test).notes, expected);
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_issue_date)
{
	auto const suffix = "invalid_xml_issue_date";

	replace("invalid_xml_issue_date", &dcp_test1_cpl_path, "<IssueDate>", "<IssueDate>x");

	auto const dir = dcp::String::compose("build/test/verify_test%1", suffix);
	auto const cpl_path = find_cpl(dir);
	auto cpl = std::make_shared<dcp::CPL>(cpl_path);

	std::vector<dcp::VerificationNote> expected = {
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, canonical(cpl_path)
				).set_cpl_id(cpl->id()).set_reference_hash("skI+5b/9LA/y6h0mcyxysJYanxI=").set_calculated_hash("sz3BeIugJ567q3HMnA62JeRw4TE="),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML,
				"invalid character encountered",
				canonical(cpl_path), 5
				).set_cpl_id(cpl->id()),
		};

	check_verify_result(dcp::verify({dir}, {}, &stage, &progress, {}, xsd_test).notes, expected);
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_pkl_id)
{
	auto const suffix = "invalid_xml_pkl_id";

	replace("invalid_xml_pkl_id", &dcp_test1_pkl_path, "<Id>urn:uuid:" + dcp_test1_pkl_id().substr(0, 3), "<Id>urn:uuid:x" + dcp_test1_pkl_id().substr(1, 2));

	auto const dir = dcp::String::compose("build/test/verify_test%1", suffix);
	auto const pkl_path = find_pkl(dir);
	auto const cpl_path = find_cpl(dir);
	auto cpl = std::make_shared<dcp::CPL>(cpl_path);

	std::vector<dcp::VerificationNote> expected = {
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML,
				"value 'urn:uuid:x199d58b-5ef8-4d49-b270-07e590ccb280' does not match regular "
				"expression facet 'urn:uuid:[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}'",
				canonical(pkl_path), 3
				),
	};

	check_verify_result(dcp::verify({dir}, {}, &stage, &progress, {}, xsd_test).notes, expected);
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_asset_map_id)
{
	auto const suffix = "invalid_xml_asset_map_id";

	replace("invalid_xml_asset_map_id", &asset_map, "<Id>urn:uuid:" + dcp_test1_asset_map_id.substr(0, 3), "<Id>urn:uuid:x" + dcp_test1_asset_map_id.substr(1, 2));

	auto const dir = dcp::String::compose("build/test/verify_test%1", suffix);
	auto const cpl_path = find_cpl(dir);
	auto const asset_map_path = find_asset_map(dir);
	auto cpl = std::make_shared<dcp::CPL>(cpl_path);

	std::vector<dcp::VerificationNote> expected = {
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML,
				"value 'urn:uuid:x17b3de4-6dda-408d-b19b-6711354b0bc3' does not match regular "
				"expression facet 'urn:uuid:[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}'",
				canonical(asset_map_path), 3
				),
	};

	check_verify_result(dcp::verify({dir}, {}, &stage, &progress, {}, xsd_test).notes, expected);
}


BOOST_AUTO_TEST_CASE (verify_invalid_standard)
{
	stages.clear ();
	auto dir = setup (3, "verify_invalid_standard");
	auto notes = dcp::verify({dir}, {}, &stage, &progress, {}, xsd_test).notes;

	path const cpl_file = dir / "cpl_cbfd2bc0-21cf-4a8f-95d8-9cddcbe51296.xml";
	path const pkl_file = dir / "pkl_d87a950c-bd6f-41f6-90cc-56ccd673e131.xml";
	path const assetmap_file = dir / "ASSETMAP";
	auto cpl = std::make_shared<dcp::CPL>(cpl_file);

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
	BOOST_CHECK_EQUAL (st->first, "Checking picture asset details");
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

	vector<dcp::VerificationNote> expected = {
		ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
		{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_STANDARD },
		ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"feature"}, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
		ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "j2c_c6035f97-b07d-4e1c-944d-603fc2ddc242.mxf"), cpl),
		ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "j2c_c6035f97-b07d-4e1c-944d-603fc2ddc242.mxf"), cpl)
	};

	for (int j = 0; j < 24; ++j) {
		expected.push_back(
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_JPEG2000_GUARD_BITS_FOR_2K, string("2")
				).set_cpl_id(cpl->id())
		);
	}

	check_verify_result(notes, expected);
}

/* DCP with a short asset */
BOOST_AUTO_TEST_CASE (verify_invalid_duration)
{
	auto dir = setup (8, "invalid_duration");

	dcp::DCP dcp(dir);
	dcp.read();
	BOOST_REQUIRE(dcp.cpls().size() == 1);
	auto cpl = dcp.cpls()[0];

	vector<dcp::VerificationNote> expected = {
		ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
		ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"feature"}, cpl),
		ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "j2c_d7576dcb-a361-4139-96b8-267f5f8d7f91.mxf"), cpl),
		ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "j2c_d7576dcb-a361-4139-96b8-267f5f8d7f91.mxf"), cpl),
		{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_STANDARD },
		dcp::VerificationNote(
			dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_DURATION, string("d7576dcb-a361-4139-96b8-267f5f8d7f91")
			).set_cpl_id(cpl->id()),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_INTRINSIC_DURATION, string("d7576dcb-a361-4139-96b8-267f5f8d7f91")
			).set_cpl_id(cpl->id()),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_DURATION, string("a2a87f5d-b749-4a7e-8d0c-9d48a4abf626")
			).set_cpl_id(cpl->id()),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_INTRINSIC_DURATION, string("a2a87f5d-b749-4a7e-8d0c-9d48a4abf626")
			).set_cpl_id(cpl->id()),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::WARNING,
			dcp::VerificationNote::Code::EMPTY_CONTENT_VERSION_LABEL_TEXT,
			cpl->file().get()
			).set_cpl_id(cpl->id())
	};

	for (int i = 0; i < 23; ++i) {
		expected.push_back(
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_JPEG2000_GUARD_BITS_FOR_2K, string("2")
				).set_cpl_id(cpl->id())
			);
	}

	check_verify_result({ dir }, {}, expected);
}


static
shared_ptr<dcp::CPL>
dcp_from_frame (dcp::ArrayData const& frame, path dir)
{
	auto asset = make_shared<dcp::MonoJ2KPictureAsset>(dcp::Fraction(24, 1), dcp::Standard::SMPTE);
	create_directories (dir);
	auto writer = asset->start_write(dir / "pic.mxf", dcp::Behaviour::MAKE_NEW);
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

	vector<dcp::VerificationNote> expected = {
		ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
		ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
		ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
		ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "pic.mxf"), cpl),
	};

	for (auto i = 0; i < 24; ++i) {
		expected.push_back(
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_JPEG2000_CODESTREAM, string("missing marker start byte")
				).set_frame(i).set_frame_rate(24).set_cpl_id(cpl->id())
			);
	}

	for (auto i = 0; i < 24; ++i) {
		expected.push_back(
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_PICTURE_FRAME_SIZE_IN_BYTES, canonical(dir / "pic.mxf")
				).set_frame(i).set_frame_rate(24).set_cpl_id(cpl->id())
			);
	}

	expected.push_back(
		dcp::VerificationNote(
			dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
			).set_cpl_id(cpl->id())
		);

	check_verify_result({ dir }, {}, expected);
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

	vector<dcp::VerificationNote> expected = {
		ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "pic.mxf"), cpl),
		ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
		ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
		ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
	};

	for (auto i = 0; i < 24; ++i) {
		expected.push_back(
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_JPEG2000_CODESTREAM, string("missing marker start byte")
				).set_frame(i).set_frame_rate(24).set_cpl_id(cpl->id())
			);
	}

	for (auto i = 0; i < 24; ++i) {
		expected.push_back(
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::NEARLY_INVALID_PICTURE_FRAME_SIZE_IN_BYTES, canonical(dir / "pic.mxf")
				).set_frame(i).set_frame_rate(24).set_cpl_id(cpl->id())
		);
	}

	expected.push_back(
		dcp::VerificationNote(
			dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
			).set_cpl_id(cpl->id())
		);

	check_verify_result ({ dir }, {}, expected);
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

	check_verify_result(
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "pic.mxf"), cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "pic.mxf"), cpl),
			dcp::VerificationNote(dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_interop_subtitles)
{
	path const dir("build/test/verify_valid_interop_subtitles");
	prepare_directory (dir);
	copy_file ("test/data/subs1.xml", dir / "subs.xml");
	auto asset = make_shared<dcp::InteropTextAsset>(dir / "subs.xml");
	auto reel_asset = make_shared<dcp::ReelInteropTextAsset>(dcp::TextType::OPEN_SUBTITLE, asset, dcp::Fraction(24, 1), 16 * 24, 0);
	auto cpl = write_dcp_with_single_asset(dir, reel_asset, dcp::Standard::INTEROP);

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_STANDARD },
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISSING_FONT, string{"theFontId"}
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE(verify_catch_missing_font_file_with_interop_ccap)
{
	path const dir("build/test/verify_catch_missing_font_file_with_interop_ccap");
	prepare_directory(dir);
	copy_file("test/data/subs1.xml", dir / "ccap.xml");
	auto asset = make_shared<dcp::InteropTextAsset>(dir / "ccap.xml");
	auto reel_asset = make_shared<dcp::ReelInteropTextAsset>(dcp::TextType::CLOSED_CAPTION, asset, dcp::Fraction(24, 1), 16 * 24, 0);
	auto cpl = write_dcp_with_single_asset(dir, reel_asset, dcp::Standard::INTEROP);

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_STANDARD },
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISSING_FONT, string{"theFontId"}
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_interop_subtitles)
{
	using namespace boost::filesystem;

	path const dir("build/test/verify_invalid_interop_subtitles");
	prepare_directory (dir);
	copy_file ("test/data/subs1.xml", dir / "subs.xml");
	auto asset = make_shared<dcp::InteropTextAsset>(dir / "subs.xml");
	auto reel_asset = make_shared<dcp::ReelInteropTextAsset>(dcp::TextType::OPEN_SUBTITLE, asset, dcp::Fraction(24, 1), 16 * 24, 0);
	auto cpl = write_dcp_with_single_asset(dir, reel_asset, dcp::Standard::INTEROP);

	{
		Editor e (dir / "subs.xml");
		e.replace ("</ReelNumber>", "</ReelNumber><Foo></Foo>");
	}

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_STANDARD },
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'Foo'"), path(), 5
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR,
				dcp::VerificationNote::Code::INVALID_XML,
				string("element 'Foo' is not allowed for content model '(SubtitleID,MovieTitle,ReelNumber,Language,LoadFont*,Font*,Subtitle*)'"),
				path(),
				29
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISSING_FONT, string{"theFontId"}
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE(verify_interop_subtitle_asset_with_no_subtitles)
{
	path const dir("build/test/verify_interop_subtitle_asset_with_no_subtitles");
	prepare_directory(dir);
	copy_file("test/data/subs4.xml", dir / "subs.xml");
	auto asset = make_shared<dcp::InteropTextAsset>(dir / "subs.xml");
	auto reel_asset = make_shared<dcp::ReelInteropTextAsset>(dcp::TextType::OPEN_SUBTITLE, asset, dcp::Fraction(24, 1), 16 * 24, 0);
	auto cpl = write_dcp_with_single_asset(dir, reel_asset, dcp::Standard::INTEROP);

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_STANDARD },
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE, asset->id(), boost::filesystem::canonical(asset->file().get())
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISSING_FONT, string{"theFontId"}
				).set_cpl_id(cpl->id())
		});

}


BOOST_AUTO_TEST_CASE(verify_interop_subtitle_asset_with_single_space_subtitle)
{
	path const dir("build/test/verify_interop_subtitle_asset_with_single_space_subtitle");
	prepare_directory(dir);
	copy_file("test/data/subs5.xml", dir / "subs.xml");
	auto asset = make_shared<dcp::InteropTextAsset>(dir / "subs.xml");
	auto reel_asset = make_shared<dcp::ReelInteropTextAsset>(dcp::TextType::OPEN_SUBTITLE, asset, dcp::Fraction(24, 1), 16 * 24, 0);
	auto cpl = write_dcp_with_single_asset(dir, reel_asset, dcp::Standard::INTEROP);

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_STANDARD },
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISSING_FONT, string{"Arial"}
				).set_cpl_id(cpl->id())
		});

}


BOOST_AUTO_TEST_CASE (verify_valid_smpte_subtitles)
{
	path const dir("build/test/verify_valid_smpte_subtitles");
	prepare_directory (dir);
	copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTETextAsset>(dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, asset, dcp::Fraction(24, 1), 6046, 0);
	auto cpl = write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result(
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_ISSUE_DATE, string{"2021-04-14T13:19:14.000+02:00"}
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INCORRECT_SUBTITLE_NAMESPACE_COUNT, asset->id()
				).set_cpl_id(cpl->id()),
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_smpte_subtitles)
{
	using namespace boost::filesystem;

	path const dir("build/test/verify_invalid_smpte_subtitles");
	prepare_directory (dir);
	/* This broken_smpte.mxf does not use urn:uuid: for its subtitle ID, which we tolerate (rightly or wrongly) */
	copy_file ("test/data/broken_smpte.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTETextAsset>(dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, asset, dcp::Fraction(24, 1), 6046, 0);
	auto cpl = write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'Foo'"), path(), 2
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR,
				dcp::VerificationNote::Code::INVALID_XML,
				string("element 'Foo' is not allowed for content model '(Id,ContentTitleText,AnnotationText?,IssueDate,ReelNumber?,Language?,EditRate,TimeCodeRate,StartTime?,DisplayType?,LoadFont*,SubtitleList)'"),
				path(),
				2
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(dir / "subs.mxf")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_ISSUE_DATE, string{"2020-05-09T00:29:21.000+02:00"}
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INCORRECT_SUBTITLE_NAMESPACE_COUNT, asset->id()
				).set_cpl_id(cpl->id()),
		});
}


BOOST_AUTO_TEST_CASE (verify_empty_text_node_in_subtitles)
{
	path const dir("build/test/verify_empty_text_node_in_subtitles");
	prepare_directory (dir);
	copy_file ("test/data/empty_text.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTETextAsset>(dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, asset, dcp::Fraction(24, 1), 192, 0);
	auto cpl = write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::EMPTY_TEXT
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_LANGUAGE, canonical(dir / "subs.mxf")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_ISSUE_DATE, string{"2021-08-09T18:34:46.000+02:00"}
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INCORRECT_SUBTITLE_NAMESPACE_COUNT, asset->id()
				).set_cpl_id(cpl->id())
		});
}


/** A <Text> node with no content except some <Font> nodes, which themselves do have content */
BOOST_AUTO_TEST_CASE (verify_empty_text_node_in_subtitles_with_child_nodes)
{
	path const dir("build/test/verify_empty_text_node_in_subtitles_with_child_nodes");
	prepare_directory (dir);
	copy_file ("test/data/empty_but_with_children.xml", dir / "subs.xml");
	auto asset = make_shared<dcp::InteropTextAsset>(dir / "subs.xml");
	auto reel_asset = make_shared<dcp::ReelInteropTextAsset>(dcp::TextType::OPEN_SUBTITLE, asset, dcp::Fraction(24, 1), 192, 0);
	auto cpl = write_dcp_with_single_asset (dir, reel_asset, dcp::Standard::INTEROP);

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_STANDARD },
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISSING_FONT, string{"font0"}
				).set_cpl_id(cpl->id())
		});
}


/** A <Text> node with no content except some <Font> nodes, which themselves also have no content */
BOOST_AUTO_TEST_CASE (verify_empty_text_node_in_subtitles_with_empty_child_nodes)
{
	path const dir("build/test/verify_empty_text_node_in_subtitles_with_empty_child_nodes");
	prepare_directory (dir);
	copy_file ("test/data/empty_with_empty_children.xml", dir / "subs.xml");
	auto asset = make_shared<dcp::InteropTextAsset>(dir / "subs.xml");
	auto reel_asset = make_shared<dcp::ReelInteropTextAsset>(dcp::TextType::OPEN_SUBTITLE, asset, dcp::Fraction(24, 1), 192, 0);
	auto cpl = write_dcp_with_single_asset (dir, reel_asset, dcp::Standard::INTEROP);

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE, asset->id(), boost::filesystem::canonical(asset->file().get())
				).set_cpl_id(cpl->id()),
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_STANDARD },
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::EMPTY_TEXT
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISSING_FONT, string{"font0"}
				).set_cpl_id(cpl->id())
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
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			{ dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::EXTERNAL_ASSET, picture->asset()->id() },
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_cpl_metadata)
{
	path const dir("build/test/verify_valid_cpl_metadata");
	prepare_directory (dir);

	copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTETextAsset>(dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, asset, dcp::Fraction(24, 1), 16 * 24, 0);

	auto reel = make_shared<dcp::Reel>();
	reel->add (reel_asset);

	reel->add (make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "", 16 * 24), 0));
	reel->add (simple_markers(16 * 24));

	auto cpl = make_shared<dcp::CPL>("hello", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
	cpl->add (reel);
	cpl->set_main_sound_configuration(dcp::MainSoundConfiguration("51/L,C,R,LFE,-,-"));
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));
	cpl->set_version_number (1);

	dcp::DCP dcp (dir);
	dcp.add (cpl);
	dcp.set_annotation_text("hello");
	dcp.write_xml ();
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
	cpl->set_main_sound_configuration(dcp::MainSoundConfiguration("51/L,C,R,LFE,-,-"));
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));
	cpl->set_version_number (1);

	reel->add (simple_markers());

	dcp::DCP dcp (dir);
	dcp.add (cpl);
	dcp.set_annotation_text("hello");
	dcp.write_xml();

	HashCalculator calc(find_cpl(dir));

	{
		Editor e (find_cpl(dir));
		e.replace ("MainSound", "MainSoundX");
	}

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "pic.mxf"), cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1440x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "pic.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'meta:MainSoundXConfiguration'"), canonical(cpl->file().get()), 50
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'meta:MainSoundXSampleRate'"), canonical(cpl->file().get()), 51
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR,
				dcp::VerificationNote::Code::INVALID_XML,
				string("element 'meta:MainSoundXConfiguration' is not allowed for content model "
				       "'(Id,AnnotationText?,EditRate,IntrinsicDuration,EntryPoint?,Duration?,"
				       "FullContentTitleText,ReleaseTerritory?,VersionNumber?,Chain?,Distributor?,"
				       "Facility?,AlternateContentVersionList?,Luminance?,MainSoundConfiguration,"
				       "MainSoundSampleRate,MainPictureStoredArea,MainPictureActiveArea,MainSubtitleLanguageList?,"
				       "ExtensionMetadataList?,)'"),
				canonical(cpl->file().get()),
				71).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, canonical(cpl->file().get())
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash())
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
	cpl->set_main_sound_configuration(dcp::MainSoundConfiguration("51/L,C,R,LFE,-,-"));
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));

	dcp::DCP dcp (dir);
	dcp.add (cpl);
	dcp.set_annotation_text("hello");
	dcp.write_xml();

	{
		Editor e (find_cpl(dir));
		e.replace ("meta:Width", "meta:WidthX");
	}

	check_verify_result (
		{ dir },
		{},
		{{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::FAILED_READ, string("missing XML tag Width in MainPictureStoredArea") }}
		);
}


BOOST_AUTO_TEST_CASE (verify_invalid_language1)
{
	path const dir("build/test/verify_invalid_language1");
	prepare_directory (dir);
	copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTETextAsset>(dir / "subs.mxf");
	asset->_language = "wrong-andbad";
	asset->write (dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, asset, dcp::Fraction(24, 1), 6046, 0);
	reel_asset->_language = "badlang";
	auto cpl = write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("badlang")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("wrong-andbad")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


/* SMPTE DCP with invalid <Language> in the MainClosedCaption reel and also in the XML within the MXF */
BOOST_AUTO_TEST_CASE (verify_invalid_language2)
{
	path const dir("build/test/verify_invalid_language2");
	prepare_directory (dir);
	copy_file ("test/data/subs.mxf", dir / "subs.mxf");
	auto asset = make_shared<dcp::SMPTETextAsset>(dir / "subs.mxf");
	asset->_language = "wrong-andbad";
	asset->write (dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::CLOSED_CAPTION, asset, dcp::Fraction(24, 1), 6046, 0);
	reel_asset->_language = "badlang";
	auto cpl = write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("badlang")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("wrong-andbad")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
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
	cpl->set_main_sound_configuration(dcp::MainSoundConfiguration("51/L,C,R,LFE,-,-"));
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));
	cpl->set_version_number (1);
	cpl->_release_territory = "fred-jim";
	auto dcp = make_shared<dcp::DCP>(dir);
	dcp->add (cpl);
	dcp->set_annotation_text("hello");
	dcp->write_xml();

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "videofoo.mxf"), cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1440x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "videofoo.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("this-is-wrong")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("andso-is-this")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("fred-jim")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_LANGUAGE, string("frobozz")
				).set_cpl_id(cpl->id()),
		});
}


static
std::tuple<vector<dcp::VerificationNote>, shared_ptr<dcp::CPL>, boost::filesystem::path>
check_picture_size (int width, int height, int frame_rate, bool three_d)
{
	using namespace boost::filesystem;

	path dcp_path = "build/test/verify_picture_test";
	prepare_directory (dcp_path);

	shared_ptr<dcp::J2KPictureAsset> mp;
	if (three_d) {
		mp = make_shared<dcp::StereoJ2KPictureAsset>(dcp::Fraction(frame_rate, 1), dcp::Standard::SMPTE);
	} else {
		mp = make_shared<dcp::MonoJ2KPictureAsset>(dcp::Fraction(frame_rate, 1), dcp::Standard::SMPTE);
	}
	auto picture_writer = mp->start_write(dcp_path / "video.mxf", dcp::Behaviour::MAKE_NEW);

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
	cpl->set_main_sound_configuration(dcp::MainSoundConfiguration("51/L,C,R,LFE,-,-"));
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area(dcp::Size(width, height));
	cpl->set_main_picture_active_area(dcp::Size(width, height));
	cpl->set_version_number (1);

	auto reel = make_shared<dcp::Reel>();

	if (three_d) {
		reel->add (make_shared<dcp::ReelStereoPictureAsset>(std::dynamic_pointer_cast<dcp::StereoJ2KPictureAsset>(mp), 0));
	} else {
		reel->add (make_shared<dcp::ReelMonoPictureAsset>(std::dynamic_pointer_cast<dcp::MonoJ2KPictureAsset>(mp), 0));
	}

	reel->add (simple_markers(frame_rate));

	cpl->add (reel);

	d->add (cpl);
	d->set_annotation_text("A Test DCP");
	d->write_xml();

	/* It seems that for the Ubuntu 16.04 compiler we can't use an initializer list here */
	return std::tuple<vector<dcp::VerificationNote>, shared_ptr<dcp::CPL>, boost::filesystem::path>{ dcp::verify({dcp_path}, {}, &stage, &progress, {}, xsd_test).notes, cpl, dcp_path };
}


static
void
check_picture_size_ok (int width, int height, int frame_rate, bool three_d)
{
	vector<dcp::VerificationNote> notes;
	shared_ptr<dcp::CPL> cpl;
	boost::filesystem::path dir;
	std::tie(notes, cpl, dir) = check_picture_size(width, height, frame_rate, three_d);

	std::vector<dcp::VerificationNote> expected = {
		ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
		ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::OK,
			dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
			dcp::String::compose("%1x%2", width, height),
			cpl->file().get()
			).set_cpl_id(cpl->id()),
		ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
		ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl)
	};
	check_verify_result(notes, expected);
}


static
void
check_picture_size_bad_frame_size (int width, int height, int frame_rate, bool three_d)
{
	vector<dcp::VerificationNote> notes;
	shared_ptr<dcp::CPL> cpl;
	boost::filesystem::path dir;
	std::tie(notes, cpl, dir) = check_picture_size(width, height, frame_rate, three_d);

	std::vector<dcp::VerificationNote> expected = {
		ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
		ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::OK,
			dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
			dcp::String::compose("%1x%2", width, height),
			cpl->file().get()
			).set_cpl_id(cpl->id()),
		ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
		ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_PICTURE_SIZE_IN_PIXELS, dcp::String::compose("%1x%2", width, height), canonical(dir / "video.mxf")
			).set_cpl_id(cpl->id())
	};
	check_verify_result(notes, expected);
}


static
void
check_picture_size_bad_2k_frame_rate (int width, int height, int frame_rate, bool three_d)
{
	vector<dcp::VerificationNote> notes;
	shared_ptr<dcp::CPL> cpl;
	boost::filesystem::path dir;
	std::tie(notes, cpl, dir) = check_picture_size(width, height, frame_rate, three_d);

	std::vector<dcp::VerificationNote> expected = {
		ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
		ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::OK,
			dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
			dcp::String::compose("%1x%2", width, height),
			cpl->file().get()
			).set_cpl_id(cpl->id()),
		ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
		ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_PICTURE_FRAME_RATE, dcp::String::compose("%1/1", frame_rate * (three_d ? 2 : 1))
			).set_cpl_id(cpl->id()),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_PICTURE_FRAME_RATE_FOR_2K, dcp::String::compose("%1/1", frame_rate), canonical(dir / "video.mxf")
			).set_cpl_id(cpl->id())
	};

	check_verify_result(notes, expected);
}


static
void
check_picture_size_bad_4k_frame_rate (int width, int height, int frame_rate, bool three_d)
{
	vector<dcp::VerificationNote> notes;
	shared_ptr<dcp::CPL> cpl;
	boost::filesystem::path dir;
	std::tie(notes, cpl, dir) = check_picture_size(width, height, frame_rate, three_d);

	std::vector<dcp::VerificationNote> expected = {
		ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
		ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::OK,
			dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
			dcp::String::compose("%1x%2", width, height),
			cpl->file().get()
			).set_cpl_id(cpl->id()),
		ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
		ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_PICTURE_FRAME_RATE_FOR_4K, dcp::String::compose("%1/1", frame_rate), canonical(dir / "video.mxf")
			).set_cpl_id(cpl->id())
	};

	check_verify_result(notes, expected);
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
	vector<dcp::VerificationNote> notes;
	shared_ptr<dcp::CPL> cpl;
	boost::filesystem::path dir;
	std::tie(notes, cpl, dir) = check_picture_size(3996, 2160, 24, true);

	std::vector<dcp::VerificationNote> expected = {
		ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
		ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::OK,
			dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
			string{"3996x2160"},
			cpl->file().get()
			).set_cpl_id(cpl->id()),
		ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
		ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
		{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_PICTURE_ASSET_RESOLUTION_FOR_3D },
	};
}


static
void
add_test_subtitle (shared_ptr<dcp::TextAsset> asset, int start_frame, int end_frame, float v_position = 0, dcp::VAlign v_align = dcp::VAlign::CENTER, string text = "Hello")
{
	asset->add (
		std::make_shared<dcp::TextString>(
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
			v_align,
			0, vector<dcp::Text::VariableZPosition>(),
			dcp::Direction::LTR,
			text,
			dcp::Effect::NONE,
			dcp::Colour(),
			dcp::Time(),
			dcp::Time(),
			0,
			std::vector<dcp::Ruby>()
		)
	);
}


BOOST_AUTO_TEST_CASE (verify_invalid_closed_caption_xml_size_in_bytes)
{
	path const dir("build/test/verify_invalid_closed_caption_xml_size_in_bytes");
	prepare_directory (dir);

	auto asset = make_shared<dcp::SMPTETextAsset>();
	for (int i = 0; i < 2048; ++i) {
		add_test_subtitle (asset, i * 24, i * 24 + 20);
	}
	add_font(asset);
	asset->set_language (dcp::LanguageTag("de-DE"));
	asset->write (dir / "subs.mxf");
	auto reel_asset = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::CLOSED_CAPTION, asset, dcp::Fraction(24, 1), 49148, 0);
	auto cpl = write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(dir / "subs.mxf")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR,
				dcp::VerificationNote::Code::INVALID_CLOSED_CAPTION_XML_SIZE_IN_BYTES,
				string("419371"),
				canonical(dir / "subs.mxf")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


static
shared_ptr<dcp::SMPTETextAsset>
make_large_subtitle_asset (path font_file)
{
	auto asset = make_shared<dcp::SMPTETextAsset>();
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

	auto reel_asset = make_shared<T>(dcp::TextType::OPEN_SUBTITLE, asset, dcp::Fraction(24, 1), 240, 0);
	auto cpl = write_dcp_with_single_asset (dir, reel_asset);

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_TIMED_TEXT_SIZE_IN_BYTES, string("121698284"), canonical(dir / "subs.mxf")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR,
				dcp::VerificationNote::Code::INVALID_TIMED_TEXT_FONT_SIZE_IN_BYTES,
				"121634816",
				canonical(dir / "subs.mxf")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(dir / "subs.mxf")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_subtitle_asset_too_large)
{
	verify_timed_text_asset_too_large<dcp::ReelSMPTETextAsset>("verify_subtitle_asset_too_large");
	verify_timed_text_asset_too_large<dcp::ReelSMPTETextAsset>("verify_closed_caption_asset_too_large");
}


BOOST_AUTO_TEST_CASE (verify_missing_subtitle_language)
{
	path dir = "build/test/verify_missing_subtitle_language";
	prepare_directory (dir);
	auto dcp = make_simple (dir, 1, 106);

	string const xml =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SubtitleReel xmlns=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\">"
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

	dcp::File xml_file(dir / "subs.xml", "w");
	BOOST_REQUIRE (xml_file);
	xml_file.write(xml.c_str(), xml.size(), 1);
	xml_file.close();
	auto subs = make_shared<dcp::SMPTETextAsset>(dir / "subs.xml");
	subs->write (dir / "subs.mxf");

	auto reel_subs = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, subs, dcp::Fraction(24, 1), 106, 0);
	auto cpl = dcp->cpls()[0];
	cpl->reels()[0]->add(reel_subs);
	dcp->write_xml();

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_LANGUAGE, canonical(dir / "subs.mxf")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_mismatched_subtitle_languages)
{
	path path ("build/test/verify_mismatched_subtitle_languages");
	auto constexpr reel_length = 192;
	auto dcp = make_simple (path, 2, reel_length);
	auto cpl = dcp->cpls()[0];

	{
		auto subs = make_shared<dcp::SMPTETextAsset>();
		subs->set_language (dcp::LanguageTag("de-DE"));
		subs->add(simple_text());
		add_font(subs);
		subs->write (path / "subs1.mxf");
		auto reel_subs = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, subs, dcp::Fraction(24, 1), reel_length, 0);
		cpl->reels()[0]->add(reel_subs);
	}

	{
		auto subs = make_shared<dcp::SMPTETextAsset>();
		subs->set_language (dcp::LanguageTag("en-US"));
		subs->add(simple_text());
		add_font(subs);
		subs->write (path / "subs2.mxf");
		auto reel_subs = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, subs, dcp::Fraction(24, 1), reel_length, 0);
		cpl->reels()[1]->add(reel_subs);
	}

	dcp->write_xml();

	check_verify_result (
		{ path },
		{},
		{
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(path / "video0.mxf"), cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(path / "video1.mxf"), cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(path / "video0.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(path / "video1.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(path / "subs1.mxf")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(path / "subs2.mxf")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_SUBTITLE_LANGUAGES
				).set_cpl_id(cpl->id()),
		});
}


BOOST_AUTO_TEST_CASE (verify_multiple_closed_caption_languages_allowed)
{
	path path ("build/test/verify_multiple_closed_caption_languages_allowed");
	auto constexpr reel_length = 192;
	auto dcp = make_simple (path, 2, reel_length);
	auto cpl = dcp->cpls()[0];

	{
		auto ccaps = make_shared<dcp::SMPTETextAsset>();
		ccaps->set_language (dcp::LanguageTag("de-DE"));
		ccaps->add(simple_text());
		add_font(ccaps);
		ccaps->write (path / "subs1.mxf");
		auto reel_ccaps = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::CLOSED_CAPTION, ccaps, dcp::Fraction(24, 1), reel_length, 0);
		cpl->reels()[0]->add(reel_ccaps);
	}

	{
		auto ccaps = make_shared<dcp::SMPTETextAsset>();
		ccaps->set_language (dcp::LanguageTag("en-US"));
		ccaps->add(simple_text());
		add_font(ccaps);
		ccaps->write (path / "subs2.mxf");
		auto reel_ccaps = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::CLOSED_CAPTION, ccaps, dcp::Fraction(24, 1), reel_length, 0);
		cpl->reels()[1]->add(reel_ccaps);
	}

	dcp->write_xml();

	check_verify_result (
		{ path },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(path / "video0.mxf"), cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(path / "video1.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(path / "video0.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(path / "video1.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(path / "subs1.mxf")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(path / "subs2.mxf")
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_missing_subtitle_start_time)
{
	path dir = "build/test/verify_missing_subtitle_start_time";
	prepare_directory (dir);
	auto dcp = make_simple (dir, 1, 106);

	string const xml =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SubtitleReel xmlns=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\">"
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

	dcp::File xml_file(dir / "subs.xml", "w");
	BOOST_REQUIRE (xml_file);
	xml_file.write(xml.c_str(), xml.size(), 1);
	xml_file.close();
	auto subs = make_shared<dcp::SMPTETextAsset>(dir / "subs.xml");
	subs->write (dir / "subs.mxf");

	auto reel_subs = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, subs, dcp::Fraction(24, 1), 106, 0);
	auto cpl = dcp->cpls()[0];
	cpl->reels()[0]->add(reel_subs);
	dcp->write_xml();

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_START_TIME, canonical(dir / "subs.mxf")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_subtitle_start_time)
{
	path dir = "build/test/verify_invalid_subtitle_start_time";
	prepare_directory (dir);
	auto dcp = make_simple (dir, 1, 106);

	string const xml =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SubtitleReel xmlns=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\">"
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

	dcp::File xml_file(dir / "subs.xml", "w");
	BOOST_REQUIRE (xml_file);
	xml_file.write(xml.c_str(), xml.size(), 1);
	xml_file.close();
	auto subs = make_shared<dcp::SMPTETextAsset>(dir / "subs.xml");
	subs->write (dir / "subs.mxf");

	auto reel_subs = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, subs, dcp::Fraction(24, 1), 106, 0);
	auto cpl = dcp->cpls()[0];
	cpl->reels().front()->add(reel_subs);
	dcp->write_xml();

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_SUBTITLE_START_TIME, canonical(dir / "subs.mxf")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME
				).set_cpl_id(cpl->id())
		});
}


class TestText
{
public:
	TestText (int in_, int out_, float v_position_ = 0, dcp::VAlign v_align_ = dcp::VAlign::CENTER, string text_ = "Hello")
		: in(in_)
		, out(out_)
		, v_position(v_position_)
		, v_align(v_align_)
		, text(text_)
	{}

	int in;
	int out;
	float v_position;
	dcp::VAlign v_align;
	string text;
};


template <class T>
shared_ptr<dcp::CPL>
dcp_with_text(dcp::TextType type, path dir, vector<TestText> subs, optional<dcp::Key> key = boost::none, optional<string> key_id = boost::none)
{
	prepare_directory (dir);
	auto asset = make_shared<dcp::SMPTETextAsset>();
	asset->set_start_time (dcp::Time());
	for (auto i: subs) {
		add_test_subtitle (asset, i.in, i.out, i.v_position, i.v_align, i.text);
	}
	asset->set_language (dcp::LanguageTag("de-DE"));
	if (key && key_id) {
		asset->set_key(*key);
		asset->set_key_id(*key_id);
	}
	add_font(asset);
	asset->write (dir / "subs.mxf");

	auto reel_asset = make_shared<T>(type, asset, dcp::Fraction(24, 1), asset->intrinsic_duration(), 0);
	return write_dcp_with_single_asset (dir, reel_asset);
}


template <class T>
shared_ptr<dcp::CPL>
dcp_with_text_from_file(dcp::TextType type, path dir, boost::filesystem::path subs_xml)
{
	prepare_directory (dir);
	auto asset = make_shared<dcp::SMPTETextAsset>(subs_xml);
	asset->set_start_time (dcp::Time());
	asset->set_language (dcp::LanguageTag("de-DE"));

	auto subs_mxf = dir / "subs.mxf";
	asset->write (subs_mxf);

	/* The call to write() puts the asset into the DCP correctly but it will have
	 * XML re-written by our parser.  Overwrite the MXF using the given file's verbatim
	 * contents.
	 */
	ASDCP::TimedText::MXFWriter writer;
	ASDCP::WriterInfo writer_info;
	writer_info.LabelSetType = ASDCP::LS_MXF_SMPTE;
	unsigned int c;
	Kumu::hex2bin (asset->id().c_str(), writer_info.AssetUUID, Kumu::UUID_Length, &c);
	DCP_ASSERT (c == Kumu::UUID_Length);
	ASDCP::TimedText::TimedTextDescriptor descriptor;
	descriptor.ContainerDuration = asset->intrinsic_duration();
	Kumu::hex2bin (asset->xml_id()->c_str(), descriptor.AssetID, ASDCP::UUIDlen, &c);
	DCP_ASSERT (c == Kumu::UUID_Length);
	ASDCP::Result_t r = writer.OpenWrite (subs_mxf.string().c_str(), writer_info, descriptor, 16384);
	BOOST_REQUIRE (!ASDCP_FAILURE(r));
	r = writer.WriteTimedTextResource (dcp::file_to_string(subs_xml));
	BOOST_REQUIRE (!ASDCP_FAILURE(r));
	writer.Finalize ();

	auto reel_asset = make_shared<T>(type, asset, dcp::Fraction(24, 1), asset->intrinsic_duration(), 0);
	return write_dcp_with_single_asset (dir, reel_asset);
}


BOOST_AUTO_TEST_CASE (verify_invalid_subtitle_first_text_time)
{
	auto const dir = path("build/test/verify_invalid_subtitle_first_text_time");
	/* Just too early */
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, dir, {{ 4 * 24 - 1, 5 * 24 }});
	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});

}


BOOST_AUTO_TEST_CASE (verify_valid_subtitle_first_text_time)
{
	auto const dir = path("build/test/verify_valid_subtitle_first_text_time");
	/* Just late enough */
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, dir, {{ 4 * 24, 5 * 24 }});
	check_verify_result(
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_subtitle_first_text_time_on_second_reel)
{
	auto const dir = path("build/test/verify_valid_subtitle_first_text_time_on_second_reel");
	prepare_directory (dir);

	auto asset1 = make_shared<dcp::SMPTETextAsset>();
	asset1->set_start_time (dcp::Time());
	/* Just late enough */
	add_test_subtitle (asset1, 4 * 24, 5 * 24);
	asset1->set_language (dcp::LanguageTag("de-DE"));
	add_font(asset1);
	asset1->write (dir / "subs1.mxf");
	auto reel_asset1 = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, asset1, dcp::Fraction(24, 1), 5 * 24, 0);
	auto reel1 = make_shared<dcp::Reel>();
	reel1->add (reel_asset1);
	auto markers1 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), 5 * 24);
	markers1->set (dcp::Marker::FFOC, dcp::Time(1, 24, 24));
	reel1->add (markers1);

	auto asset2 = make_shared<dcp::SMPTETextAsset>();
	asset2->set_start_time (dcp::Time());
	add_font(asset2);
	/* This would be too early on first reel but should be OK on the second */
	add_test_subtitle (asset2, 3, 4 * 24);
	asset2->set_language (dcp::LanguageTag("de-DE"));
	asset2->write (dir / "subs2.mxf");
	auto reel_asset2 = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, asset2, dcp::Fraction(24, 1), 4 * 24, 0);
	auto reel2 = make_shared<dcp::Reel>();
	reel2->add (reel_asset2);
	auto markers2 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), 4 * 24);
	markers2->set (dcp::Marker::LFOC, dcp::Time(4 * 24 - 1, 24, 24));
	reel2->add (markers2);

	auto cpl = make_shared<dcp::CPL>("hello", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
	cpl->add (reel1);
	cpl->add (reel2);
	auto dcp = make_shared<dcp::DCP>(dir);
	dcp->add (cpl);
	dcp->set_annotation_text("hello");
	dcp->write_xml();

	check_verify_result(
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_subtitle_spacing)
{
	auto const dir = path("build/test/verify_invalid_subtitle_spacing");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::OPEN_SUBTITLE,
		dir,
		{
			{ 4 * 24,     5 * 24 },
			{ 5 * 24 + 1, 6 * 24 },
		});
	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_SPACING
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_subtitle_spacing)
{
	auto const dir = path("build/test/verify_valid_subtitle_spacing");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::OPEN_SUBTITLE,
		dir,
		{
			{ 4 * 24,      5 * 24 },
			{ 5 * 24 + 16, 8 * 24 },
		});

	check_verify_result(
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE(verify_invalid_subtitle_duration)
{
	auto const dir = path("build/test/verify_invalid_subtitle_duration");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, dir, {{ 4 * 24, 4 * 24 - 1 }});
	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_SUBTITLE_DURATION
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE(verify_invalid_subtitle_duration_bv21)
{
	auto const dir = path("build/test/verify_invalid_subtitle_duration_bv21");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, dir, {{ 4 * 24, 4 * 24 + 1 }});
	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_DURATION_BV21
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_subtitle_duration)
{
	auto const dir = path("build/test/verify_valid_subtitle_duration");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, dir, {{ 4 * 24, 4 * 24 + 17 }});

	check_verify_result(
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_subtitle_overlapping_reel_boundary)
{
	auto const dir = path("build/test/verify_subtitle_overlapping_reel_boundary");
	prepare_directory (dir);
	auto asset = make_shared<dcp::SMPTETextAsset>();
	asset->set_start_time (dcp::Time());
	add_test_subtitle (asset, 0, 4 * 24);
	add_font(asset);
	asset->set_language (dcp::LanguageTag("de-DE"));
	asset->write (dir / "subs.mxf");

	auto reel_asset = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, asset, dcp::Fraction(24, 1), 3 * 24, 0);
	auto cpl = write_dcp_with_single_asset (dir, reel_asset);
	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_TIMED_TEXT_DURATION , "72 96", boost::filesystem::canonical(asset->file().get())
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::SUBTITLE_OVERLAPS_REEL_BOUNDARY
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});

}


BOOST_AUTO_TEST_CASE (verify_invalid_subtitle_line_count1)
{
	auto const dir = path ("build/test/invalid_subtitle_line_count1");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::OPEN_SUBTITLE,
		dir,
		{
			{ 96, 200, 0.0, dcp::VAlign::CENTER, "We" },
			{ 96, 200, 0.1, dcp::VAlign::CENTER, "have" },
			{ 96, 200, 0.2, dcp::VAlign::CENTER, "four" },
			{ 96, 200, 0.3, dcp::VAlign::CENTER, "lines" }
		});
	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_LINE_COUNT
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_subtitle_line_count1)
{
	auto const dir = path ("build/test/verify_valid_subtitle_line_count1");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::OPEN_SUBTITLE,
		dir,
		{
			{ 96, 200, 0.0, dcp::VAlign::CENTER, "We" },
			{ 96, 200, 0.1, dcp::VAlign::CENTER, "have" },
			{ 96, 200, 0.2, dcp::VAlign::CENTER, "four" },
		});

	check_verify_result(
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_subtitle_line_count2)
{
	auto const dir = path ("build/test/verify_invalid_subtitle_line_count2");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::OPEN_SUBTITLE,
		dir,
		{
			{ 96, 300, 0.0, dcp::VAlign::CENTER, "We" },
			{ 96, 300, 0.1, dcp::VAlign::CENTER, "have" },
			{ 150, 180, 0.2, dcp::VAlign::CENTER, "four" },
			{ 150, 180, 0.3, dcp::VAlign::CENTER, "lines" }
		});
	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_LINE_COUNT
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_subtitle_line_count2)
{
	auto const dir = path ("build/test/verify_valid_subtitle_line_count2");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::OPEN_SUBTITLE,
		dir,
		{
			{ 96, 300, 0.0, dcp::VAlign::CENTER, "We" },
			{ 96, 300, 0.1, dcp::VAlign::CENTER, "have" },
			{ 150, 180, 0.2, dcp::VAlign::CENTER, "four" },
			{ 190, 250, 0.3, dcp::VAlign::CENTER, "lines" }
		});

	check_verify_result(
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_subtitle_line_length1)
{
	auto const dir = path ("build/test/verify_invalid_subtitle_line_length1");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::OPEN_SUBTITLE,
		dir,
		{
			{ 96, 300, 0.0, dcp::VAlign::CENTER, "012345678901234567890123456789012345678901234567890123" }
		});
	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::NEARLY_INVALID_SUBTITLE_LINE_LENGTH
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_subtitle_line_length2)
{
	auto const dir = path ("build/test/verify_invalid_subtitle_line_length2");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::OPEN_SUBTITLE,
		dir,
		{
			{ 96, 300, 0.0, dcp::VAlign::CENTER, "012345678901234567890123456789012345678901234567890123456789012345678901234567890" }
		});
	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_LINE_LENGTH
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_closed_caption_line_count1)
{
	auto const dir = path ("build/test/verify_valid_closed_caption_line_count1");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::CLOSED_CAPTION,
		dir,
		{
			{ 96, 200, 0.0, dcp::VAlign::CENTER, "We" },
			{ 96, 200, 0.1, dcp::VAlign::CENTER, "have" },
			{ 96, 200, 0.2, dcp::VAlign::CENTER, "four" },
			{ 96, 200, 0.3, dcp::VAlign::CENTER, "lines" }
		});
	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_CLOSED_CAPTION_LINE_COUNT
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_closed_caption_line_count2)
{
	auto const dir = path ("build/test/verify_valid_closed_caption_line_count2");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::OPEN_SUBTITLE,
		dir,
		{
			{ 96, 200, 0.0, dcp::VAlign::CENTER, "We" },
			{ 96, 200, 0.1, dcp::VAlign::CENTER, "have" },
			{ 96, 200, 0.2, dcp::VAlign::CENTER, "four" },
		});

	check_verify_result(
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_closed_caption_line_count3)
{
	auto const dir = path ("build/test/verify_invalid_closed_caption_line_count3");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::CLOSED_CAPTION,
		dir,
		{
			{ 96, 300, 0.0, dcp::VAlign::CENTER, "We" },
			{ 96, 300, 0.1, dcp::VAlign::CENTER, "have" },
			{ 150, 180, 0.2, dcp::VAlign::CENTER, "four" },
			{ 150, 180, 0.3, dcp::VAlign::CENTER, "lines" }
		});
	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_CLOSED_CAPTION_LINE_COUNT
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_closed_caption_line_count4)
{
	auto const dir = path ("build/test/verify_valid_closed_caption_line_count4");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::CLOSED_CAPTION,
		dir,
		{
			{ 96, 300, 0.0, dcp::VAlign::CENTER, "We" },
			{ 96, 300, 0.1, dcp::VAlign::CENTER, "have" },
			{ 150, 180, 0.2, dcp::VAlign::CENTER, "four" },
			{ 190, 250, 0.3, dcp::VAlign::CENTER, "lines" }
		});

	check_verify_result(
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_valid_closed_caption_line_length)
{
	auto const dir = path ("build/test/verify_valid_closed_caption_line_length");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::CLOSED_CAPTION,
		dir,
		{
			{ 96, 300, 0.0, dcp::VAlign::CENTER, "01234567890123456789012345678901" }
		});

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_closed_caption_line_length)
{
	auto const dir = path ("build/test/verify_invalid_closed_caption_line_length");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::CLOSED_CAPTION,
		dir,
		{
			{ 96, 300, 0.0, dcp::VAlign::CENTER, "0123456789012345678901234567890123" }
		});
	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_CLOSED_CAPTION_LINE_LENGTH
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_mismatched_closed_caption_valign1)
{
	auto const dir = path ("build/test/verify_mismatched_closed_caption_valign1");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::CLOSED_CAPTION,
		dir,
		{
			{ 96, 300, 0.0, dcp::VAlign::TOP, "This" },
			{ 96, 300, 0.1, dcp::VAlign::TOP, "is" },
			{ 96, 300, 0.2, dcp::VAlign::TOP, "fine" },
		});
	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_mismatched_closed_caption_valign2)
{
	auto const dir = path ("build/test/verify_mismatched_closed_caption_valign2");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::CLOSED_CAPTION,
		dir,
		{
			{ 96, 300, 0.0, dcp::VAlign::TOP, "This" },
			{ 96, 300, 0.1, dcp::VAlign::TOP, "is" },
			{ 96, 300, 0.2, dcp::VAlign::CENTER, "not fine" },
		});
	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CLOSED_CAPTION_VALIGN
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_incorrect_closed_caption_ordering1)
{
	auto const dir = path ("build/test/verify_invalid_incorrect_closed_caption_ordering1");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::CLOSED_CAPTION,
		dir,
		{
			{ 96, 300, 0.0, dcp::VAlign::TOP, "This" },
			{ 96, 300, 0.1, dcp::VAlign::TOP, "is" },
			{ 96, 300, 0.2, dcp::VAlign::TOP, "fine" },
		});

	check_verify_result(
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_incorrect_closed_caption_ordering2)
{
	auto const dir = path ("build/test/verify_invalid_incorrect_closed_caption_ordering2");
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset> (
		dcp::TextType::CLOSED_CAPTION,
		dir,
		{
			{ 96, 300, 0.2, dcp::VAlign::BOTTOM, "This" },
			{ 96, 300, 0.1, dcp::VAlign::BOTTOM, "is" },
			{ 96, 300, 0.0, dcp::VAlign::BOTTOM, "also fine" },
		});

	check_verify_result(
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_incorrect_closed_caption_ordering3)
{
	auto const dir = path ("build/test/verify_incorrect_closed_caption_ordering3");
	auto cpl = dcp_with_text_from_file<dcp::ReelSMPTETextAsset>(dcp::TextType::CLOSED_CAPTION, dir, "test/data/verify_incorrect_closed_caption_ordering3.xml");
	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INCORRECT_CLOSED_CAPTION_ORDERING
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_incorrect_closed_caption_ordering4)
{
	auto const dir = path ("build/test/verify_incorrect_closed_caption_ordering4");
	auto cpl = dcp_with_text_from_file<dcp::ReelSMPTETextAsset>(dcp::TextType::CLOSED_CAPTION, dir, "test/data/verify_incorrect_closed_caption_ordering4.xml");

	check_verify_result(
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
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
	auto sound = simple_sound (dir, "foo", dcp::MXFMetadata(), "de-DE", 24, 96000, boost::none);
	auto reel_sound = make_shared<dcp::ReelSoundAsset>(sound, 0);
	reel->add (reel_sound);
	reel->add (simple_markers());
	auto cpl = make_shared<dcp::CPL>("hello", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
	cpl->add (reel);
	auto dcp = make_shared<dcp::DCP>(dir);
	dcp->add (cpl);
	dcp->set_annotation_text("hello");
	dcp->write_xml();

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "videofoo.mxf"), cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "videofoo.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_SOUND_FRAME_RATE, string("96000"), canonical(dir / "audiofoo.mxf")
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_missing_cpl_annotation_text)
{
	path const dir("build/test/verify_missing_cpl_annotation_text");
	auto dcp = make_simple (dir);
	dcp->write_xml();

	BOOST_REQUIRE_EQUAL (dcp->cpls().size(), 1U);

	auto const cpl = dcp->cpls()[0];

	HashCalculator calc(cpl->file().get());

	{
		BOOST_REQUIRE (cpl->file());
		Editor e(cpl->file().get());
		e.replace("<AnnotationText>A Test DCP</AnnotationText>", "");
	}

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_ANNOTATION_TEXT, canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, canonical(cpl->file().get())
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash())
		});
}


BOOST_AUTO_TEST_CASE (verify_mismatched_cpl_annotation_text)
{
	path const dir("build/test/verify_mismatched_cpl_annotation_text");
	auto dcp = make_simple (dir);
	dcp->write_xml();

	BOOST_REQUIRE_EQUAL (dcp->cpls().size(), 1U);
	auto const cpl = dcp->cpls()[0];

	HashCalculator calc(cpl->file().get());

	{
		BOOST_REQUIRE (cpl->file());
		Editor e(cpl->file().get());
		e.replace("<AnnotationText>A Test DCP</AnnotationText>", "<AnnotationText>A Test DCP 1</AnnotationText>");
	}

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISMATCHED_CPL_ANNOTATION_TEXT, canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, canonical(cpl->file().get())
				).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_mismatched_asset_duration)
{
	path const dir("build/test/verify_mismatched_asset_duration");
	prepare_directory (dir);
	shared_ptr<dcp::DCP> dcp (new dcp::DCP(dir));
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);

	shared_ptr<dcp::MonoJ2KPictureAsset> mp = simple_picture (dir, "", 24);
	shared_ptr<dcp::SoundAsset> ms = simple_sound (dir, "", dcp::MXFMetadata(), "en-US", 25);

	auto reel = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(mp, 0),
		make_shared<dcp::ReelSoundAsset>(ms, 0)
		);

	reel->add (simple_markers());
	cpl->add (reel);

	dcp->add (cpl);
	dcp->set_annotation_text("A Test DCP");
	dcp->write_xml();

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_ASSET_DURATION
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, canonical(cpl->file().get())
				).set_cpl_id(cpl->id())
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

	auto subs = make_shared<dcp::SMPTETextAsset>();
	subs->set_language (dcp::LanguageTag("de-DE"));
	subs->set_start_time (dcp::Time());
	subs->add(simple_text());
	add_font(subs);
	subs->write (dir / "subs.mxf");
	auto reel_subs = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, subs, dcp::Fraction(24, 1), reel_length, 0);

	auto reel1 = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "1", reel_length), 0),
		make_shared<dcp::ReelSoundAsset>(simple_sound(dir, "1", dcp::MXFMetadata(), "en-US", reel_length), 0)
		);

	if (add_to_reel1) {
		reel1->add (make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, subs, dcp::Fraction(24, 1), reel_length, 0));
	}

	auto markers1 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), reel_length);
	markers1->set (dcp::Marker::FFOC, dcp::Time(1, 24, 24));
	reel1->add (markers1);

	cpl->add (reel1);

	auto reel2 = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "2", reel_length), 0),
		make_shared<dcp::ReelSoundAsset>(simple_sound(dir, "2", dcp::MXFMetadata(), "en-US", reel_length), 0)
		);

	if (add_to_reel2) {
		reel2->add (make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, subs, dcp::Fraction(24, 1), reel_length, 0));
	}

	auto markers2 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), reel_length);
	markers2->set (dcp::Marker::LFOC, dcp::Time(reel_length - 1, 24, 24));
	reel2->add (markers2);

	cpl->add (reel2);

	dcp->add (cpl);
	dcp->set_annotation_text("A Test DCP");
	dcp->write_xml();

	return cpl;
}


BOOST_AUTO_TEST_CASE (verify_missing_main_subtitle_from_some_reels)
{
	{
		path dir ("build/test/missing_main_subtitle_from_some_reels");
		auto cpl = verify_subtitles_must_be_in_all_reels_check (dir, true, false);
		check_verify_result (
			{ dir },
			{},
			{
				ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
				ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
				ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
				ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
				ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
				ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video1.mxf"), cpl),
				ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video2.mxf"), cpl),
				ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
				ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video1.mxf"), cpl),
				ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video2.mxf"), cpl),
				dcp::VerificationNote(
					dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_MAIN_SUBTITLE_FROM_SOME_REELS
					).set_cpl_id(cpl->id()),
				dcp::VerificationNote(
					dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
					).set_cpl_id(cpl->id())
			});

	}

	{
		path dir ("build/test/verify_subtitles_must_be_in_all_reels2");
		auto cpl = verify_subtitles_must_be_in_all_reels_check (dir, true, true);
		check_verify_result(
			{dir},
			{},
			{
				ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
				ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
				ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
				ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
				ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
				ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video1.mxf"), cpl),
				ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video2.mxf"), cpl),
				ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
				ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video1.mxf"), cpl),
				ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video2.mxf"), cpl),
				dcp::VerificationNote(
					dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
					).set_cpl_id(cpl->id())
			});
	}

	{
		path dir ("build/test/verify_subtitles_must_be_in_all_reels1");
		auto cpl = verify_subtitles_must_be_in_all_reels_check (dir, false, false);
		check_verify_result(
			{dir},
			{},
			{
				ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
				ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
				ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
				ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
				ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
				ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video1.mxf"), cpl),
				ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video2.mxf"), cpl),
				ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
				ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video1.mxf"), cpl),
				ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video2.mxf"), cpl),
				dcp::VerificationNote(
					dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
					).set_cpl_id(cpl->id())
			});
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

	auto subs = make_shared<dcp::SMPTETextAsset>();
	subs->set_language (dcp::LanguageTag("de-DE"));
	subs->set_start_time (dcp::Time());
	subs->add(simple_text());
	add_font(subs);
	subs->write (dir / "subs.mxf");

	auto reel1 = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "1", reel_length), 0),
		make_shared<dcp::ReelSoundAsset>(simple_sound(dir, "1", dcp::MXFMetadata(), "en-US", reel_length), 0)
		);

	for (int i = 0; i < caps_in_reel1; ++i) {
		reel1->add (make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::CLOSED_CAPTION, subs, dcp::Fraction(24, 1), reel_length, 0));
	}

	auto markers1 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), reel_length);
	markers1->set (dcp::Marker::FFOC, dcp::Time(1, 24, 24));
	reel1->add (markers1);

	cpl->add (reel1);

	auto reel2 = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "2", reel_length), 0),
		make_shared<dcp::ReelSoundAsset>(simple_sound(dir, "2", dcp::MXFMetadata(), "en-US", reel_length), 0)
		);

	for (int i = 0; i < caps_in_reel2; ++i) {
		reel2->add (make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::CLOSED_CAPTION, subs, dcp::Fraction(24, 1), reel_length, 0));
	}

	auto markers2 = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), reel_length);
	markers2->set (dcp::Marker::LFOC, dcp::Time(reel_length - 1, 24, 24));
	reel2->add (markers2);

	cpl->add (reel2);

	dcp->add (cpl);
	dcp->set_annotation_text("A Test DCP");
	dcp->write_xml();

	return cpl;
}


BOOST_AUTO_TEST_CASE (verify_mismatched_closed_caption_asset_counts)
{
	{
		path dir ("build/test/mismatched_closed_caption_asset_counts");
		auto cpl = verify_closed_captions_must_be_in_all_reels_check (dir, 3, 4);
		check_verify_result (
			{dir},
			{},
			{
				ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
				ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video1.mxf"), cpl),
				ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video2.mxf"), cpl),
				ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
				ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
				ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video1.mxf"), cpl),
				ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video2.mxf"), cpl),
				ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
				ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
				ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
				dcp::VerificationNote(
					dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_CLOSED_CAPTION_ASSET_COUNTS
					).set_cpl_id(cpl->id()),
				dcp::VerificationNote(
					dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
					).set_cpl_id(cpl->id())
			});
	}

	{
		path dir ("build/test/verify_closed_captions_must_be_in_all_reels2");
		auto cpl = verify_closed_captions_must_be_in_all_reels_check (dir, 4, 4);
		check_verify_result(
			{dir},
			{},
			{
				ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
				ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video1.mxf"), cpl),
				ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video2.mxf"), cpl),
				ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
				ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
				ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
				ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
				ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video1.mxf"), cpl),
				ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video2.mxf"), cpl),
				ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
				dcp::VerificationNote(
					dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
					).set_cpl_id(cpl->id())
			});
	}

	{
		path dir ("build/test/verify_closed_captions_must_be_in_all_reels3");
		auto cpl = verify_closed_captions_must_be_in_all_reels_check (dir, 0, 0);
		check_verify_result(
			{dir},
			{},
			{
				ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
				ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video1.mxf"), cpl),
				ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video2.mxf"), cpl),
				ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
				ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
				ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
				ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
				ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video1.mxf"), cpl),
				ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video2.mxf"), cpl),
				ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
				dcp::VerificationNote(
					dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
					).set_cpl_id(cpl->id())
			});
	}
}


template <class T>
void
verify_text_entry_point_check(dcp::TextType type, path dir, dcp::VerificationNote::Code code, boost::function<void (shared_ptr<T>)> adjust)
{
	prepare_directory (dir);
	auto dcp = make_shared<dcp::DCP>(dir);
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);

	auto constexpr reel_length = 192;

	auto subs = make_shared<dcp::SMPTETextAsset>();
	subs->set_language (dcp::LanguageTag("de-DE"));
	subs->set_start_time (dcp::Time());
	subs->add(simple_text());
	add_font(subs);
	subs->write (dir / "subs.mxf");
	auto reel_text = make_shared<T>(type, subs, dcp::Fraction(24, 1), reel_length, 0);
	adjust (reel_text);

	auto reel = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(simple_picture(dir, "", reel_length), 0),
		make_shared<dcp::ReelSoundAsset>(simple_sound(dir, "", dcp::MXFMetadata(), "en-US", reel_length), 0)
		);

	reel->add (reel_text);

	reel->add (simple_markers(reel_length));

	cpl->add (reel);

	dcp->add (cpl);
	dcp->set_annotation_text("A Test DCP");
	dcp->write_xml();

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, code, subs->id()
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_text_entry_point)
{
	verify_text_entry_point_check<dcp::ReelSMPTETextAsset>(
		dcp::TextType::OPEN_SUBTITLE,
		"build/test/verify_subtitle_entry_point_must_be_present",
		dcp::VerificationNote::Code::MISSING_SUBTITLE_ENTRY_POINT,
		[](shared_ptr<dcp::ReelSMPTETextAsset> asset) {
			asset->unset_entry_point ();
			}
		);

	verify_text_entry_point_check<dcp::ReelSMPTETextAsset>(
		dcp::TextType::OPEN_SUBTITLE,
		"build/test/verify_subtitle_entry_point_must_be_zero",
		dcp::VerificationNote::Code::INCORRECT_SUBTITLE_ENTRY_POINT,
		[](shared_ptr<dcp::ReelSMPTETextAsset> asset) {
			asset->set_entry_point (4);
			}
		);

	verify_text_entry_point_check<dcp::ReelSMPTETextAsset>(
		dcp::TextType::CLOSED_CAPTION,
		"build/test/verify_closed_caption_entry_point_must_be_present",
		dcp::VerificationNote::Code::MISSING_CLOSED_CAPTION_ENTRY_POINT,
		[](shared_ptr<dcp::ReelSMPTETextAsset> asset) {
			asset->unset_entry_point ();
			}
		);

	verify_text_entry_point_check<dcp::ReelSMPTETextAsset>(
		dcp::TextType::CLOSED_CAPTION,
		"build/test/verify_closed_caption_entry_point_must_be_zero",
		dcp::VerificationNote::Code::INCORRECT_CLOSED_CAPTION_ENTRY_POINT,
		[](shared_ptr<dcp::ReelSMPTETextAsset> asset) {
			asset->set_entry_point (9);
			}
		);
}


BOOST_AUTO_TEST_CASE (verify_missing_hash)
{
	RNGFixer fix;

	path const dir("build/test/verify_missing_hash");
	auto dcp = make_simple (dir);
	dcp->write_xml();

	BOOST_REQUIRE_EQUAL (dcp->cpls().size(), 1U);
	auto const cpl = dcp->cpls()[0];
	BOOST_REQUIRE_EQUAL (cpl->reels().size(), 1U);
	BOOST_REQUIRE (cpl->reels()[0]->main_picture());
	auto asset_id = cpl->reels()[0]->main_picture()->id();

	HashCalculator calc(cpl->file().get());

	{
		BOOST_REQUIRE (cpl->file());
		Editor e(cpl->file().get());
		e.delete_first_line_containing("<Hash>");
	}

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->file().get()
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_HASH, asset_id
				).set_cpl_id(cpl->id())
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
	auto cpl = dcp->cpls()[0];
	cpl->set_content_kind(dcp::ContentKind::FEATURE);
	auto markers_asset = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), 24);
	for (auto const& i: markers) {
		markers_asset->set (i.first, i.second);
	}
	cpl->reels()[0]->add(markers_asset);
	dcp->write_xml();

	for (auto& note: test_notes) {
		note.set_cpl_id(cpl->id());
	}

	test_notes.push_back(ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl));
	test_notes.push_back(ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl));
	test_notes.push_back(ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl));
	test_notes.push_back(ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl));
	test_notes.push_back(
		dcp::VerificationNote(
			dcp::VerificationNote::Type::OK,
			dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
			string{"1998x1080"},
			cpl->file().get()
			).set_cpl_id(cpl->id())
		);
	test_notes.push_back(ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"feature"}, cpl));
	test_notes.push_back(ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl));
	test_notes.push_back(ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl));
	test_notes.push_back(ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl));

	check_verify_result({dir}, {}, test_notes);
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
	dcp->write_xml();

	check_verify_result(
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA_VERSION_NUMBER, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_missing_extension_metadata1)
{
	path dir = "build/test/verify_missing_extension_metadata1";
	auto dcp = make_simple (dir);
	dcp->write_xml();

	BOOST_REQUIRE_EQUAL (dcp->cpls().size(), 1U);
	auto cpl = dcp->cpls()[0];

	HashCalculator calc(cpl->file().get());

	{
		Editor e (cpl->file().get());
		e.delete_lines ("<meta:ExtensionMetadataList>", "</meta:ExtensionMetadataList>");
	}

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->file().get()
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_EXTENSION_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_missing_extension_metadata2)
{
	path dir = "build/test/verify_missing_extension_metadata2";
	auto dcp = make_simple (dir);
	dcp->write_xml();

	auto cpl = dcp->cpls()[0];

	HashCalculator calc(cpl->file().get());

	{
		Editor e (cpl->file().get());
		e.delete_lines ("<meta:ExtensionMetadata scope=\"http://isdcf.com/ns/cplmd/app\">", "</meta:ExtensionMetadata>");
	}

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->file().get()
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_EXTENSION_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_cpl_extension_metadata3)
{
	path dir = "build/test/verify_invalid_xml_cpl_extension_metadata3";
	auto dcp = make_simple (dir);
	dcp->write_xml();

	auto const cpl = dcp->cpls()[0];

	HashCalculator calc(cpl->file().get());

	{
		Editor e (cpl->file().get());
		e.replace ("<meta:Name>A", "<meta:NameX>A");
		e.replace ("n</meta:Name>", "n</meta:NameX>");
	}

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'meta:NameX'"), cpl->file().get(), 70
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("element 'meta:NameX' is not allowed for content model '(Name,PropertyList?,)'"), cpl->file().get(), 77).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->file().get()
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_extension_metadata1)
{
	path dir = "build/test/verify_invalid_extension_metadata1";
	auto dcp = make_simple (dir);
	dcp->write_xml();

	auto cpl = dcp->cpls()[0];

	HashCalculator calc(cpl->file().get());

	{
		Editor e (cpl->file().get());
		e.replace ("Application", "Fred");
	}

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->file().get()
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_EXTENSION_METADATA, string("<Name> should be 'Application'"), cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_extension_metadata2)
{
	path dir = "build/test/verify_invalid_extension_metadata2";
	auto dcp = make_simple (dir);
	dcp->write_xml();

	auto cpl = dcp->cpls()[0];

	HashCalculator calc(cpl->file().get());

	{
		Editor e (cpl->file().get());
		e.replace ("DCP Constraints Profile", "Fred");
	}

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->file().get()
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_EXTENSION_METADATA, string("No correctly-formed DCP Constraints Profile found"), cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_cpl_extension_metadata6)
{
	path dir = "build/test/verify_invalid_xml_cpl_extension_metadata6";
	auto dcp = make_simple (dir);
	dcp->write_xml();

	auto const cpl = dcp->cpls()[0];

	HashCalculator calc(cpl->file().get());

	{
		Editor e (cpl->file().get());
		e.replace ("<meta:Value>", "<meta:ValueX>");
		e.replace ("</meta:Value>", "</meta:ValueX>");
	}

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'meta:ValueX'"), cpl->file().get(), 74
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("element 'meta:ValueX' is not allowed for content model '(Name,Value)'"), cpl->file().get(), 75
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->file().get()
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_EXTENSION_METADATA, string("No correctly-formed DCP Constraints Profile found"), cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_cpl_extension_metadata7)
{
	path dir = "build/test/verify_invalid_xml_cpl_extension_metadata7";
	auto dcp = make_simple (dir);
	dcp->write_xml();

	auto const cpl = dcp->cpls()[0];

	HashCalculator calc(cpl->file().get());

	{
		Editor e (cpl->file().get());
		e.replace ("SMPTE-RDD-52:2020-Bv2.1", "Fred");
	}

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->file().get()
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_EXTENSION_METADATA, string("No correctly-formed DCP Constraints Profile found"), cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_cpl_extension_metadata8)
{
	path dir = "build/test/verify_invalid_xml_cpl_extension_metadata8";
	auto dcp = make_simple (dir);
	dcp->write_xml();

	auto const cpl = dcp->cpls()[0];

	HashCalculator calc(cpl->file().get());

	{
		Editor e (cpl->file().get());
		e.replace ("<meta:Property>", "<meta:PropertyX>");
		e.replace ("</meta:Property>", "</meta:PropertyX>");
	}

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'meta:PropertyX'"), cpl->file().get(), 72
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("element 'meta:PropertyX' is not allowed for content model '(Property+)'"), cpl->file().get(), 76).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->file().get()
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_EXTENSION_METADATA, string("No correctly-formed DCP Constraints Profile found"), cpl->file().get()
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_invalid_xml_cpl_extension_metadata9)
{
	path dir = "build/test/verify_invalid_xml_cpl_extension_metadata9";
	auto dcp = make_simple (dir);
	dcp->write_xml();

	auto const cpl = dcp->cpls()[0];

	HashCalculator calc(cpl->file().get());

	{
		Editor e (cpl->file().get());
		e.replace ("<meta:PropertyList>", "<meta:PropertyListX>");
		e.replace ("</meta:PropertyList>", "</meta:PropertyListX>");
	}

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("no declaration found for element 'meta:PropertyListX'"), cpl->file().get(), 71
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_XML, string("element 'meta:PropertyListX' is not allowed for content model '(Name,PropertyList?,)'"), cpl->file().get(), 77
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, cpl->file().get()
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
		});
}



BOOST_AUTO_TEST_CASE (verify_unsigned_cpl_with_encrypted_content)
{
	path const dir = "build/test/verify_unsigned_cpl_with_encrypted_content";
	prepare_directory (dir);
	for (auto i: directory_iterator("test/ref/DCP/encryption_test")) {
		copy_file (i.path(), dir / i.path().filename());
	}

	path const pkl = dir / ( "pkl_" + encryption_test_pkl_id() + ".xml");
	path const cpl_path = dir / ( "cpl_" + encryption_test_cpl_id() + ".xml");

	HashCalculator calc(cpl_path);

	{
		Editor e(cpl_path);
		e.delete_lines ("<dsig:Signature", "</dsig:Signature>");
	}

	auto cpl = std::make_shared<dcp::CPL>(cpl_path);

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::ALL_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"feature"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, canonical(cpl_path)
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_PKL_ANNOTATION_TEXT_WITH_CPL, encryption_test_pkl_id(), canonical(pkl)
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_FFEC_IN_FEATURE
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_FFMC_IN_FEATURE
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSING_FFOC
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSING_LFOC
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, canonical(cpl_path)
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::UNSIGNED_CPL_WITH_ENCRYPTED_CONTENT, canonical(cpl_path)
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_unsigned_pkl_with_encrypted_content)
{
	path dir = "build/test/unsigned_pkl_with_encrypted_content";
	prepare_directory (dir);
	for (auto i: directory_iterator("test/ref/DCP/encryption_test")) {
		copy_file (i.path(), dir / i.path().filename());
	}

	path const cpl_path = dir / ("cpl_" + encryption_test_cpl_id() + ".xml");
	path const pkl = dir / ("pkl_" + encryption_test_pkl_id() + ".xml");
	{
		Editor e (pkl);
		e.delete_lines ("<dsig:Signature", "</dsig:Signature>");
	}

	auto cpl = std::make_shared<dcp::CPL>(cpl_path);

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::ALL_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"feature"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_PKL_ANNOTATION_TEXT_WITH_CPL, encryption_test_pkl_id(), canonical(pkl)
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_FFEC_IN_FEATURE
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_FFMC_IN_FEATURE
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSING_FFOC
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSING_LFOC
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, canonical(cpl_path)
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::UNSIGNED_PKL_WITH_ENCRYPTED_CONTENT, encryption_test_pkl_id(), canonical(pkl)
				)
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
		Editor e (dir / dcp_test1_pkl());
		e.delete_lines ("<dsig:Signature", "</dsig:Signature>");
	}

	auto cpl = make_shared<dcp::CPL>(find_cpl(dir));

	check_verify_result(
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
		});
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

	auto mp = make_shared<dcp::MonoJ2KPictureAsset>(dcp::Fraction (24, 1), dcp::Standard::SMPTE);
	mp->set_key (key);

	auto writer = mp->start_write(dir / "video.mxf", dcp::Behaviour::MAKE_NEW);
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
	cpl->set_main_sound_configuration(dcp::MainSoundConfiguration("51/L,C,R,LFE,-,-"));
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));
	cpl->set_version_number (1);

	d.add (cpl);

	d.set_issuer("OpenDCP 0.0.25");
	d.set_creator("OpenDCP 0.0.25");
	d.set_issue_date("2012-07-17T04:45:18+00:00");
	d.set_annotation_text("A Test DCP");
	d.write_xml(signer);

	check_verify_result (
		{dir},
		{},
		{
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1440x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::PARTIALLY_ENCRYPTED
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE (verify_jpeg2000_codestream_2k)
{
	vector<dcp::VerificationNote> notes;
	dcp::MonoJ2KPictureAsset picture (find_file(private_test / "data" / "JourneyToJah_TLR-1_F_EN-DE-FR_CH_51_2K_LOK_20140225_DGL_SMPTE_OV", "j2c.mxf"));
	auto reader = picture.start_read ();
	auto frame = reader->get_frame (0);
	verify_j2k(frame, 0, 0, 24, notes);
	BOOST_CHECK(notes.empty());
}


BOOST_AUTO_TEST_CASE (verify_jpeg2000_codestream_4k)
{
	vector<dcp::VerificationNote> notes;
	dcp::MonoJ2KPictureAsset picture (find_file(private_test / "data" / "sul", "TLR"));
	auto reader = picture.start_read ();
	auto frame = reader->get_frame (0);
	verify_j2k(frame, 0, 0, 24, notes);
	BOOST_CHECK(notes.empty());
}


BOOST_AUTO_TEST_CASE (verify_jpeg2000_codestream_libdcp)
{
	boost::filesystem::path dir = "build/test/verify_jpeg2000_codestream_libdcp";
	prepare_directory (dir);
	auto dcp = make_simple (dir);
	dcp->write_xml ();
	vector<dcp::VerificationNote> notes;
	dcp::MonoJ2KPictureAsset picture (find_file(dir, "video"));
	auto reader = picture.start_read ();
	auto frame = reader->get_frame (0);
	verify_j2k(frame, 0, 0, 24, notes);
	BOOST_CHECK(notes.empty());
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
	auto r = writer.OpenWrite(subs_mxf.string().c_str(), writer_info, descriptor, 4096);
	BOOST_REQUIRE (ASDCP_SUCCESS(r));
	writer.WriteTimedTextResource (dcp::String::compose(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SubtitleReel xmlns=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\">"
		"<Id>urn:uuid:%1</Id>"
		"<ContentTitleText>Content</ContentTitleText>"
		"<AnnotationText>Annotation</AnnotationText>"
		"<IssueDate>2018-10-02T12:25:14</IssueDate>"
		"<ReelNumber>1</ReelNumber>"
		"<Language>en-US</Language>"
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
		"</SubtitleReel>",
		xml_id).c_str());

	writer.Finalize();

	auto subs_asset = make_shared<dcp::SMPTETextAsset>(subs_mxf);
	auto subs_reel = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, subs_asset, dcp::Fraction(24, 1), 240, 0);

	auto cpl = write_dcp_with_single_asset (dir, subs_reel);

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_TIMED_TEXT_DURATION , "240 0", boost::filesystem::canonical(subs_mxf)
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_TIMED_TEXT_RESOURCE_ID
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id())
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
	auto r = writer.OpenWrite(subs_mxf.string().c_str(), writer_info, descriptor, 4096);
	BOOST_REQUIRE (ASDCP_SUCCESS(r));
	writer.WriteTimedTextResource (dcp::String::compose(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SubtitleReel xmlns=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\">"
		"<Id>urn:uuid:%1</Id>"
		"<ContentTitleText>Content</ContentTitleText>"
		"<AnnotationText>Annotation</AnnotationText>"
		"<IssueDate>2018-10-02T12:25:14+02:00</IssueDate>"
		"<ReelNumber>1</ReelNumber>"
		"<Language>en-US</Language>"
		"<EditRate>25 1</EditRate>"
		"<TimeCodeRate>25</TimeCodeRate>"
		"<StartTime>00:00:00:00</StartTime>"
		"<LoadFont ID=\"font\">urn:uuid:0ce6e0ba-58b9-4344-8929-4d9c959c2d55</LoadFont>"
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

	auto subs_asset = make_shared<dcp::SMPTETextAsset>(subs_mxf);
	auto subs_reel = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, subs_asset, dcp::Fraction(24, 1), 240, 0);

	auto cpl = write_dcp_with_single_asset (dir, subs_reel);

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISMATCHED_TIMED_TEXT_DURATION , "240 0", boost::filesystem::canonical(subs_mxf)
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INCORRECT_TIMED_TEXT_ASSET_ID
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get()
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_ISSUE_DATE, string{"2018-10-02T12:25:14+02:00"}
				).set_cpl_id(cpl->id())
		});
}


/** Check a DCP with a 3D asset marked as 2D */
BOOST_AUTO_TEST_CASE (verify_threed_marked_as_twod)
{
	auto const path = private_test / "data" / "xm";

	auto cpl = std::make_shared<dcp::CPL>(find_prefix(path, "CPL_"));
	BOOST_REQUIRE(cpl);

	check_verify_result (
		{ path },
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(path / "0d6f57e6-adac-4e1d-bfbe-d162bf13e2cd_j2c.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(path / "0d6f57e6-adac-4e1d-bfbe-d162bf13e2cd_j2c.mxf"), cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING,
				dcp::VerificationNote::Code::THREED_ASSET_MARKED_AS_TWOD, boost::filesystem::canonical(find_file(path, "j2c"))
				),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR,
				dcp::VerificationNote::Code::INVALID_STANDARD
				)
		});

}


BOOST_AUTO_TEST_CASE (verify_unexpected_things_in_main_markers)
{
	path dir = "build/test/verify_unexpected_things_in_main_markers";
	prepare_directory (dir);
	auto dcp = make_simple (dir, 1, 24);
	dcp->write_xml();

	HashCalculator calc(find_cpl(dir));

	{
		Editor e (find_cpl(dir));
		e.insert(
			"          <IntrinsicDuration>24</IntrinsicDuration>",
			"<EntryPoint>0</EntryPoint><Duration>24</Duration>"
			);
	}

	auto cpl = make_shared<dcp::CPL>(find_cpl(dir));

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, canonical(find_cpl(dir))
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::UNEXPECTED_ENTRY_POINT
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::UNEXPECTED_DURATION
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE(verify_invalid_content_kind)
{
	path dir = "build/test/verify_invalid_content_kind";
	prepare_directory (dir);
	auto dcp = make_simple (dir, 1, 24);
	dcp->write_xml();

	HashCalculator calc(find_cpl(dir));

	{
		Editor e(find_cpl(dir));
		e.replace("trailer", "trip");
	}

	auto cpl = std::make_shared<dcp::CPL>(find_cpl(dir));

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, canonical(find_cpl(dir))
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_CONTENT_KIND, string("trip")
				).set_cpl_id(cpl->id()),
		});

}


BOOST_AUTO_TEST_CASE(verify_valid_content_kind)
{
	path dir = "build/test/verify_valid_content_kind";
	prepare_directory (dir);
	auto dcp = make_simple (dir, 1, 24);
	dcp->write_xml();

	HashCalculator calc(find_cpl(dir));

	{
		Editor e(find_cpl(dir));
		e.replace("<ContentKind>trailer</ContentKind>", "<ContentKind scope=\"http://bobs.contents/\">trip</ContentKind>");
	}

	auto cpl = std::make_shared<dcp::CPL>(find_cpl(dir));

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, canonical(find_cpl(dir))
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
		});
}


BOOST_AUTO_TEST_CASE(verify_invalid_main_picture_active_area_1)
{
	path dir = "build/test/verify_invalid_main_picture_active_area_1";
	prepare_directory(dir);
	auto dcp = make_simple(dir, 1, 24);
	dcp->write_xml();

	auto constexpr area = "<meta:MainPictureActiveArea>";

	HashCalculator calc(find_cpl(dir));

	{
		Editor e(find_cpl(dir));
		e.delete_lines_after(area, 2);
		e.insert(area, "<meta:Height>4080</meta:Height>");
		e.insert(area, "<meta:Width>1997</meta:Width>");
	}

	dcp::PKL pkl(find_pkl(dir));
	auto cpl = std::make_shared<dcp::CPL>(find_cpl(dir));

	check_verify_result(
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, canonical(find_cpl(dir))
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_MAIN_PICTURE_ACTIVE_AREA, "width 1997 is not a multiple of 2", canonical(find_cpl(dir))
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_MAIN_PICTURE_ACTIVE_AREA, "height 4080 is bigger than the asset height 1080", canonical(find_cpl(dir))
				).set_cpl_id(cpl->id()),
		});
}


BOOST_AUTO_TEST_CASE(verify_invalid_main_picture_active_area_2)
{
	path dir = "build/test/verify_invalid_main_picture_active_area_2";
	prepare_directory(dir);
	auto dcp = make_simple(dir, 1, 24);
	dcp->write_xml();

	auto constexpr area = "<meta:MainPictureActiveArea>";

	HashCalculator calc(find_cpl(dir));

	{
		Editor e(find_cpl(dir));
		e.delete_lines_after(area, 2);
		e.insert(area, "<meta:Height>5125</meta:Height>");
		e.insert(area, "<meta:Width>9900</meta:Width>");
	}

	dcp::PKL pkl(find_pkl(dir));
	auto cpl = std::make_shared<dcp::CPL>(find_cpl(dir));

	check_verify_result(
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_CPL_HASHES, canonical(find_cpl(dir))
				).set_cpl_id(cpl->id()).set_reference_hash(calc.old_hash()).set_calculated_hash(calc.new_hash()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_MAIN_PICTURE_ACTIVE_AREA, "height 5125 is not a multiple of 2", canonical(find_cpl(dir))
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_MAIN_PICTURE_ACTIVE_AREA, "width 9900 is bigger than the asset width 1998", canonical(find_cpl(dir))
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_MAIN_PICTURE_ACTIVE_AREA, "height 5125 is bigger than the asset height 1080", canonical(find_cpl(dir))
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE(verify_duplicate_pkl_asset_ids)
{
	RNGFixer rg;

	path dir = "build/test/verify_duplicate_pkl_asset_ids";
	prepare_directory(dir);
	auto dcp = make_simple(dir, 1, 24);
	dcp->write_xml();

	{
		Editor e(find_pkl(dir));
		e.replace("urn:uuid:5407b210-4441-4e97-8b16-8bdc7c12da54", "urn:uuid:6affb8ee-0020-4dff-a53c-17652f6358ab");
	}

	dcp::PKL pkl(find_pkl(dir));
	auto cpl = std::make_shared<dcp::CPL>(find_cpl(dir));

	check_verify_result(
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			{ dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::DUPLICATE_ASSET_ID_IN_PKL, pkl.id(), canonical(find_pkl(dir)) },
		});
}


BOOST_AUTO_TEST_CASE(verify_duplicate_assetmap_asset_ids)
{
	RNGFixer rg;

	path dir = "build/test/verify_duplicate_assetmap_asset_ids";
	prepare_directory(dir);
	auto dcp = make_simple(dir, 1, 24);
	dcp->write_xml();

	{
		Editor e(find_asset_map(dir));
		e.replace("urn:uuid:5407b210-4441-4e97-8b16-8bdc7c12da54", "urn:uuid:97f0f352-5b77-48ee-a558-9df37717f4fa");
	}

	dcp::PKL pkl(find_pkl(dir));
	dcp::AssetMap asset_map(find_asset_map(dir));
	auto cpl = make_shared<dcp::CPL>(find_cpl(dir));

	check_verify_result(
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				canonical(cpl->file().get())
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::DUPLICATE_ASSET_ID_IN_ASSETMAP, asset_map.id(), canonical(find_asset_map(dir))
				),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::EXTERNAL_ASSET, string("5407b210-4441-4e97-8b16-8bdc7c12da54")
				)
		});
}


BOOST_AUTO_TEST_CASE(verify_mismatched_sound_channel_counts)
{
	boost::filesystem::path const path = "build/test/verify_mismatched_sound_channel_counts";

	dcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	auto constexpr sample_rate = 48000;
	auto constexpr frames = 240;

	boost::filesystem::remove_all(path);
	boost::filesystem::create_directories(path);
	auto dcp = make_shared<dcp::DCP>(path);
	auto cpl = make_shared<dcp::CPL>("hello", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
	cpl->set_annotation_text("hello");
	cpl->set_main_sound_configuration(dcp::MainSoundConfiguration("51/L,R"));
	cpl->set_main_sound_sample_rate(sample_rate);
	cpl->set_main_picture_stored_area(dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area(dcp::Size(1998, 1080));
	cpl->set_version_number(1);

	{

		/* Reel with 2 channels of audio */

		auto mp = simple_picture(path, "1", frames, {});
		auto ms = simple_sound(path, "1", mxf_meta, "en-US", frames, sample_rate, {}, 2);

		auto reel = make_shared<dcp::Reel>(
			std::make_shared<dcp::ReelMonoPictureAsset>(mp, 0),
			std::make_shared<dcp::ReelSoundAsset>(ms, 0)
			);

		auto markers = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), frames);
		markers->set(dcp::Marker::FFOC, dcp::Time(0, 0, 0, 1, 24));
		reel->add(markers);

		cpl->add(reel);
	}

	{
		/* Reel with 6 channels of audio */

		auto mp = simple_picture(path, "2", frames, {});
		auto ms = simple_sound(path, "2", mxf_meta, "en-US", frames, sample_rate, {}, 6);

		auto reel = make_shared<dcp::Reel>(
			std::make_shared<dcp::ReelMonoPictureAsset>(mp, 0),
			std::make_shared<dcp::ReelSoundAsset>(ms, 0)
			);

		auto markers = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), frames);
		markers->set(dcp::Marker::LFOC, dcp::Time(0, 0, 0, frames - 1, 24));
		reel->add(markers);

		cpl->add(reel);
	}

	dcp->add(cpl);
	dcp->set_annotation_text("hello");
	dcp->write_xml();

	check_verify_result(
		{ path },
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(path / "video1.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(path / "video1.mxf"), cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(path / "video2.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(path / "video2.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_SOUND_CHANNEL_COUNTS, canonical(find_file(path, "audio2"))
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE(verify_invalid_main_sound_configuration)
{
	boost::filesystem::path const path = "build/test/verify_invalid_main_sound_configuration";

	dcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	auto constexpr sample_rate = 48000;
	auto constexpr frames = 240;

	boost::filesystem::remove_all(path);
	boost::filesystem::create_directories(path);
	auto dcp = make_shared<dcp::DCP>(path);
	auto cpl = make_shared<dcp::CPL>("hello", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
	cpl->set_annotation_text("hello");
	cpl->set_main_sound_configuration(dcp::MainSoundConfiguration("51/L,R,C,LFE,Ls,Rs"));
	cpl->set_main_sound_sample_rate(sample_rate);
	cpl->set_main_picture_stored_area(dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area(dcp::Size(1998, 1080));
	cpl->set_version_number(1);

	auto mp = simple_picture(path, "1", frames, {});
	auto ms = simple_sound(path, "1", mxf_meta, "en-US", frames, sample_rate, {}, 2);

	auto reel = make_shared<dcp::Reel>(
		std::make_shared<dcp::ReelMonoPictureAsset>(mp, 0),
		std::make_shared<dcp::ReelSoundAsset>(ms, 0)
		);

	auto markers = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), frames);
	markers->set(dcp::Marker::FFOC, dcp::Time(0, 0, 0, 1, 24));
	markers->set(dcp::Marker::LFOC, dcp::Time(0, 0, 9, 23, 24));
	reel->add(markers);

	cpl->add(reel);

	dcp->add(cpl);
	dcp->set_annotation_text("hello");
	dcp->write_xml();

	check_verify_result(
		{ path },
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(path / "video1.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(path / "video1.mxf"), cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR,
				dcp::VerificationNote::Code::INVALID_MAIN_SOUND_CONFIGURATION,
				std::string{"MainSoundConfiguration has 6 channels but sound assets have 2"},
				canonical(find_cpl(path))
				).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE(verify_invalid_tile_part_size)
{
	boost::filesystem::path const path = "build/test/verify_invalid_tile_part_size";
	auto constexpr video_frames = 24;
	auto constexpr sample_rate = 48000;

	boost::filesystem::remove_all(path);
	boost::filesystem::create_directories(path);

	auto mp = make_shared<dcp::MonoJ2KPictureAsset>(dcp::Fraction(24, 1), dcp::Standard::SMPTE);
	auto picture_writer = mp->start_write(path / "video.mxf", dcp::Behaviour::MAKE_NEW);

	dcp::Size const size(1998, 1080);
	auto image = make_shared<dcp::OpenJPEGImage>(size);
	boost::random::mt19937 rng(1);
	boost::random::uniform_int_distribution<> dist(0, 4095);
	for (int c = 0; c < 3; ++c) {
		for (int p = 0; p < (1998 * 1080); ++p) {
			image->data(c)[p] = dist(rng);
		}
	}
	auto j2c = dcp::compress_j2k(image, 750000000, video_frames, false, false);
	for (int i = 0; i < 24; ++i) {
		picture_writer->write(j2c.data(), j2c.size());
	}
	picture_writer->finalize();

	auto dcp = make_shared<dcp::DCP>(path);
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
	cpl->set_content_version(
		dcp::ContentVersion("urn:uuid:75ac29aa-42ac-1234-ecae-49251abefd11", "content-version-label-text")
		);
	cpl->set_main_sound_configuration(dcp::MainSoundConfiguration("51/L,R,C,LFE,Ls,Rs"));
	cpl->set_main_sound_sample_rate(sample_rate);
	cpl->set_main_picture_stored_area(dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area(dcp::Size(1998, 1080));
	cpl->set_version_number(1);

	auto ms = simple_sound(path, "", dcp::MXFMetadata(), "en-US", video_frames, sample_rate, {});

	auto reel = make_shared<dcp::Reel>(
		make_shared<dcp::ReelMonoPictureAsset>(mp, 0),
		make_shared<dcp::ReelSoundAsset>(ms, 0)
		);

	cpl->add(reel);
	dcp->add(cpl);
	dcp->set_annotation_text("A Test DCP");
	dcp->write_xml();

	vector<dcp::VerificationNote> expected = {
		ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::OK,
			dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
			string{"1998x1080"},
			cpl->file().get()
			).set_cpl_id(cpl->id()),
		ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(path / "video.mxf"), cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
		ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSING_FFOC
			).set_cpl_id(cpl->id()),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSING_LFOC
			).set_cpl_id(cpl->id())
	};

	for (auto frame = 0; frame < 24; frame++) {
		expected.push_back(
			dcp::VerificationNote(
				dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_PICTURE_FRAME_SIZE_IN_BYTES, canonical(path / "video.mxf")
			).set_frame(frame).set_frame_rate(24).set_cpl_id(cpl->id())
		);
	}

	int component_sizes[] = {
		1321816,
		1294414,
		1289881,
	};

	for (auto frame = 0; frame < 24; frame++) {
		for (auto component = 0; component < 3; component++) {
			expected.push_back(
				dcp::VerificationNote(
					dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_JPEG2000_TILE_PART_SIZE
					).set_frame(frame).set_frame_rate(24).set_component(component).set_size(component_sizes[component]).set_cpl_id(cpl->id())
				);
		}
	}

	check_verify_result({ path }, {}, expected);
}


BOOST_AUTO_TEST_CASE(verify_too_many_subtitle_namespaces)
{
	boost::filesystem::path const dir = "test/ref/DCP/subtitle_namespace_test";
	dcp::DCP dcp(dir);
	dcp.read();
	BOOST_REQUIRE(!dcp.cpls().empty());
	auto cpl = dcp.cpls()[0];

	check_verify_result(
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"feature"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"Dcp_FTR-1_F_XX-XX_MOS_2K_20230407_SMPTE_OV"}, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "j2c_42b34dcd-caa5-4c7b-aa0f-66a590947ba1.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "j2c_42b34dcd-caa5-4c7b-aa0f-66a590947ba1.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_FFEC_IN_FEATURE
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_FFMC_IN_FEATURE
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_SUBTITLE_LANGUAGE, canonical(find_file(dir, "sub_"))
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, canonical(find_file(dir, "cpl_"))
				).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::INCORRECT_SUBTITLE_NAMESPACE_COUNT, std::string{"315de731-1173-484c-9a35-bdacf5a9d99d"}
				).set_cpl_id(cpl->id()),
		});
}


BOOST_AUTO_TEST_CASE(verify_missing_load_font_for_font)
{
	path const dir("build/test/verify_missing_load_font");
	prepare_directory (dir);
	copy_file ("test/data/subs1.xml", dir / "subs.xml");
	{
		Editor editor(dir / "subs.xml");
		editor.delete_first_line_containing("LoadFont");
	}
	auto asset = make_shared<dcp::InteropTextAsset>(dir / "subs.xml");
	auto reel_asset = make_shared<dcp::ReelInteropTextAsset>(dcp::TextType::OPEN_SUBTITLE, asset, dcp::Fraction(24, 1), 16 * 24, 0);
	auto cpl = write_dcp_with_single_asset(dir, reel_asset, dcp::Standard::INTEROP);

	check_verify_result (
		{dir},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			{ dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_STANDARD },
			dcp::VerificationNote(dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISSING_LOAD_FONT_FOR_FONT).set_id("theFontId").set_cpl_id(cpl->id())
		});

}


BOOST_AUTO_TEST_CASE(verify_missing_load_font)
{
	boost::filesystem::path const dir = "build/test/verify_missing_load_font";
	prepare_directory(dir);
	auto dcp = make_simple (dir, 1, 202);

	string const xml =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SubtitleReel xmlns=\"http://www.smpte-ra.org/schemas/428-7/2010/DCST\">"
		"<Id>urn:uuid:e6a8ae03-ebbf-41ed-9def-913a87d1493a</Id>"
		"<ContentTitleText>Content</ContentTitleText>"
		"<AnnotationText>Annotation</AnnotationText>"
		"<IssueDate>2018-10-02T12:25:14+02:00</IssueDate>"
		"<ReelNumber>1</ReelNumber>"
		"<EditRate>24 1</EditRate>"
		"<TimeCodeRate>24</TimeCodeRate>"
		"<StartTime>00:00:00:00</StartTime>"
		"<Language>de-DE</Language>"
		"<SubtitleList>"
		"<Font ID=\"arial\" Color=\"FFFEFEFE\" Weight=\"normal\" Size=\"42\" Effect=\"border\" EffectColor=\"FF181818\" AspectAdjust=\"1.00\">"
		"<Subtitle SpotNumber=\"1\" TimeIn=\"00:00:06:00\" TimeOut=\"00:00:08:10\" FadeUpTime=\"00:00:00:00\" FadeDownTime=\"00:00:00:00\">"
		"<Text Hposition=\"0.0\" Halign=\"center\" Valign=\"bottom\" Vposition=\"13.5\" Direction=\"ltr\">Hello world</Text>"
		"</Subtitle>"
		"</Font>"
		"</SubtitleList>"
		"</SubtitleReel>";

	dcp::File xml_file(dir / "subs.xml", "w");
	BOOST_REQUIRE(xml_file);
	xml_file.write(xml.c_str(), xml.size(), 1);
	xml_file.close();
	auto subs = make_shared<dcp::SMPTETextAsset>(dir / "subs.xml");
	subs->write(dir / "subs.mxf");

	auto reel_subs = make_shared<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, subs, dcp::Fraction(24, 1), 202, 0);
	auto cpl = dcp->cpls()[0];
	cpl->reels()[0]->add(reel_subs);
	dcp->write_xml();

	check_verify_result (
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISSING_LOAD_FONT).set_id(reel_subs->id()).set_cpl_id(cpl->id())
		});
}


BOOST_AUTO_TEST_CASE(verify_spots_wrong_asset)
{
	boost::filesystem::path const dir = "build/test/verify_spots_wrong_asset";
	boost::filesystem::remove_all(dir);

	auto dcp1 = make_simple(dir / "1");
	dcp1->write_xml();
	auto cpl = dcp1->cpls()[0];

	auto const asset_1 = dcp::MonoJ2KPictureAsset(dir / "1" / "video.mxf").id();

	auto dcp2 = make_simple(dir / "2");
	dcp2->write_xml();
	auto const asset_2 = dcp::MonoJ2KPictureAsset(dir / "2" / "video.mxf").id();

	boost::filesystem::remove(dir / "1" / "video.mxf");
	boost::filesystem::copy_file(dir / "2" / "video.mxf", dir / "1" / "video.mxf");

	check_verify_result(
		{dir / "1"},
		{},
		{
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			dcp::VerificationNote(dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::MISMATCHED_ASSET_MAP_ID).set_id(asset_1).set_other_id(asset_2)
		});
}


BOOST_AUTO_TEST_CASE(verify_cpl_content_version_label_text_empty)
{
	boost::filesystem::path const dir = "build/test/verify_cpl_content_version_label_text_empty";
	boost::filesystem::remove_all(dir);

	auto dcp = make_simple(dir);
	BOOST_REQUIRE(dcp->cpls().size() == 1);
	auto cpl = dcp->cpls()[0];
	cpl->set_content_version(dcp::ContentVersion(""));
	dcp->write_xml();

	check_verify_result(
		{dir},
		{},
		{
			dcp::VerificationNote(
				dcp::VerificationNote::Type::OK,
				dcp::VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA,
				string{"1998x1080"},
				cpl->file().get()
				).set_cpl_id(cpl->id()),
			ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"A Test DCP"}, cpl),
			ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "video.mxf"), cpl),
			ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "video.mxf"), cpl),
			dcp::VerificationNote(dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::EMPTY_CONTENT_VERSION_LABEL_TEXT, cpl->file().get()).set_cpl_id(cpl->id())
		});
}


/** Check that we don't get any strange errors when verifying encrypted DCPs (DoM #2659) */
BOOST_AUTO_TEST_CASE(verify_encrypted_smpte_dcp)
{
	auto const dir = path("build/test/verify_encrypted_smpte_dcp");
	dcp::Key key;
	auto key_id = dcp::make_uuid();
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, dir, {{ 4 * 24, 5 * 24 }}, key, key_id);

	dcp::DecryptedKDM kdm(dcp::LocalTime(), dcp::LocalTime(), "", "", "");
	kdm.add_key(dcp::DecryptedKDMKey(string{"MDIK"}, key_id, key, cpl->id(), dcp::Standard::SMPTE));

	path const pkl_file = find_file(dir, "pkl_");
	path const cpl_file = find_file(dir, "cpl_");

	check_verify_result(
		{ dir },
		{ kdm },
		{
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			ok(dcp::VerificationNote::Code::ALL_ENCRYPTED, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, canonical(cpl_file)
				).set_cpl_id(cpl->id())
		});
}


/** Check that we don't get any strange errors when verifying encrypted DCPs without a KDM (DoM #2916) */
BOOST_AUTO_TEST_CASE(verify_encrypted_smpte_dcp_without_kdm)
{
	auto const dir = path("build/test/verify_encrypted_smpte_dcp_without_kdm");
	dcp::Key key;
	auto key_id = dcp::make_uuid();
	auto cpl = dcp_with_text<dcp::ReelSMPTETextAsset>(dcp::TextType::OPEN_SUBTITLE, dir, {{ 4 * 24, 5 * 24 }}, key, key_id);

	path const pkl_file = find_file(dir, "pkl_");
	path const cpl_file = find_file(dir, "cpl_");

	check_verify_result(
		{ dir },
		{},
		{
			ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
			ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
			ok(dcp::VerificationNote::Code::ALL_ENCRYPTED, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"trailer"}, cpl),
			ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
			ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"hello"}, cpl),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSED_CHECK_OF_ENCRYPTED
			).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSED_CHECK_OF_ENCRYPTED
			).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSED_CHECK_OF_ENCRYPTED
			).set_cpl_id(cpl->id()),
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, canonical(cpl_file)
				).set_cpl_id(cpl->id()),
		});
}


BOOST_AUTO_TEST_CASE(verify_invalid_sound_bit_depth)
{
	auto const dir = private_test / "data" / "16_bit_audio";

	auto cpl = make_shared<dcp::CPL>(find_prefix(dir, "CPL_"));

	vector<dcp::VerificationNote> notes = {
		ok(dcp::VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL, cpl),
		ok(dcp::VerificationNote::Code::MATCHING_CPL_HASHES, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_KIND, string{"Advertisement"}, cpl),
		ok(dcp::VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, cpl->content_version()->label_text, cpl),
		ok(dcp::VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, string{"204794_Kitex_Scoobee_Day_Bags_30_Sec_Malayalam_220524_RADQR"}, cpl),
		ok(dcp::VerificationNote::Code::NONE_ENCRYPTED, cpl),
		ok(dcp::VerificationNote::Code::CORRECT_PICTURE_HASH, canonical(dir / "Video.mxf"), cpl),
		ok(dcp::VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, canonical(dir / "Video.mxf"), cpl),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::ERROR, dcp::VerificationNote::Code::INVALID_SOUND_BIT_DEPTH, "16", canonical(dir / "Audio.mxf")
			).set_cpl_id(cpl->id()),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_HASH, string("fd4796c2-9c84-454c-91f4-13ad127cea8a")
			).set_cpl_id(cpl->id()),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_HASH, string("9d5e8bc4-676b-4306-a86d-03f70c73b457")
			).set_cpl_id(cpl->id()),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::MISSING_CPL_METADATA, canonical(cpl->file().get())
			).set_cpl_id(cpl->id()),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSING_FFOC
			).set_cpl_id(cpl->id()),
		dcp::VerificationNote(
			dcp::VerificationNote::Type::WARNING, dcp::VerificationNote::Code::MISSING_LFOC
			).set_cpl_id(cpl->id()),
	};

	for (auto i = 0; i < 792; ++i) {
		notes.push_back(
			dcp::VerificationNote(
				dcp::VerificationNote::Type::BV21_ERROR, dcp::VerificationNote::Code::INVALID_JPEG2000_GUARD_BITS_FOR_2K, string("2")
				).set_cpl_id(cpl->id())
			);
	}

	check_verify_result({ dir }, {}, notes);
}


BOOST_AUTO_TEST_CASE(overlapping_subtitles)
{
	auto asset = make_shared<dcp::InteropTextAsset>();

	asset->add(std::make_shared<dcp::TextString>(
			optional<string>{}, false, false, false,
			dcp::Colour{}, 42, 0,
			dcp::Time(0, 0, 0, 0, 24),
			dcp::Time(0, 0, 8, 0, 24),
			0, dcp::HAlign::CENTER,
			0, dcp::VAlign::CENTER,
			0, vector<dcp::Text::VariableZPosition>(),
			dcp::Direction::LTR,
			"",
			dcp::Effect::NONE, dcp::Colour{}, dcp::Time{}, dcp::Time{}, 0, vector<dcp::Ruby>{}
			));

	asset->add(std::make_shared<dcp::TextString>(
			optional<string>{}, false, false, false,
			dcp::Colour{}, 42, 0,
			dcp::Time(0, 0, 2, 0, 24),
			dcp::Time(0, 0, 4, 0, 24),
			0, dcp::HAlign::CENTER,
			0, dcp::VAlign::CENTER,
			0, vector<dcp::Text::VariableZPosition>(),
			dcp::Direction::LTR,
			"Hello",
			dcp::Effect::NONE, dcp::Colour{}, dcp::Time{}, dcp::Time{}, 0, vector<dcp::Ruby>{}
			));

	dcp::LinesCharactersResult result;
	dcp::verify_text_lines_and_characters(asset, 64, 80, &result);
}


BOOST_AUTO_TEST_CASE(multiple_metadata_property)
{
	vector<dcp::VerificationNote> notes;
	auto stage = [](std::string, boost::optional<boost::filesystem::path>) {};
	auto progress = [](float) {};

	dcp::Context context(notes, {}, stage, progress, {});
	context.cpl = make_shared<dcp::CPL>(private_test / "CPL_6935f81f-30d3-4283-898e-5bb1e9c2558c.xml");
	verify_extension_metadata(context);
}


BOOST_AUTO_TEST_CASE(only_verify_assets_once)
{
	auto const dir = boost::filesystem::path("build/test/only_verify_assets_once");
	prepare_directory(dir);

	/* Make a DCP which re-uses two assets */

	auto picture = simple_picture(dir, "foo");
	auto sound = simple_sound(dir, "foo", dcp::MXFMetadata(), "de-DE", 24, 96000, boost::none);
	auto cpl = make_shared<dcp::CPL>("hello", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
	for (int i = 0; i < 2; ++i) {
		auto reel = make_shared<dcp::Reel>();
		auto reel_picture = make_shared<dcp::ReelMonoPictureAsset>(picture, 0);
		reel->add(reel_picture);
		auto reel_sound = make_shared<dcp::ReelSoundAsset>(sound, 0);
		reel->add(reel_sound);
		reel->add(simple_markers());
		cpl->add(reel);
	}

	auto dcp = make_shared<dcp::DCP>(dir);
	dcp->add(cpl);
	dcp->set_annotation_text("hello");
	dcp->write_xml();

	vector<string> stages;
	auto stage = [&stages](std::string stage, boost::optional<boost::filesystem::path>) { stages.push_back(stage); };
	auto progress = [](float) {};

	dcp::verify({dir}, {}, stage, progress, {}, xsd_test);

	BOOST_CHECK_EQUAL(std::count(stages.begin(), stages.end(), "Checking picture asset hash"), 1);
	BOOST_CHECK_EQUAL(std::count(stages.begin(), stages.end(), "Checking sound asset hash"), 1);

	for (auto i: stages) {
		std::cout << i << "\n";
	}
}

