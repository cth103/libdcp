/*
    Copyright (C) 2020 Carl Hetherington <cth@carlh.net>

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
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>


using std::list;
using std::string;
using std::vector;
using boost::optional;
using boost::shared_ptr;


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
dump_notes (list<dcp::VerificationNote> const & notes)
{
	BOOST_FOREACH (dcp::VerificationNote i, notes) {
		std::cout << dcp::note_to_string(i) << "\n";
	}
}


static
void
check_no_errors (boost::filesystem::path path)
{
	vector<boost::filesystem::path> directories;
	directories.push_back (path);
	list<dcp::VerificationNote> notes = dcp::verify (directories, &stage, &progress, xsd_test);
	dump_notes (notes);
	BOOST_CHECK (notes.empty());
}


template <class T>
shared_ptr<T>
pointer_to_id_in_list (shared_ptr<T> needle, list<shared_ptr<T> > haystack)
{
	BOOST_FOREACH (shared_ptr<T> i, haystack) {
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

	BOOST_FOREACH (boost::filesystem::path i, inputs)
	{
		dcp::DCP input_dcp (i);
		input_dcp.read ();

		BOOST_REQUIRE (input_dcp.cpls().size() == 1);
		shared_ptr<dcp::CPL> input_cpl = input_dcp.cpls().front();

		shared_ptr<dcp::CPL> output_cpl = pointer_to_id_in_list (input_cpl, output_dcp.cpls());
		BOOST_REQUIRE (output_cpl);

		BOOST_FOREACH (shared_ptr<dcp::Asset> i, input_dcp.assets(true)) {
			shared_ptr<dcp::Asset> o = pointer_to_id_in_list(i, output_dcp.assets());
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
	dcp::combine (inputs, out);

	check_no_errors (out);
	check_combined (inputs, out);
}


BOOST_AUTO_TEST_CASE (combine_two_dcps_with_same_asset_filenames_test)
{
	using namespace boost::algorithm;
	using namespace boost::filesystem;
	boost::filesystem::path const out = "build/test/combine_two_dcps_with_same_asset_filenames_test";

	shared_ptr<dcp::DCP> second = make_simple ("build/test/combine_input2");
	second->write_xml (dcp::SMPTE);

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

	shared_ptr<dcp::DCP> first = make_simple_with_interop_subs ("build/test/combine_input1");
	first->write_xml (dcp::INTEROP);

	shared_ptr<dcp::DCP> second = make_simple_with_interop_subs ("build/test/combine_input2");
	second->write_xml (dcp::INTEROP);

	remove_all (out);
	vector<path> inputs;
	inputs.push_back ("build/test/combine_input1");
	inputs.push_back ("build/test/combine_input2");
	dcp::combine (inputs, out);

	check_no_errors (out);
	check_combined (inputs, out);
}


BOOST_AUTO_TEST_CASE (combine_two_dcps_with_smpte_subs_test)
{
	using namespace boost::algorithm;
	using namespace boost::filesystem;
	boost::filesystem::path const out = "build/test/combine_two_dcps_with_smpte_subs_test";

	shared_ptr<dcp::DCP> first = make_simple_with_smpte_subs ("build/test/combine_input1");
	first->write_xml (dcp::SMPTE);

	shared_ptr<dcp::DCP> second = make_simple_with_smpte_subs ("build/test/combine_input2");
	second->write_xml (dcp::SMPTE);

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

	shared_ptr<dcp::DCP> first = make_simple_with_interop_ccaps ("build/test/combine_input1");
	first->write_xml (dcp::INTEROP);

	shared_ptr<dcp::DCP> second = make_simple_with_interop_ccaps ("build/test/combine_input2");
	second->write_xml (dcp::INTEROP);

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

	shared_ptr<dcp::DCP> first = make_simple_with_smpte_ccaps ("build/test/combine_input1");
	first->write_xml (dcp::SMPTE);

	shared_ptr<dcp::DCP> second = make_simple_with_smpte_ccaps ("build/test/combine_input2");
	second->write_xml (dcp::SMPTE);

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

	shared_ptr<dcp::DCP> first = make_simple ("build/test/combine_input1", 4);
	first->write_xml (dcp::SMPTE);

	shared_ptr<dcp::DCP> second = make_simple ("build/test/combine_input2", 4);
	second->write_xml (dcp::SMPTE);

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

	shared_ptr<dcp::DCP> first = make_simple ("build/test/combine_input1", 1);
	first->write_xml (dcp::SMPTE);

	remove_all ("build/test/combine_input2");
	shared_ptr<dcp::DCP> second(new dcp::DCP("build/test/combine_input2"));

	dcp::MXFMetadata mxf_meta;
	mxf_meta.company_name = "OpenDCP";
	mxf_meta.product_version = "0.0.25";

	shared_ptr<dcp::CPL> cpl (new dcp::CPL("A Test DCP", dcp::FEATURE));
	cpl->set_content_version (
		dcp::ContentVersion("urn:uuid:75ac29aa-42ac-1234-ecae-49251abefd11","content-version-label-text")
		);

	shared_ptr<dcp::ReelMonoPictureAsset> pic(new dcp::ReelMonoPictureAsset(simple_picture("build/test/combine_input2", ""), 0));
	shared_ptr<dcp::ReelSoundAsset> sound(new dcp::ReelSoundAsset(first->cpls().front()->reels().front()->main_sound()->asset(), 0));
	cpl->add (shared_ptr<dcp::Reel>(new dcp::Reel(pic, sound)));
	second->add (cpl);
	second->write_xml (dcp::SMPTE);

	remove_all (out);
	vector<path> inputs;
	inputs.push_back ("build/test/combine_input1");
	inputs.push_back ("build/test/combine_input2");
	dcp::combine (inputs, out);

	check_no_errors (out);
	check_combined (inputs, out);
}


/* XXX: same CPL names */
/* XXX: Interop PNG subs */
