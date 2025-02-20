/*
    Copyright (C) 2022 Carl Hetherington <cth@carlh.net>

    This file is part of DCP-o-matic.

    DCP-o-matic is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DCP-o-matic is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DCP-o-matic.  If not, see <http://www.gnu.org/licenses/>.

*/


#include "verify.h"
#include "verify_report.h"
#include <boost/test/unit_test.hpp>
#include "test.h"


BOOST_AUTO_TEST_CASE(verify_report_basically_ok)
{
	dcp::HTMLFormatter formatter("build/test/verify_report_basically_ok.html");
	dcp::verify_report(
		{
			dcp::verify(
				{ private_test / "TONEPLATES-SMPTE-PLAINTEXT_TST_F_XX-XX_ITL-TD_51-XX_2K_WOE_20111001_WOE_OV" },
				{},
				[](std::string, boost::optional<boost::filesystem::path>) {},
				[](float) {},
				{},
				xsd_test
				)
		},
		formatter
		);
}


BOOST_AUTO_TEST_CASE(text_formatter)
{
	{
		dcp::TextFormatter fmt("build/test/text_formatter.txt");

		fmt.heading("Heading");
		fmt.subheading("Subheading");
		auto A = fmt.unordered_list();
		fmt.list_item("Foo");
		fmt.list_item("Bar");
		auto B = fmt.unordered_list();
		fmt.list_item("Fred");
		fmt.list_item("Jim");
		fmt.list_item("Sheila");
	}

#ifdef LIBDCP_WINDOWS
	check_file("test/data/text_formatter_windows.txt", "build/test/text_formatter.txt");
#else
	check_file("test/data/text_formatter.txt", "build/test/text_formatter.txt");
#endif
}

