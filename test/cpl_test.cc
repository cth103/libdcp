/*
    Copyright (C) 2022 Carl Hetherington <cth@carlh.net>

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


#include "cpl.h"
#include "test.h"
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE(cpl_content_kind_test1)
{
	dcp::CPL cpl("test/data/cpl_content_kind_test1.xml");
	BOOST_CHECK_EQUAL(cpl.content_kind().name(), "feature");
	BOOST_CHECK(!cpl.content_kind().scope());
	cpl.write_xml("build/test/cpl_content_kind_test1.xml", {});
	check_xml(dcp::file_to_string("test/ref/cpl_content_kind_test1.xml"), dcp::file_to_string("build/test/cpl_content_kind_test1.xml"), {});
}


BOOST_AUTO_TEST_CASE(cpl_content_kind_test2)
{
	dcp::CPL cpl("test/data/cpl_content_kind_test2.xml");
	BOOST_CHECK_EQUAL(cpl.content_kind().name(), "clip");
	BOOST_REQUIRE(static_cast<bool>(cpl.content_kind().scope()));
	BOOST_CHECK_EQUAL(*cpl.content_kind().scope(), "http://www.smpte-ra.org/schemas/429-16/2014/CPL-Metadata#scope/content-kind");
	cpl.write_xml("build/test/cpl_content_kind_test2.xml", {});
	check_xml(dcp::file_to_string("test/ref/cpl_content_kind_test2.xml"), dcp::file_to_string("build/test/cpl_content_kind_test2.xml"), {});
}


BOOST_AUTO_TEST_CASE(cpl_content_kind_test3)
{
	dcp::CPL cpl("test/data/cpl_content_kind_test3.xml");
	BOOST_CHECK_EQUAL(cpl.content_kind().name(), "tangoadvert");
	BOOST_REQUIRE(static_cast<bool>(cpl.content_kind().scope()));
	BOOST_CHECK_EQUAL(*cpl.content_kind().scope(), "youvebeentangoed");
	cpl.write_xml("build/test/cpl_content_kind_test3.xml", {});
	check_xml(dcp::file_to_string("test/ref/cpl_content_kind_test3.xml"), dcp::file_to_string("build/test/cpl_content_kind_test3.xml"), {});
}


BOOST_AUTO_TEST_CASE(interop_cpl_with_metadata_test)
{
	dcp::CPL cpl(private_test / "CPL_f383c150-5021-4110-9aae-64da6c1ffbdb.xml");
	/* The main thing is that the CPL read doesn't throw, but let's just check one thing for luck */
	BOOST_CHECK_EQUAL(cpl.annotation_text().get_value_or(""), "EyeLeader2sec_XSN_F-133_XX-XX_MOS_4K_20230124_EYE_IOP_OV");
}

