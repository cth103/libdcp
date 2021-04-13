/*
    Copyright (C) 2012-2020 Carl Hetherington <cth@carlh.net>

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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libdcp_test
#include "compose.hpp"
#include "cpl.h"
#include "dcp.h"
#include "interop_subtitle_asset.h"
#include "j2k_transcode.h"
#include "mono_picture_asset.h"
#include "mono_picture_asset.h"
#include "openjpeg_image.h"
#include "picture_asset_writer.h"
#include "picture_asset_writer.h"
#include "reel.h"
#include "reel_asset.h"
#include "reel_interop_closed_caption_asset.h"
#include "reel_interop_subtitle_asset.h"
#include "reel_markers_asset.h"
#include "reel_mono_picture_asset.h"
#include "reel_mono_picture_asset.h"
#include "reel_smpte_closed_caption_asset.h"
#include "reel_smpte_subtitle_asset.h"
#include "reel_sound_asset.h"
#include "smpte_subtitle_asset.h"
#include "sound_asset.h"
#include "sound_asset_writer.h"
#include "test.h"
#include "util.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <asdcp/KM_util.h>
#include <asdcp/KM_prng.h>
LIBDCP_ENABLE_WARNINGS
#include <sndfile.h>
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <boost/test/unit_test.hpp>
#include <cstdio>
#include <iostream>


using std::string;
using std::min;
using std::vector;
using std::shared_ptr;
using std::make_shared;
using boost::optional;


boost::filesystem::path private_test;
boost::filesystem::path xsd_test = "build/test/xsd with spaces";


struct TestConfig
{
	TestConfig()
	{
		dcp::init ();
		if (boost::unit_test::framework::master_test_suite().argc >= 2) {
			private_test = boost::unit_test::framework::master_test_suite().argv[1];
		}

		using namespace boost::filesystem;
		boost::system::error_code ec;
		remove_all (xsd_test, ec);
		boost::filesystem::create_directory (xsd_test);
		for (directory_iterator i = directory_iterator("xsd"); i != directory_iterator(); ++i) {
			copy_file (*i, xsd_test / i->path().filename());
		}
	}
};


void
check_xml (xmlpp::Element* ref, xmlpp::Element* test, vector<string> ignore_tags, bool ignore_whitespace)
{
	BOOST_CHECK_EQUAL (ref->get_name (), test->get_name ());
	BOOST_CHECK_EQUAL (ref->get_namespace_prefix (), test->get_namespace_prefix ());

	if (find(ignore_tags.begin(), ignore_tags.end(), ref->get_name()) != ignore_tags.end()) {
		return;
	}

	auto whitespace_content = [](xmlpp::Node* node) {
		auto content = dynamic_cast<xmlpp::ContentNode*>(node);
		return content && content->get_content().find_first_not_of(" \t\r\n") == string::npos;
	};

	auto ref_children = ref->get_children ();
	auto test_children = test->get_children ();

	auto k = ref_children.begin ();
	auto l = test_children.begin ();
	while (k != ref_children.end() && l != test_children.end()) {

		if (dynamic_cast<xmlpp::CommentNode*>(*k)) {
			++k;
			continue;
		}

		if (dynamic_cast<xmlpp::CommentNode*>(*l)) {
			++l;
			continue;
		}

		if (whitespace_content(*k) && ignore_whitespace) {
			++k;
			continue;
		}

		if (whitespace_content(*l) && ignore_whitespace) {
			++l;
			continue;
		}

		/* XXX: should be doing xmlpp::EntityReference, xmlpp::XIncludeEnd, xmlpp::XIncludeStart */

		auto ref_el = dynamic_cast<xmlpp::Element*> (*k);
		auto test_el = dynamic_cast<xmlpp::Element*> (*l);
		BOOST_CHECK ((ref_el && test_el) || (!ref_el && !test_el));
		if (ref_el && test_el) {
			check_xml (ref_el, test_el, ignore_tags, ignore_whitespace);
		}

		auto ref_cn = dynamic_cast<xmlpp::ContentNode*> (*k);
		auto test_cn = dynamic_cast<xmlpp::ContentNode*> (*l);
		BOOST_CHECK ((ref_cn && test_cn) || (!ref_cn && !test_cn));
		if (ref_cn && test_cn) {
			BOOST_CHECK_EQUAL (ref_cn->get_content(), test_cn->get_content());
		}

		++k;
		++l;
	}

	while (k != ref_children.end() && ignore_whitespace && whitespace_content(*k)) {
		++k;
	}

	while (l != test_children.end() && ignore_whitespace && whitespace_content(*l)) {
		++l;
	}

	BOOST_REQUIRE (k == ref_children.end());
	BOOST_REQUIRE (l == test_children.end());

	auto ref_attributes = ref->get_attributes ();
	auto test_attributes = test->get_attributes ();
	BOOST_CHECK_EQUAL (ref_attributes.size(), test_attributes.size ());

	auto m = ref_attributes.begin();
	auto n = test_attributes.begin();
	while (m != ref_attributes.end ()) {
		BOOST_CHECK_EQUAL ((*m)->get_name(), (*n)->get_name());
		BOOST_CHECK_EQUAL ((*m)->get_value(), (*n)->get_value());

		++m;
		++n;
	}
}

