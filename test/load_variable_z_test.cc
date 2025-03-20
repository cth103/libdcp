/*
    Copyright (C) 2025 Carl Hetherington <cth@carlh.net>

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


#include "load_variable_z.h"
#include "smpte_text_asset.h"
#include "warnings.h"
#include "compose.hpp"
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <boost/test/unit_test.hpp>


using std::string;


static
dcp::LoadVariableZ
create(string id, string content)
{
	cxml::Document doc("LoadVariableZ");
	doc.read_string(dcp::String::compose("<LoadVariableZ ID=\"%1\">%2</LoadVariableZ>", id, content));
	return dcp::LoadVariableZ(dynamic_cast<xmlpp::Element const*>(doc.node()));

}


static
string
xml(dcp::LoadVariableZ z)
{
	xmlpp::Document doc;
	z.as_xml(doc.create_root_node("LoadVariableZ"));
	return doc.write_to_string();
}


BOOST_AUTO_TEST_CASE(variable_z_test)
{
	for (auto bad: { "", "-4.2 hello", "1:2:3", "-6.4:0", "-6.2:" }) {
		dcp::LoadVariableZ test = create("foo", bad);
		BOOST_CHECK_MESSAGE(!test.valid(), bad);
		BOOST_CHECK_EQUAL(xml(test), dcp::String::compose("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<LoadVariableZ ID=\"foo\">%1</LoadVariableZ>\n", bad));
	}

	dcp::LoadVariableZ good = create("bar", "   -1.4  4.6:2 \t 9.1:9\n");
	BOOST_CHECK(good.valid());
	BOOST_CHECK_EQUAL(good.positions().size(), 3U);
	BOOST_CHECK_CLOSE(good.positions()[0].position, -1.4, 0.1);
	BOOST_CHECK_EQUAL(good.positions()[0].duration, 1);
	BOOST_CHECK_CLOSE(good.positions()[1].position, 4.6, 0.1);
	BOOST_CHECK_EQUAL(good.positions()[1].duration, 2);
	BOOST_CHECK_CLOSE(good.positions()[2].position, 9.1, 0.1);
	BOOST_CHECK_EQUAL(good.positions()[2].duration, 9);
	BOOST_CHECK_EQUAL(xml(good), "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<LoadVariableZ ID=\"bar\">-1.4 4.6:2 9.1:9</LoadVariableZ>\n");

	dcp::LoadVariableZ made("baz");
	BOOST_CHECK(!made.valid());
	made.set_positions({{-0.6, 2}, {4.2, 9}, {5, 1}});
	BOOST_CHECK_EQUAL(xml(made), "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<LoadVariableZ ID=\"baz\">-0.6:2 4.2:9 5.0</LoadVariableZ>\n");
}


BOOST_AUTO_TEST_CASE(variable_z_pass_through)
{
	dcp::SMPTETextAsset asset("test/data/subtitles_with_vZani.xml");
	BOOST_CHECK_EQUAL(asset.xml_as_string(), dcp::file_to_string("test/data/subtitles_with_vZani_parsed.xml"));
}


