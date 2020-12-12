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
#include "file.h"
#include "interop_subtitle_asset.h"
#include "mono_picture_asset.h"
#include "picture_asset_writer.h"
#include "reel.h"
#include "reel_mono_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_closed_caption_asset.h"
#include "reel_subtitle_asset.h"
#include "sound_asset.h"
#include "sound_asset_writer.h"
#include "smpte_subtitle_asset.h"
#include "mono_picture_asset.h"
#include "openjpeg_image.h"
#include "j2k.h"
#include "picture_asset_writer.h"
#include "reel_mono_picture_asset.h"
#include "reel_asset.h"
#include "test.h"
#include "util.h"
#include <asdcp/KM_util.h>
#include <asdcp/KM_prng.h>
#include <sndfile.h>
#include <libxml++/libxml++.h>
#include <boost/test/unit_test.hpp>
#include <cstdio>
#include <iostream>

using std::string;
using std::min;
using std::list;
using std::vector;
using std::shared_ptr;
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
check_xml (xmlpp::Element* ref, xmlpp::Element* test, list<string> ignore_tags, bool ignore_whitespace)
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
check_xml (string ref, string test, list<string> ignore, bool ignore_whitespace)
{
	xmlpp::DomParser* ref_parser = new xmlpp::DomParser ();
	ref_parser->parse_memory (ref);
	xmlpp::Element* ref_root = ref_parser->get_document()->get_root_node ();
	xmlpp::DomParser* test_parser = new xmlpp::DomParser ();
	test_parser->parse_memory (test);
	xmlpp::Element* test_root = test_parser->get_document()->get_root_node ();

	check_xml (ref_root, test_root, ignore, ignore_whitespace);
}