void
check_xml (string ref, string test, vector<string> ignore, bool ignore_whitespace)
{
	auto ref_parser = new xmlpp::DomParser ();
	ref_parser->parse_memory (ref);
	auto ref_root = ref_parser->get_document()->get_root_node ();
	auto test_parser = new xmlpp::DomParser ();
	test_parser->parse_memory (test);
	auto test_root = test_parser->get_document()->get_root_node ();

	check_xml (ref_root, test_root, ignore, ignore_whitespace);
}

void
check_file (boost::filesystem::path ref, boost::filesystem::path check)
{
	uintmax_t size = boost::filesystem::file_size (ref);
	BOOST_CHECK_EQUAL (size, boost::filesystem::file_size(check));
	auto ref_file = dcp::fopen_boost (ref, "rb");
	BOOST_REQUIRE (ref_file);
	auto check_file = dcp::fopen_boost (check, "rb");
	BOOST_REQUIRE (check_file);

	int const buffer_size = 65536;
	auto ref_buffer = new uint8_t[buffer_size];
	auto check_buffer = new uint8_t[buffer_size];

	uintmax_t pos = 0;

	while (pos < size) {
		uintmax_t this_time = min (uintmax_t(buffer_size), size - pos);
		size_t r = fread (ref_buffer, 1, this_time, ref_file);
		BOOST_CHECK_EQUAL (r, this_time);
		r = fread (check_buffer, 1, this_time, check_file);
		BOOST_CHECK_EQUAL (r, this_time);

		if (memcmp(ref_buffer, check_buffer, this_time) != 0) {
			for (int i = 0; i < buffer_size; ++i) {
				if (ref_buffer[i] != check_buffer[i]) {
					BOOST_CHECK_MESSAGE (
						false,
						dcp::String::compose("File %1 differs from reference %2 at offset %3", check, ref, pos + i)
						);
					break;
				}
			}
			break;
		}

		pos += this_time;
	}

	delete[] ref_buffer;
	delete[] check_buffer;

	fclose (ref_file);
	fclose (check_file);
}


RNGFixer::RNGFixer ()
{
	Kumu::cth_test = true;
	Kumu::FortunaRNG().Reset();
}


RNGFixer::~RNGFixer ()
{
	Kumu::cth_test = false;
}


shared_ptr<dcp::MonoPictureAsset>
simple_picture (boost::filesystem::path path, string suffix, int frames)
{
	dcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	shared_ptr<dcp::MonoPictureAsset> mp (new dcp::MonoPictureAsset (dcp::Fraction (24, 1), dcp::Standard::SMPTE));
	mp->set_metadata (mxf_meta);
	shared_ptr<dcp::PictureAssetWriter> picture_writer = mp->start_write (path / dcp::String::compose("video%1.mxf", suffix), false);

	dcp::Size const size (1998, 1080);
	auto image = make_shared<dcp::OpenJPEGImage>(size);
	for (int i = 0; i < 3; ++i) {
		memset (image->data(i), 0, 2 * size.width * size.height);
	}
	auto j2c = dcp::compress_j2k (image, 100000000, 24, false, false);

	for (int i = 0; i < frames; ++i) {
		picture_writer->write (j2c.data(), j2c.size());
	}
	picture_writer->finalize ();

	return mp;
}


