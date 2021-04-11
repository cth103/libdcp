/*
    Copyright (C) 2020-2021 Carl Hetherington <cth@carlh.net>

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


#include "certificate_chain.h"
#include "cpl.h"
#include "exceptions.h"
#include "language_tag.h"
#include "reel.h"
#include "reel_smpte_subtitle_asset.h"
#include "stream_operators.h"
#include "test.h"
#include <memory>
#include <boost/test/unit_test.hpp>


using std::list;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::vector;


BOOST_AUTO_TEST_CASE (cpl_metadata_bad_values_test)
{
	dcp::CPL cpl("", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	BOOST_CHECK_THROW (cpl.set_version_number(-1), dcp::BadSettingError);

	vector<dcp::ContentVersion> cv = {
		dcp::ContentVersion("same-id", "version 1"),
		dcp::ContentVersion("same-id", "version 2")
	};
	BOOST_CHECK_THROW (cpl.set_content_versions(cv), dcp::DuplicateIdError);
}


BOOST_AUTO_TEST_CASE (main_sound_configuration_test1)
{
	dcp::MainSoundConfiguration msc("51/L,R,C,LFE,-,-");
	BOOST_CHECK_EQUAL (msc.to_string(), "51/L,R,C,LFE,-,-");
	BOOST_CHECK_EQUAL (msc.channels(), 6);
	BOOST_CHECK_EQUAL (msc.field(), dcp::MCASoundField::FIVE_POINT_ONE);
	BOOST_CHECK_EQUAL (msc.mapping(0).get(), dcp::Channel::LEFT);
	BOOST_CHECK_EQUAL (msc.mapping(1).get(), dcp::Channel::RIGHT);
	BOOST_CHECK_EQUAL (msc.mapping(2).get(), dcp::Channel::CENTRE);
	BOOST_CHECK_EQUAL (msc.mapping(3).get(), dcp::Channel::LFE);
	BOOST_CHECK (!msc.mapping(4));
	BOOST_CHECK (!msc.mapping(5));
}


BOOST_AUTO_TEST_CASE (main_sound_configuration_test2)
{
	dcp::MainSoundConfiguration msc("71/L,R,C,LFE,-,-");
	BOOST_CHECK_EQUAL (msc.to_string(), "71/L,R,C,LFE,-,-");
	BOOST_CHECK_EQUAL (msc.channels(), 6);
	BOOST_CHECK_EQUAL (msc.field(), dcp::MCASoundField::SEVEN_POINT_ONE);
	BOOST_CHECK_EQUAL (msc.mapping(0).get(), dcp::Channel::LEFT);
	BOOST_CHECK_EQUAL (msc.mapping(1).get(), dcp::Channel::RIGHT);
	BOOST_CHECK_EQUAL (msc.mapping(2).get(), dcp::Channel::CENTRE);
	BOOST_CHECK_EQUAL (msc.mapping(3).get(), dcp::Channel::LFE);
	BOOST_CHECK (!msc.mapping(4));
	BOOST_CHECK (!msc.mapping(5));
}


BOOST_AUTO_TEST_CASE (main_sound_configuration_test3)
{
	dcp::MainSoundConfiguration msc("71/L,-,C,LFE,Lss,Rss");
	BOOST_CHECK_EQUAL (msc.to_string(), "71/L,-,C,LFE,Lss,Rss");
	BOOST_CHECK_EQUAL (msc.channels(), 6);
	BOOST_CHECK_EQUAL (msc.field(), dcp::MCASoundField::SEVEN_POINT_ONE);
	BOOST_CHECK_EQUAL (msc.mapping(0).get(), dcp::Channel::LEFT);
	BOOST_CHECK (!msc.mapping(1));
	BOOST_CHECK_EQUAL (msc.mapping(2).get(), dcp::Channel::CENTRE);
	BOOST_CHECK_EQUAL (msc.mapping(3).get(), dcp::Channel::LFE);
	BOOST_CHECK_EQUAL (msc.mapping(4).get(), dcp::Channel::LS);
	BOOST_CHECK_EQUAL (msc.mapping(5).get(), dcp::Channel::RS);
}


BOOST_AUTO_TEST_CASE (main_sound_configuration_test4)
{
	dcp::MainSoundConfiguration msc("71/L,-,C,LFE,Lss,Rss,-,-,-,-,-,-,-,-,-");
	BOOST_CHECK_EQUAL (msc.to_string(), "71/L,-,C,LFE,Lss,Rss,-,-,-,-,-,-,-,-,-");
	BOOST_CHECK_EQUAL (msc.channels(), 15);
	BOOST_CHECK_EQUAL (msc.field(), dcp::MCASoundField::SEVEN_POINT_ONE);
	BOOST_CHECK_EQUAL (msc.mapping(0).get(), dcp::Channel::LEFT);
	BOOST_CHECK (!msc.mapping(1));
	BOOST_CHECK_EQUAL (msc.mapping(2).get(), dcp::Channel::CENTRE);
	BOOST_CHECK_EQUAL (msc.mapping(3).get(), dcp::Channel::LFE);
	BOOST_CHECK_EQUAL (msc.mapping(4).get(), dcp::Channel::LS);
	BOOST_CHECK_EQUAL (msc.mapping(5).get(), dcp::Channel::RS);
	for (int i = 6; i < 15; ++i) {
		BOOST_CHECK (!msc.mapping(i));
	}
}


BOOST_AUTO_TEST_CASE (main_sound_configuration_test5)
{
	dcp::MainSoundConfiguration msc("71/L,-,C,LFE,Lss,Rss,HI,VIN,-,-,Lrs,Rrs,DBOX,FSKSync,SLVS");
	BOOST_CHECK_EQUAL (msc.to_string(), "71/L,-,C,LFE,Lss,Rss,HI,VIN,-,-,Lrs,Rrs,DBOX,FSKSync,SLVS");
	BOOST_CHECK_EQUAL (msc.channels(), 15);
	BOOST_CHECK_EQUAL (msc.field(), dcp::MCASoundField::SEVEN_POINT_ONE);
	BOOST_CHECK_EQUAL (msc.mapping(0).get(), dcp::Channel::LEFT);
	BOOST_CHECK (!msc.mapping(1));
	BOOST_CHECK_EQUAL (msc.mapping(2).get(), dcp::Channel::CENTRE);
	BOOST_CHECK_EQUAL (msc.mapping(3).get(), dcp::Channel::LFE);
	BOOST_CHECK_EQUAL (msc.mapping(4).get(), dcp::Channel::LS);
	BOOST_CHECK_EQUAL (msc.mapping(5).get(), dcp::Channel::RS);
	BOOST_CHECK_EQUAL (msc.mapping(6).get(), dcp::Channel::HI);
	BOOST_CHECK_EQUAL (msc.mapping(7).get(), dcp::Channel::VI);
	BOOST_CHECK (!msc.mapping(8));
	BOOST_CHECK (!msc.mapping(9));
	BOOST_CHECK_EQUAL (msc.mapping(10).get(), dcp::Channel::BSL);
	BOOST_CHECK_EQUAL (msc.mapping(11).get(), dcp::Channel::BSR);
	BOOST_CHECK_EQUAL (msc.mapping(12).get(), dcp::Channel::MOTION_DATA);
	BOOST_CHECK_EQUAL (msc.mapping(13).get(), dcp::Channel::SYNC_SIGNAL);
	BOOST_CHECK_EQUAL (msc.mapping(14).get(), dcp::Channel::SIGN_LANGUAGE);
}


BOOST_AUTO_TEST_CASE (luminance_test1)
{
	BOOST_CHECK_NO_THROW (dcp::Luminance(4, dcp::Luminance::Unit::CANDELA_PER_SQUARE_METRE));
	BOOST_CHECK_THROW (dcp::Luminance(-4, dcp::Luminance::Unit::CANDELA_PER_SQUARE_METRE), dcp::MiscError);
}


BOOST_AUTO_TEST_CASE (luminance_test2)
{
	auto doc = make_shared<cxml::Document>("Luminance");

	doc->read_string (
		"<Luminance units=\"candela-per-square-metre\">4.5</Luminance>"
		);

	dcp::Luminance lum (doc);
	BOOST_CHECK (lum.unit() == dcp::Luminance::Unit::CANDELA_PER_SQUARE_METRE);
	BOOST_CHECK_CLOSE (lum.value(), 4.5, 0.1);
}


BOOST_AUTO_TEST_CASE (luminance_test3)
{
	auto doc = make_shared<cxml::Document>("Luminance");

	doc->read_string (
		"<Luminance units=\"candela-per-square-motre\">4.5</Luminance>"
		);

	BOOST_CHECK_THROW (new dcp::Luminance(doc), dcp::XMLError);
}


BOOST_AUTO_TEST_CASE (luminance_test4)
{
	auto doc = make_shared<cxml::Document>("Luminance");

	doc->read_string (
		"<Luminance units=\"candela-per-square-metre\">-4.5</Luminance>"
		);

	/* We tolerate out-of-range values when reading from XML */
	dcp::Luminance lum (doc);
	BOOST_CHECK (lum.unit() == dcp::Luminance::Unit::CANDELA_PER_SQUARE_METRE);
	BOOST_CHECK_CLOSE (lum.value(), -4.5, 0.1);
}


