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


#include "combine.h"
#include "cpl.h"
#include "dcp.h"
#include "interop_subtitle_asset.h"
#include "reel_subtitle_asset.h"
#include "reel_mono_picture_asset.h"
#include "reel_sound_asset.h"
#include "test.h"
#include "types.h"
#include "verify.h"
#include "reel_markers_asset.h"
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>


using std::list;
using std::string;
using std::make_shared;
using std::vector;
using std::shared_ptr;
using boost::optional;


static void
stage (string, optional<boost::filesystem::path>)
{
}


static void
progress (float)
{
}


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
check_no_errors (boost::filesystem::path path)
{
	vector<boost::filesystem::path> directories;
	directories.push_back (path);
	auto notes = dcp::verify (directories, &stage, &progress, xsd_test);
	vector<dcp::VerificationNote> filtered_notes;
	std::copy_if (notes.begin(), notes.end(), std::back_inserter(filtered_notes), [](dcp::VerificationNote const& i) {
		return i.code() != dcp::VerificationNote::Code::INVALID_STANDARD && i.code() != dcp::VerificationNote::Code::INVALID_SUBTITLE_DURATION;
	});
	dump_notes (filtered_notes);
	BOOST_CHECK (filtered_notes.empty());
}


template <class T>
shared_ptr<T>
pointer_to_id_in_vector (shared_ptr<T> needle, vector<shared_ptr<T>> haystack)
{
	for (auto i: haystack) {
		if (i->id() == needle->id()) {
 			return i;
		}
	}

	return shared_ptr<T>();
}


static
void
note_handler (dcp::NoteType, std::string)
{
	// std::cout << "> " << n << "\n";
}


static
void
check_combined (vector<boost::filesystem::path> inputs, boost::filesystem::path output)
{
	dcp::DCP output_dcp (output);
	output_dcp.read ();

	dcp::EqualityOptions options;
	options.load_font_nodes_can_differ = true;

	for (auto i: inputs) {
		dcp::DCP input_dcp (i);
		input_dcp.read ();

		BOOST_REQUIRE (input_dcp.cpls().size() == 1);
		auto input_cpl = input_dcp.cpls().front();

		auto output_cpl = pointer_to_id_in_vector (input_cpl, output_dcp.cpls());
		BOOST_REQUIRE (output_cpl);

		for (auto i: input_dcp.assets(true)) {
			auto o = pointer_to_id_in_vector(i, output_dcp.assets());
			BOOST_REQUIRE_MESSAGE (o, "Could not find " << i->id() << " in combined DCP.");
			BOOST_CHECK (i->equals(o, options, note_handler));
		}
	}
}


BOOST_AUTO_TEST_CASE (combine_single_dcp_test)
{
	using namespace boost::algorithm;
	using namespace boost::filesystem;
	boost::filesystem::path const out = "build/test/combine_single_dcp_test";

	remove_all (out);
	vector<path> inputs;
	inputs.push_back ("test/ref/DCP/dcp_test1");
	dcp::combine (
		inputs,
		out,
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::String::compose("libdcp %1", dcp::version),
		dcp::LocalTime().as_string(),
		"A Test DCP"
		);

	check_no_errors (out);
	check_combined (inputs, out);
}


BOOST_AUTO_TEST_CASE (combine_two_dcps_with_same_asset_filenames_test)
{
	using namespace boost::algorithm;
	using namespace boost::filesystem;
	boost::filesystem::path const out = "build/test/combine_two_dcps_with_same_asset_filenames_test";

	auto second = make_simple ("build/test/combine_input2");
	second->write_xml ();

	remove_all (out);
	vector<path> inputs;
	inputs.push_back ("test/ref/DCP/dcp_test1");
	inputs.push_back ("build/test/combine_input2");
	dcp::combine (inputs, out);

	check_no_errors (out);
	check_combined (inputs, out);
}