shared_ptr<dcp::SoundAsset>
simple_sound (boost::filesystem::path path, string suffix, dcp::MXFMetadata mxf_meta, string language, int frames, int sample_rate)
{
	int const channels = 6;

	/* Set a valid language, then overwrite it, so that the language parameter can be badly formed */
	shared_ptr<dcp::SoundAsset> ms (new dcp::SoundAsset(dcp::Fraction(24, 1), sample_rate, channels, dcp::LanguageTag("en-US"), dcp::Standard::SMPTE));
	ms->_language = language;
	ms->set_metadata (mxf_meta);
	shared_ptr<dcp::SoundAssetWriter> sound_writer = ms->start_write (path / dcp::String::compose("audio%1.mxf", suffix));

	int const samples_per_frame = sample_rate / 24;

	float* silence[channels];
	for (auto i = 0; i < channels; ++i) {
		silence[i] = new float[samples_per_frame];
		memset (silence[i], 0, samples_per_frame * sizeof(float));
	}

	for (auto i = 0; i < frames; ++i) {
		sound_writer->write (silence, samples_per_frame);
	}

	sound_writer->finalize ();

	for (auto i = 0; i < channels; ++i) {
		delete[] silence[i];
	}

	return ms;
}


shared_ptr<dcp::DCP>
make_simple (boost::filesystem::path path, int reels, int frames, dcp::Standard standard)
{
	/* Some known metadata */
	dcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	boost::filesystem::remove_all (path);
	boost::filesystem::create_directories (path);
	auto d = make_shared<dcp::DCP>(path);
	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::TRAILER, standard);
	cpl->set_annotation_text ("A Test DCP");
	cpl->set_issuer ("OpenDCP 0.0.25");
	cpl->set_creator ("OpenDCP 0.0.25");
	cpl->set_issue_date ("2012-07-17T04:45:18+00:00");
	cpl->set_content_version (
		dcp::ContentVersion("urn:uuid:75ac29aa-42ac-1234-ecae-49251abefd11", "content-version-label-text")
		);
	cpl->set_main_sound_configuration("51/L,R,C,LFE,Ls,Rs");
	cpl->set_main_sound_sample_rate(48000);
	cpl->set_main_picture_stored_area(dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area(dcp::Size(1998, 1080));
	cpl->set_version_number(1);

	for (int i = 0; i < reels; ++i) {
		string suffix = reels == 1 ? "" : dcp::String::compose("%1", i);

		shared_ptr<dcp::MonoPictureAsset> mp = simple_picture (path, suffix, frames);
		shared_ptr<dcp::SoundAsset> ms = simple_sound (path, suffix, mxf_meta, "en-US", frames);

		auto reel = make_shared<dcp::Reel>(
			shared_ptr<dcp::ReelMonoPictureAsset>(new dcp::ReelMonoPictureAsset(mp, 0)),
			shared_ptr<dcp::ReelSoundAsset>(new dcp::ReelSoundAsset(ms, 0))
			);

		auto markers = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), frames, 0);
		if (i == 0) {
			markers->set (dcp::Marker::FFOC, dcp::Time(0, 0, 0, 1, 24));
		}
		if (i == reels - 1) {
			markers->set (dcp::Marker::LFOC, dcp::Time(0, 0, 0, frames - 1, 24));
		}
		reel->add (markers);

		cpl->add (reel);
	}

	d->add (cpl);
	return d;
}


shared_ptr<dcp::Subtitle>
simple_subtitle ()
{
	return make_shared<dcp::SubtitleString>(
		optional<string>(),
		false,
		false,
		false,
		dcp::Colour(255, 255, 255),
		42,
		1,
		dcp::Time(0, 0, 4, 0, 24),
		dcp::Time(0, 0, 8, 0, 24),
		0.5,
		dcp::HAlign::CENTER,
		0.8,
		dcp::VAlign::TOP,
		dcp::Direction::LTR,
		"Hello world",
		dcp::Effect::NONE,
		dcp::Colour(255, 255, 255),
		dcp::Time(),
		dcp::Time()
		);
}


shared_ptr<dcp::ReelMarkersAsset>
simple_markers (int frames)
{
	auto markers = make_shared<dcp::ReelMarkersAsset>(dcp::Fraction(24, 1), frames, 0);
	markers->set (dcp::Marker::FFOC, dcp::Time(1, 24, 24));
	markers->set (dcp::Marker::LFOC, dcp::Time(frames - 1, 24, 24));
	return markers;
}