/** A test where most CPL metadata is present */
BOOST_AUTO_TEST_CASE (cpl_metadata_read_test1)
{
	dcp::CPL cpl("test/ref/cpl_metadata_test1.xml");

	BOOST_CHECK_EQUAL (cpl.full_content_title_text().get(), "full-content-title");
	BOOST_CHECK (cpl.full_content_title_text_language().get() == "de");
	BOOST_CHECK (cpl.release_territory().get() == "ES");
	BOOST_CHECK_EQUAL (cpl.version_number().get(), 2);
	BOOST_CHECK_EQUAL (cpl.status().get(), dcp::Status::FINAL);
	BOOST_CHECK_EQUAL (cpl.chain().get(), "the-chain");
	BOOST_CHECK_EQUAL (cpl.distributor().get(), "the-distributor");
	BOOST_CHECK_EQUAL (cpl.facility().get(), "the-facility");
	BOOST_CHECK (cpl.luminance() == dcp::Luminance(4.5, dcp::Luminance::Unit::FOOT_LAMBERT));

	dcp::MainSoundConfiguration msc(cpl.main_sound_configuration().get());
	BOOST_CHECK_EQUAL (msc.mapping(0).get(), dcp::Channel::LEFT);
	BOOST_CHECK_EQUAL (msc.mapping(1).get(), dcp::Channel::RIGHT);
	BOOST_CHECK_EQUAL (msc.mapping(2).get(), dcp::Channel::CENTRE);
	BOOST_CHECK_EQUAL (msc.mapping(3).get(), dcp::Channel::LFE);
	BOOST_CHECK (!msc.mapping(4));
	BOOST_CHECK (!msc.mapping(5));
	BOOST_CHECK (!msc.mapping(6));
	BOOST_CHECK (!msc.mapping(7));
	BOOST_CHECK (!msc.mapping(8));
	BOOST_CHECK (!msc.mapping(9));
	BOOST_CHECK (!msc.mapping(10));
	BOOST_CHECK (!msc.mapping(11));
	BOOST_CHECK (!msc.mapping(12));
	BOOST_CHECK_EQUAL (msc.mapping(13).get(), dcp::Channel::SYNC_SIGNAL);

	BOOST_CHECK_EQUAL (cpl.main_sound_sample_rate().get(), 48000);
	BOOST_CHECK (cpl.main_picture_stored_area().get() == dcp::Size(1998, 1080));
	BOOST_CHECK (cpl.main_picture_active_area().get() == dcp::Size(1440, 1080));

	auto reels = cpl.reels ();
	BOOST_REQUIRE_EQUAL (reels.size(), 1);
	BOOST_REQUIRE (reels.front()->main_subtitle()->language());
	BOOST_CHECK_EQUAL (reels.front()->main_subtitle()->language().get(), "de-DE");

	auto asl = cpl.additional_subtitle_languages();
	BOOST_REQUIRE_EQUAL (asl.size(), 2);
	BOOST_CHECK_EQUAL (asl[0], "en-US");
	BOOST_CHECK_EQUAL (asl[1], "fr-ZA");

	BOOST_CHECK (cpl.additional_subtitle_languages() == asl);
}