BOOST_AUTO_TEST_CASE (combine_two_dcps_with_interop_subs_test)
{
	using namespace boost::algorithm;
	using namespace boost::filesystem;
	boost::filesystem::path const out = "build/test/combine_two_dcps_with_interop_subs_test";

	auto first = make_simple_with_interop_subs ("build/test/combine_input1");
	first->write_xml ();

	auto second = make_simple_with_interop_subs ("build/test/combine_input2");
	second->write_xml ();

	remove_all (out);
	vector<path> inputs = {"build/test/combine_input1", "build/test/combine_input2"};
	dcp::combine (inputs, out);

	check_no_errors (out);
	check_combined (inputs, out);
}


BOOST_AUTO_TEST_CASE (combine_two_dcps_with_smpte_subs_test)
{
	using namespace boost::algorithm;
	using namespace boost::filesystem;
	boost::filesystem::path const out = "build/test/combine_two_dcps_with_smpte_subs_test";

	auto first = make_simple_with_smpte_subs ("build/test/combine_input1");
	first->write_xml ();

	auto second = make_simple_with_smpte_subs ("build/test/combine_input2");
	second->write_xml ();

	remove_all (out);
	vector<path> inputs;
	inputs.push_back ("build/test/combine_input1");
	inputs.push_back ("build/test/combine_input2");
	dcp::combine (inputs, out);

	check_no_errors (out);
	check_combined (inputs, out);
}


BOOST_AUTO_TEST_CASE (combine_two_dcps_with_interop_ccaps_test)
{
	using namespace boost::algorithm;
	using namespace boost::filesystem;
	boost::filesystem::path const out = "build/test/combine_two_dcps_with_interop_ccaps_test";

	auto first = make_simple_with_interop_ccaps ("build/test/combine_input1");
	first->write_xml ();

	auto second = make_simple_with_interop_ccaps ("build/test/combine_input2");
	second->write_xml ();

	remove_all (out);
	vector<path> inputs;
	inputs.push_back ("build/test/combine_input1");
	inputs.push_back ("build/test/combine_input2");
	dcp::combine (inputs, out);

	check_no_errors (out);
	check_combined (inputs, out);
}


BOOST_AUTO_TEST_CASE (combine_two_dcps_with_smpte_ccaps_test)
{
	using namespace boost::algorithm;
	using namespace boost::filesystem;
	boost::filesystem::path const out = "build/test/combine_two_dcps_with_interop_ccaps_test";

	auto first = make_simple_with_smpte_ccaps ("build/test/combine_input1");
	first->write_xml ();

	auto second = make_simple_with_smpte_ccaps ("build/test/combine_input2");
	second->write_xml ();

	remove_all (out);
	vector<path> inputs;
	inputs.push_back ("build/test/combine_input1");
	inputs.push_back ("build/test/combine_input2");
	dcp::combine (inputs, out);

	check_no_errors (out);
	check_combined (inputs, out);
}


BOOST_AUTO_TEST_CASE (combine_two_multi_reel_dcps)
{
	using namespace boost::algorithm;
	using namespace boost::filesystem;
	boost::filesystem::path const out = "build/test/combine_two_multi_reel_dcps";

	auto first = make_simple ("build/test/combine_input1", 4);
	first->write_xml ();

	auto second = make_simple ("build/test/combine_input2", 4);
	second->write_xml ();

	remove_all (out);
	vector<path> inputs;
	inputs.push_back ("build/test/combine_input1");
	inputs.push_back ("build/test/combine_input2");
	dcp::combine (inputs, out);

	check_no_errors (out);
	check_combined (inputs, out);
}