shared_ptr<dcp::DCP>
make_simple_with_interop_subs (boost::filesystem::path path)
{
	auto dcp = make_simple (path, 1, 24, dcp::Standard::INTEROP);

	auto subs = make_shared<dcp::InteropSubtitleAsset>();
	subs->add (simple_subtitle());

	boost::filesystem::create_directory (path / "subs");
	dcp::ArrayData data(4096);
	subs->add_font ("afont", data);
	subs->write (path / "subs" / "subs.xml");

	auto reel_subs = make_shared<dcp::ReelInteropSubtitleAsset>(subs, dcp::Fraction(24, 1), 240, 0);
	dcp->cpls().front()->reels().front()->add (reel_subs);

	return dcp;
}


shared_ptr<dcp::DCP>
make_simple_with_smpte_subs (boost::filesystem::path path)
{
	auto dcp = make_simple (path, 1, 192);

	auto subs = make_shared<dcp::SMPTESubtitleAsset>();
	subs->set_language (dcp::LanguageTag("de-DE"));
	subs->set_start_time (dcp::Time());
	subs->add (simple_subtitle());

	subs->write (path / "subs.mxf");

	auto reel_subs = make_shared<dcp::ReelSMPTESubtitleAsset>(subs, dcp::Fraction(24, 1), 192, 0);
	dcp->cpls().front()->reels().front()->add (reel_subs);

	return dcp;
}


shared_ptr<dcp::DCP>
make_simple_with_interop_ccaps (boost::filesystem::path path)
{
	auto dcp = make_simple (path, 1, 24, dcp::Standard::INTEROP);

	auto subs = make_shared<dcp::InteropSubtitleAsset>();
	subs->add (simple_subtitle());
	subs->write (path / "ccap.xml");

	auto reel_caps = make_shared<dcp::ReelInteropClosedCaptionAsset>(subs, dcp::Fraction(24, 1), 240, 0);
	dcp->cpls()[0]->reels()[0]->add (reel_caps);

	return dcp;
}


shared_ptr<dcp::DCP>
make_simple_with_smpte_ccaps (boost::filesystem::path path)
{
	auto dcp = make_simple (path, 1, 192);

	auto subs = make_shared<dcp::SMPTESubtitleAsset>();
	subs->set_language (dcp::LanguageTag("de-DE"));
	subs->set_start_time (dcp::Time());
	subs->add (simple_subtitle());
	subs->write (path / "ccap.mxf");

	auto reel_caps = make_shared<dcp::ReelSMPTEClosedCaptionAsset>(subs, dcp::Fraction(24, 1), 192, 0);
	dcp->cpls()[0]->reels()[0]->add(reel_caps);

	return dcp;
}


shared_ptr<dcp::OpenJPEGImage>
black_image (dcp::Size size)
{
	auto image = make_shared<dcp::OpenJPEGImage>(size);
	int const pixels = size.width * size.height;
	for (int i = 0; i < 3; ++i) {
		memset (image->data(i), 0, pixels * sizeof(int));
	}
	return image;
}


shared_ptr<dcp::ReelAsset>
black_picture_asset (boost::filesystem::path dir, int frames)
{
	auto image = black_image ();
	auto frame = dcp::compress_j2k (image, 100000000, 24, false, false);
	BOOST_REQUIRE (frame.size() < 230000000 / (24 * 8));

	auto asset = make_shared<dcp::MonoPictureAsset>(dcp::Fraction(24, 1), dcp::Standard::SMPTE);
	asset->set_metadata (dcp::MXFMetadata("libdcp", "libdcp", "1.6.4devel"));
	boost::filesystem::create_directories (dir);
	auto writer = asset->start_write (dir / "pic.mxf", true);
	for (int i = 0; i < frames; ++i) {
		writer->write (frame.data(), frame.size());
	}
	writer->finalize ();

	return make_shared<dcp::ReelMonoPictureAsset>(asset, 0);
}


boost::filesystem::path
find_file (boost::filesystem::path dir, string filename_part)
{
	boost::optional<boost::filesystem::path> found;
	for (auto i: boost::filesystem::directory_iterator(dir)) {
		if (i.path().filename().string().find(filename_part) != string::npos) {
			BOOST_REQUIRE (!found);
			found = i;
		}
	}
	BOOST_REQUIRE (found);
	return *found;
}


BOOST_GLOBAL_FIXTURE (TestConfig);