/** A test where most CPL metadata is present */
BOOST_AUTO_TEST_CASE (cpl_metadata_write_test1)
{
	RNGFixer fix;

	dcp::CPL cpl("", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	cpl.set_issue_date ("2020-08-28T13:35:06+02:00");

	vector<dcp::ContentVersion> cv = {
		dcp::ContentVersion("some-id", "version 1"),
		dcp::ContentVersion("another-id", "version 2")
	};;
	cpl.set_content_versions (cv);

	cpl.set_full_content_title_text ("full-content-title");
	cpl.set_full_content_title_text_language (dcp::LanguageTag("de"));
	cpl.set_release_territory (dcp::LanguageTag::RegionSubtag("ES"));
	cpl.set_version_number (2);
	cpl.set_status (dcp::Status::FINAL);
	cpl.set_chain ("the-chain");
	cpl.set_distributor ("the-distributor");
	cpl.set_facility ("the-facility");
	cpl.set_luminance (dcp::Luminance(4.5, dcp::Luminance::Unit::FOOT_LAMBERT));
	cpl.set_issuer ("libdcp1.6.4devel");
	cpl.set_creator ("libdcp1.6.4devel");

	dcp::MainSoundConfiguration msc(dcp::MCASoundField::SEVEN_POINT_ONE, 16);
	msc.set_mapping (0, dcp::Channel::LEFT);
	msc.set_mapping (1, dcp::Channel::RIGHT);
	msc.set_mapping (2, dcp::Channel::CENTRE);
	msc.set_mapping (3, dcp::Channel::LFE);
	msc.set_mapping (13, dcp::Channel::SYNC_SIGNAL);
	cpl.set_main_sound_configuration (msc.to_string());

	cpl.set_main_sound_sample_rate (48000);
	cpl.set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl.set_main_picture_active_area (dcp::Size(1440, 1080));

	auto doc = make_shared<cxml::Document>("MainSubtitle");

	doc->read_string (
		"<MainSubtitle>"
		"<Id>urn:uuid:8bca1489-aab1-9259-a4fd-8150abc1de12</Id>"
		"<AnnotationText>Goodbye world!</AnnotationText>"
		"<EditRate>25 1</EditRate>"
		"<IntrinsicDuration>1870</IntrinsicDuration>"
		"<EntryPoint>0</EntryPoint>"
		"<Duration>525</Duration>"
		"<KeyId>urn:uuid:540cbf10-ab14-0233-ab1f-fb31501cabfa</KeyId>"
		"<Hash>3EABjX9BB1CAWhLUtHhrGSyLgOY=</Hash>"
		"<Language>de-DE</Language>"
		"</MainSubtitle>"
		);

	auto reel = make_shared<dcp::Reel>();
	reel->add (black_picture_asset("build/test/cpl_metadata_write_test1"));
	reel->add (make_shared<dcp::ReelSMPTESubtitleAsset>(doc));
	cpl.add (reel);

	vector<dcp::LanguageTag> lt;
	lt.push_back(dcp::LanguageTag("en-US"));
	lt.push_back(dcp::LanguageTag("fr-ZA"));
	cpl.set_additional_subtitle_languages (lt);

	cpl.write_xml ("build/test/cpl_metadata_write_test1.xml", shared_ptr<dcp::CertificateChain>());
	check_xml (
		dcp::file_to_string("test/ref/cpl_metadata_test1.xml"),
		dcp::file_to_string("build/test/cpl_metadata_write_test1.xml"),
		vector<string>()
		);
}


/** A test where most CPL metadata is present */
BOOST_AUTO_TEST_CASE (cpl_metadata_roundtrip_test_1)
{
	dcp::CPL cpl ("test/ref/cpl_metadata_test1.xml");
	cpl.write_xml ("build/test/cpl_metadata_roundtrip_test1.xml", shared_ptr<dcp::CertificateChain>());
	vector<string> ignore;
	ignore.push_back ("Id");
	check_xml (
		dcp::file_to_string("test/ref/cpl_metadata_test1.xml"),
		dcp::file_to_string("build/test/cpl_metadata_roundtrip_test1.xml"),
		ignore
		);
}


/** A test where only a bare minimum of CPL metadata is present */
BOOST_AUTO_TEST_CASE (cpl_metadata_write_test2)
{
	RNGFixer fix;

	dcp::CPL cpl("", dcp::ContentKind::FEATURE, dcp::Standard::SMPTE);
	cpl.set_issue_date ("2020-08-28T13:35:06+02:00");
	cpl.set_content_version (dcp::ContentVersion("id", "version"));
	cpl.set_issuer ("libdcp1.6.4devel");
	cpl.set_creator ("libdcp1.6.4devel");

	dcp::MainSoundConfiguration msc(dcp::MCASoundField::SEVEN_POINT_ONE, 16);
	msc.set_mapping (0, dcp::Channel::LEFT);
	msc.set_mapping (1, dcp::Channel::RIGHT);
	msc.set_mapping (2, dcp::Channel::CENTRE);
	msc.set_mapping (3, dcp::Channel::LFE);
	msc.set_mapping (13, dcp::Channel::SYNC_SIGNAL);
	cpl.set_main_sound_configuration (msc.to_string());

	cpl.set_main_sound_sample_rate (48000);
	cpl.set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl.set_main_picture_active_area (dcp::Size(1440, 1080));

	shared_ptr<dcp::Reel> reel(new dcp::Reel());
	reel->add (black_picture_asset("build/test/cpl_metadata_write_test1"));
	cpl.add (reel);

	cpl.write_xml ("build/test/cpl_metadata_write_test2.xml", shared_ptr<dcp::CertificateChain>());
	check_xml (
		dcp::file_to_string("test/ref/cpl_metadata_test2.xml"),
		dcp::file_to_string("build/test/cpl_metadata_write_test2.xml"),
		vector<string>()
		);
}


/** A test where only a bare minimum of CPL metadata is present */
BOOST_AUTO_TEST_CASE (cpl_metadata_read_test2)
{
	dcp::CPL cpl("test/ref/cpl_metadata_test2.xml");

	BOOST_CHECK_EQUAL (cpl.full_content_title_text().get(), "");
	BOOST_CHECK (!cpl.full_content_title_text_language());
	BOOST_CHECK (!cpl.release_territory());
	BOOST_CHECK (!cpl.version_number());
	BOOST_CHECK (!cpl.status());
	BOOST_CHECK (!cpl.chain());
	BOOST_CHECK (!cpl.distributor());
	BOOST_CHECK (!cpl.facility());
	BOOST_CHECK (!cpl.luminance());

	dcp::MainSoundConfiguration msc(cpl.main_sound_configuration().get());
	BOOST_CHECK_EQUAL (msc.mapping(0).get(), dcp::Channel::LEFT);
	BOOST_CHECK_EQUAL (msc.mapping(1).get(), dcp::Channel::RIGHT);
	BOOST_CHECK_EQUAL (msc.mapping(2).get(), dcp::Channel::CENTRE);
	BOOST_CHECK_EQUAL (msc.mapping(3).get(), dcp::Channel::LFE);
	BOOST_CHECK (!msc.mapping(4));
	BOOST_CHECK (!msc.mapping(5));
	BOOST_CHECK (!msc.mapping(6));
	BOOST_CHECK (!msc.mapping(7));
	BOOST_CHECK (!msc.mapping(8));
	BOOST_CHECK (!msc.mapping(9));
	BOOST_CHECK (!msc.mapping(10));
	BOOST_CHECK (!msc.mapping(11));
	BOOST_CHECK (!msc.mapping(12));
	BOOST_CHECK_EQUAL (msc.mapping(13).get(), dcp::Channel::SYNC_SIGNAL);

	BOOST_CHECK_EQUAL (cpl.main_sound_sample_rate().get(), 48000);
	BOOST_CHECK (cpl.main_picture_stored_area().get() == dcp::Size(1998, 1080));
	BOOST_CHECK (cpl.main_picture_active_area().get() == dcp::Size(1440, 1080));

	auto reels = cpl.reels ();
	BOOST_REQUIRE_EQUAL (reels.size(), 1);
}


/** A test where only a bare minimum of CPL metadata is present */
BOOST_AUTO_TEST_CASE (cpl_metadata_roundtrip_test_2)
{
	dcp::CPL cpl ("test/ref/cpl_metadata_test2.xml");
	cpl.write_xml ("build/test/cpl_metadata_roundtrip_test2.xml", shared_ptr<dcp::CertificateChain>());
	check_xml (
		dcp::file_to_string("test/ref/cpl_metadata_test2.xml"),
		dcp::file_to_string("build/test/cpl_metadata_roundtrip_test2.xml"),
		{"Id"}
		);
}

