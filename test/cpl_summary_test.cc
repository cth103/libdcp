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


#include "dcp.h"
#include "filesystem.h"
#include "test.h"
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE(cpl_summary_test1)
{
	auto const name = "TONEPLATES-SMPTE-PLAINTEXT_TST_F_XX-XX_ITL-TD_51-XX_2K_WOE_20111001_WOE_OV";
	auto const dir = private_test / name;
	dcp::DCP dcp(dir);
	auto cpls = dcp.cpl_summaries();

	BOOST_REQUIRE_EQUAL(cpls.size(), 1U);

	BOOST_CHECK_EQUAL(cpls[0].dcp_directory, dcp::filesystem::canonical(dir));
	BOOST_CHECK_EQUAL(cpls[0].cpl_id, "0435b2ae-741b-4853-ad7c-6014060344aa");
	BOOST_REQUIRE(static_cast<bool>(cpls[0].cpl_annotation_text));
	BOOST_CHECK_EQUAL(*cpls[0].cpl_annotation_text, name);
	BOOST_CHECK_EQUAL(
		cpls[0].cpl_file,
		dcp::filesystem::canonical(dir / "cpl_0435b2ae-741b-4853-ad7c-6014060344aa_.xml")
	);
	BOOST_CHECK(!cpls[0].encrypted);
	BOOST_CHECK(cpls[0].last_write_time > 0);
}


BOOST_AUTO_TEST_CASE(cpl_summary_test2)
{
	auto const name = "TONEPLATES-SMPTE-ENCRYPTED_TST_F_XX-XX_ITL-TD_51-XX_2K_WOE_20111001_WOE_OV";
	auto const dir = private_test / name;
	dcp::DCP dcp(dir);
	auto cpls = dcp.cpl_summaries();

	BOOST_REQUIRE_EQUAL(cpls.size(), 1U);

	BOOST_CHECK_EQUAL(cpls[0].dcp_directory, dcp::filesystem::canonical(dir));
	BOOST_CHECK_EQUAL(cpls[0].cpl_id, "eece17de-77e8-4a55-9347-b6bab5724b9f");
	BOOST_REQUIRE(static_cast<bool>(cpls[0].cpl_annotation_text));
	BOOST_CHECK_EQUAL(*cpls[0].cpl_annotation_text, name);
	BOOST_CHECK_EQUAL(
		cpls[0].cpl_file,
		dcp::filesystem::canonical(dir / "cpl_eece17de-77e8-4a55-9347-b6bab5724b9f_.xml")
	);
	BOOST_CHECK(cpls[0].encrypted);
	BOOST_CHECK(cpls[0].last_write_time > 0);
}


BOOST_AUTO_TEST_CASE(cpl_summary_test3)
{
	auto const name = "data/SMPTE_TST-B1PB2P_S_EN-EN-CCAP_5171-HI-VI_2K_ISDCF_20151123_DPPT_SMPTE_combo/";
	auto const dir = private_test / name;
	dcp::DCP dcp(dir);
	auto cpls = dcp.cpl_summaries();

	BOOST_REQUIRE_EQUAL(cpls.size(), 2U);

	BOOST_CHECK_EQUAL(cpls[0].dcp_directory, dcp::filesystem::canonical(dir));
	BOOST_CHECK_EQUAL(cpls[0].cpl_id, "0f404021-652a-4cca-8a7e-c181c5bb83f9");
	BOOST_REQUIRE(!static_cast<bool>(cpls[0].cpl_annotation_text));
	BOOST_CHECK_EQUAL(
		cpls[0].cpl_file,
		dcp::filesystem::canonical(dir / "CPL_SMPTE_TST-B1P_S_EN-EN-CCAP_51-HI-VI_2K_ISDCF_20151123_DPPT_SMPTE-mod.xml")
	);
	BOOST_CHECK(!cpls[0].encrypted);
	BOOST_CHECK(cpls[0].last_write_time > 0);

	BOOST_CHECK_EQUAL(cpls[1].dcp_directory, dcp::filesystem::canonical(dir));
	BOOST_CHECK_EQUAL(cpls[1].cpl_id, "29e1a00b-0e19-4d5b-a1d6-24e97b331de6");
	BOOST_REQUIRE(!static_cast<bool>(cpls[1].cpl_annotation_text));
	BOOST_CHECK_EQUAL(
		cpls[1].cpl_file,
		dcp::filesystem::canonical(dir / "CPL_SMPTE_TST-B2P_S_EN-EN-CCAP_71-HI-VI_2K_ISDCF_20151123_DPPT_SMPTE-mod.xml")
	);
	BOOST_CHECK(!cpls[1].encrypted);
	BOOST_CHECK(cpls[1].last_write_time > 0);
}