void
check_file (boost::filesystem::path ref, boost::filesystem::path check)
{
	uintmax_t N = boost::filesystem::file_size (ref);
	BOOST_CHECK_EQUAL (N, boost::filesystem::file_size (check));
	FILE* ref_file = dcp::fopen_boost (ref, "rb");
	BOOST_REQUIRE (ref_file);
	FILE* check_file = dcp::fopen_boost (check, "rb");
	BOOST_REQUIRE (check_file);

	int const buffer_size = 65536;
	uint8_t* ref_buffer = new uint8_t[buffer_size];
	uint8_t* check_buffer = new uint8_t[buffer_size];

	string error;
	error = "File " + check.string() + " differs from reference " + ref.string();

	while (N) {
		uintmax_t this_time = min (uintmax_t (buffer_size), N);
		size_t r = fread (ref_buffer, 1, this_time, ref_file);
		BOOST_CHECK_EQUAL (r, this_time);
		r = fread (check_buffer, 1, this_time, check_file);
		BOOST_CHECK_EQUAL (r, this_time);

		BOOST_CHECK_MESSAGE (memcmp (ref_buffer, check_buffer, this_time) == 0, error);
		if (memcmp (ref_buffer, check_buffer, this_time)) {
			break;
		}

		N -= this_time;
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
simple_picture (boost::filesystem::path path, string suffix)
{
	dcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	shared_ptr<dcp::MonoPictureAsset> mp (new dcp::MonoPictureAsset (dcp::Fraction (24, 1), dcp::SMPTE));
	mp->set_metadata (mxf_meta);
	shared_ptr<dcp::PictureAssetWriter> picture_writer = mp->start_write (path / dcp::String::compose("video%1.mxf", suffix), false);
	dcp::File j2c ("test/data/flat_red.j2c");
	for (int i = 0; i < 24; ++i) {
		picture_writer->write (j2c.data (), j2c.size ());
	}
	picture_writer->finalize ();

	return mp;
}


shared_ptr<dcp::SoundAsset>
simple_sound (boost::filesystem::path path, string suffix, dcp::MXFMetadata mxf_meta, string language)
{
	/* Set a valid language, then overwrite it, so that the language parameter can be badly formed */
	shared_ptr<dcp::SoundAsset> ms (new dcp::SoundAsset(dcp::Fraction(24, 1), 48000, 1, dcp::LanguageTag("en-US"), dcp::SMPTE));
	ms->_language = language;
	ms->set_metadata (mxf_meta);
	vector<dcp::Channel> active_channels;
	active_channels.push_back (dcp::LEFT);
	shared_ptr<dcp::SoundAssetWriter> sound_writer = ms->start_write (path / dcp::String::compose("audio%1.mxf", suffix), active_channels);

	SF_INFO info;
	info.format = 0;
	SNDFILE* sndfile = sf_open ("test/data/1s_24-bit_48k_silence.wav", SFM_READ, &info);
	BOOST_CHECK (sndfile);
	float buffer[4096*6];
	float* channels[1];
	channels[0] = buffer;
	while (true) {
		sf_count_t N = sf_readf_float (sndfile, buffer, 4096);
		sound_writer->write (channels, N);
		if (N < 4096) {
			break;
		}
	}

	sound_writer->finalize ();

	return ms;
}


shared_ptr<dcp::DCP>
make_simple (boost::filesystem::path path, int reels)
{
	/* Some known metadata */
	dcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	boost::filesystem::remove_all (path);
	boost::filesystem::create_directories (path);
	shared_ptr<dcp::DCP> d (new dcp::DCP (path));
	shared_ptr<dcp::CPL> cpl (new dcp::CPL ("A Test DCP", dcp::FEATURE));
	cpl->set_annotation_text ("A Test DCP");
	cpl->set_issuer ("OpenDCP 0.0.25");
	cpl->set_creator ("OpenDCP 0.0.25");
	cpl->set_issue_date ("2012-07-17T04:45:18+00:00");
	cpl->set_content_version (
		dcp::ContentVersion("urn:uuid:75ac29aa-42ac-1234-ecae-49251abefd11", "content-version-label-text")
		);

	for (int i = 0; i < reels; ++i) {
		string suffix = reels == 1 ? "" : dcp::String::compose("%1", i);

		shared_ptr<dcp::MonoPictureAsset> mp = simple_picture (path, suffix);
		shared_ptr<dcp::SoundAsset> ms = simple_sound (path, suffix, mxf_meta, "en-US");

		cpl->add (shared_ptr<dcp::Reel> (
				  new dcp::Reel (
					  shared_ptr<dcp::ReelMonoPictureAsset>(new dcp::ReelMonoPictureAsset(mp, 0)),
					  shared_ptr<dcp::ReelSoundAsset>(new dcp::ReelSoundAsset(ms, 0))
					  )
				  ));
	}

	d->add (cpl);
	return d;
}


shared_ptr<dcp::Subtitle>
simple_subtitle ()
{
	return shared_ptr<dcp::Subtitle>(
		new dcp::SubtitleString(
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
			dcp::HALIGN_CENTER,
			0.8,
			dcp::VALIGN_TOP,
			dcp::DIRECTION_LTR,
			"Hello world",
			dcp::NONE,
			dcp::Colour(255, 255, 255),
			dcp::Time(),
			dcp::Time()
			)
		);
}


shared_ptr<dcp::DCP>
make_simple_with_interop_subs (boost::filesystem::path path)
{
	shared_ptr<dcp::DCP> dcp = make_simple (path);

	shared_ptr<dcp::InteropSubtitleAsset> subs(new dcp::InteropSubtitleAsset());
	subs->add (simple_subtitle());

	boost::filesystem::create_directory (path / "subs");
	dcp::ArrayData data(4096);
	subs->add_font ("afont", data);
	subs->write (path / "subs" / "subs.xml");

	shared_ptr<dcp::ReelSubtitleAsset> reel_subs(new dcp::ReelSubtitleAsset(subs, dcp::Fraction(24, 1), 240, 0));
	dcp->cpls().front()->reels().front()->add (reel_subs);

	return dcp;
}


shared_ptr<dcp::DCP>
make_simple_with_smpte_subs (boost::filesystem::path path)
{
	shared_ptr<dcp::DCP> dcp = make_simple (path);

	shared_ptr<dcp::SMPTESubtitleAsset> subs(new dcp::SMPTESubtitleAsset());
	subs->add (simple_subtitle());

	dcp::ArrayData data(4096);
	subs->write (path / "subs.mxf");

	shared_ptr<dcp::ReelSubtitleAsset> reel_subs(new dcp::ReelSubtitleAsset(subs, dcp::Fraction(24, 1), 240, 0));
	dcp->cpls().front()->reels().front()->add (reel_subs);

	return dcp;
}


shared_ptr<dcp::DCP>
make_simple_with_interop_ccaps (boost::filesystem::path path)
{
	shared_ptr<dcp::DCP> dcp = make_simple (path);

	shared_ptr<dcp::InteropSubtitleAsset> subs(new dcp::InteropSubtitleAsset());
	subs->add (simple_subtitle());
	subs->write (path / "ccap.xml");

	shared_ptr<dcp::ReelClosedCaptionAsset> reel_caps(new dcp::ReelClosedCaptionAsset(subs, dcp::Fraction(24, 1), 240, 0));
	dcp->cpls().front()->reels().front()->add (reel_caps);

	return dcp;
}


shared_ptr<dcp::DCP>
make_simple_with_smpte_ccaps (boost::filesystem::path path)
{
	shared_ptr<dcp::DCP> dcp = make_simple (path);

	shared_ptr<dcp::SMPTESubtitleAsset> subs(new dcp::SMPTESubtitleAsset());
	subs->add (simple_subtitle());
	subs->write (path / "ccap.mxf");

	shared_ptr<dcp::ReelClosedCaptionAsset> reel_caps(new dcp::ReelClosedCaptionAsset(subs, dcp::Fraction(24, 1), 240, 0));
	dcp->cpls().front()->reels().front()->add (reel_caps);

	return dcp;
}


shared_ptr<dcp::OpenJPEGImage>
black_image ()
{
	shared_ptr<dcp::OpenJPEGImage> image(new dcp::OpenJPEGImage(dcp::Size(1998, 1080)));
	int const pixels = 1998 * 1080;
	for (int i = 0; i < 3; ++i) {
		memset (image->data(i), 0, pixels * sizeof(int));
	}
	return image;
}


shared_ptr<dcp::ReelAsset>
black_picture_asset (boost::filesystem::path dir, int frames)
{
	shared_ptr<dcp::OpenJPEGImage> image = black_image ();
	dcp::ArrayData frame = dcp::compress_j2k (image, 100000000, 24, false, false);
	BOOST_REQUIRE (frame.size() < 230000000 / (24 * 8));

	shared_ptr<dcp::MonoPictureAsset> asset(new dcp::MonoPictureAsset(dcp::Fraction(24, 1), dcp::SMPTE));
	boost::filesystem::create_directories (dir);
	shared_ptr<dcp::PictureAssetWriter> writer = asset->start_write (dir / "pic.mxf", true);
	for (int i = 0; i < frames; ++i) {
		writer->write (frame.data(), frame.size());
	}
	writer->finalize ();

	return shared_ptr<dcp::ReelAsset>(new dcp::ReelMonoPictureAsset(asset, 0));
}


BOOST_GLOBAL_FIXTURE (TestConfig);