BOOST_AUTO_TEST_CASE (combine_two_dcps_with_shared_asset)
{
	using namespace boost::filesystem;
	boost::filesystem::path const out = "build/test/combine_two_dcps_with_shared_asset";

	auto first = make_simple ("build/test/combine_input1", 1);
	first->write_xml ();

	remove_all ("build/test/combine_input2");
	auto second = make_shared<dcp::DCP>("build/test/combine_input2");

	dcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
	cpl->set_content_version (
		dcp::ContentVersion("urn:uuid:75ac29aa-42ac-1234-ecae-49251abefd11","content-version-label-text")
		);
	cpl->set_main_sound_configuration ("L,C,R,Lfe,-,-");
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));
	cpl->set_version_number(1);

	auto pic = make_shared<dcp::ReelMonoPictureAsset>(simple_picture("build/test/combine_input2", ""), 0);
	auto sound = make_shared<dcp::ReelSoundAsset>(first->cpls().front()->reels().front()->main_sound()->asset(), 0);
	auto reel = make_shared<dcp::Reel>(pic, sound);
	reel->add (simple_markers());
	cpl->add (reel);
	second->add (cpl);
	second->write_xml ();

	remove_all (out);
	vector<path> inputs;
	inputs.push_back ("build/test/combine_input1");
	inputs.push_back ("build/test/combine_input2");
	dcp::combine (inputs, out);

	check_no_errors (out);
	check_combined (inputs, out);
}


/** Two DCPs each with a copy of the exact same asset */
BOOST_AUTO_TEST_CASE (combine_two_dcps_with_duplicated_asset)
{
	using namespace boost::filesystem;
	boost::filesystem::path const out = "build/test/combine_two_dcps_with_duplicated_asset";

	auto first = make_simple ("build/test/combine_input1", 1);
	first->write_xml ();

	remove_all ("build/test/combine_input2");
	auto second = make_shared<dcp::DCP>("build/test/combine_input2");

	dcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	auto cpl = make_shared<dcp::CPL>("A Test DCP", dcp::ContentKind::TRAILER, dcp::Standard::SMPTE);
	cpl->set_content_version (
		dcp::ContentVersion("urn:uuid:75ac29aa-42ac-1234-ecae-49251abefd11","content-version-label-text")
		);
	cpl->set_main_sound_configuration ("L,C,R,Lfe,-,-");
	cpl->set_main_sound_sample_rate (48000);
	cpl->set_main_picture_stored_area (dcp::Size(1998, 1080));
	cpl->set_main_picture_active_area (dcp::Size(1440, 1080));
	cpl->set_version_number(1);

	auto pic = make_shared<dcp::ReelMonoPictureAsset>(simple_picture("build/test/combine_input2", ""), 0);
	auto first_sound_asset = first->cpls()[0]->reels()[0]->main_sound()->asset()->file();
	BOOST_REQUIRE (first_sound_asset);
	boost::filesystem::path second_sound_asset = "build/test/combine_input2/my_great_audio.mxf";
	boost::filesystem::copy_file (*first_sound_asset, second_sound_asset);
	auto sound = make_shared<dcp::ReelSoundAsset>(make_shared<dcp::SoundAsset>(second_sound_asset), 0);
	auto reel = make_shared<dcp::Reel>(pic, sound);
	reel->add (simple_markers());
	cpl->add (reel);
	second->add (cpl);
	second->write_xml ();

	remove_all (out);
	vector<path> inputs;
	inputs.push_back ("build/test/combine_input1");
	inputs.push_back ("build/test/combine_input2");
	dcp::combine (inputs, out);

	check_no_errors (out);
	check_combined (inputs, out);

	BOOST_REQUIRE (!boost::filesystem::exists(out / "my_great_audio.mxf"));
}


BOOST_AUTO_TEST_CASE (check_cpls_unchanged_after_combine)
{
	boost::filesystem::path in = "build/test/combine_one_dcp_with_composition_metadata_in";
	boost::filesystem::path out = "build/test/combine_one_dcp_with_composition_metadata_out";
	auto dcp = make_simple (in);
	dcp->write_xml ();

	dcp::combine ({in}, out);

	BOOST_REQUIRE_EQUAL (dcp->cpls().size(), 1U);
	auto cpl = dcp->cpls()[0]->file();
	BOOST_REQUIRE (cpl);
	check_file (*cpl, out / cpl->filename());
}


/* XXX: same CPL names */
/* XXX: Interop PNG subs */
