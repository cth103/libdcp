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


#include "exceptions.h"
#include "language_tag.h"
#include <boost/test/unit_test.hpp>


using std::vector;
using std::string;


BOOST_AUTO_TEST_CASE (language_tag_create_test)
{
	/* Bad subtags raise errors */

	{
		dcp::LanguageTag t;

		BOOST_CHECK_THROW (t.to_string(), dcp::LanguageTagError);

		BOOST_CHECK_THROW (t.set_language("sheila"), dcp::LanguageTagError);
		BOOST_CHECK_THROW (t.set_script("frobozz"), dcp::LanguageTagError);
		BOOST_CHECK_THROW (t.set_region("ostrabaglous"), dcp::LanguageTagError);
		BOOST_CHECK_THROW (dcp::LanguageTag::VariantSubtag("universe"), dcp::LanguageTagError);
		BOOST_CHECK_THROW (dcp::LanguageTag::ExtlangSubtag("universe"), dcp::LanguageTagError);
	}

	/* Duplicate subtags raise errors */

	{
		dcp::LanguageTag t;

		BOOST_CHECK_NO_THROW (t.add_variant("rozaj"));
		BOOST_CHECK_THROW (t.add_variant("rozaj"), dcp::LanguageTagError);

		BOOST_CHECK_NO_THROW (t.add_extlang("ltg"));
		BOOST_CHECK_THROW (t.add_extlang("ltg"), dcp::LanguageTagError);
	}

	/* Language */

	{
		dcp::LanguageTag t;
		BOOST_CHECK_NO_THROW (t.set_language("de"));
		BOOST_CHECK_EQUAL (t.to_string(), "de");
		BOOST_CHECK_EQUAL (t.description(), "German");
	}

	/* Case is ignored */

	{
		dcp::LanguageTag t;
		BOOST_CHECK_NO_THROW (t.set_language("dE"));
		BOOST_CHECK_EQUAL (t.to_string(), "dE");
	}

	/* Language + script */

	{
		dcp::LanguageTag t;
		BOOST_CHECK_NO_THROW (t.set_language("zh"));
		BOOST_CHECK_NO_THROW (t.set_script("Hant"));
		BOOST_CHECK_EQUAL (t.to_string(), "zh-Hant");
		BOOST_CHECK_EQUAL (t.description(), "Chinese written using the Han (Traditional variant) script");
	}

	/* Language + region */

	{
		dcp::LanguageTag t;
		BOOST_CHECK_NO_THROW (t.set_language("de"));
		BOOST_CHECK_NO_THROW (t.set_region("DE"));
		BOOST_CHECK_EQUAL (t.to_string(), "de-DE");
		BOOST_CHECK_EQUAL (t.description(), "German for Germany");
	}

	/* Language + variant */

	{
		dcp::LanguageTag t;
		BOOST_CHECK_NO_THROW (t.set_language("sl"));
		BOOST_CHECK_NO_THROW (t.add_variant("rozaj"));
		BOOST_CHECK_EQUAL (t.to_string(), "sl-rozaj");
		BOOST_CHECK_EQUAL (t.description(), "Rezijan dialect of Slovenian");
	}

	/* Language + 2 variants */

	{
		dcp::LanguageTag t;
		BOOST_CHECK_NO_THROW (t.set_language("sl"));
		BOOST_CHECK_NO_THROW (t.add_variant("biske"));
		BOOST_CHECK_NO_THROW (t.add_variant("rozaj"));
		BOOST_CHECK_EQUAL (t.to_string(), "sl-biske-rozaj");
		BOOST_CHECK_EQUAL (t.description(), "The Bila dialect of Resian dialect of Rezijan dialect of Slovenian");
	}

	/* Language + extlang */

	{
		dcp::LanguageTag t;
		BOOST_CHECK_NO_THROW (t.set_language("sl"));
		BOOST_CHECK_NO_THROW (t.add_extlang("afb"));
		BOOST_CHECK_EQUAL (t.to_string(), "sl-afb");
		BOOST_CHECK_EQUAL (t.description(), "Slovenian, Gulf Arabic");
	}

	/* Language + 2 extlangs */

	{
		dcp::LanguageTag t;
		BOOST_CHECK_NO_THROW (t.set_language("sl"));
		BOOST_CHECK_NO_THROW (t.add_extlang("afb"));
		BOOST_CHECK_NO_THROW (t.add_extlang("ltg"));
		BOOST_CHECK_EQUAL (t.to_string(), "sl-afb-ltg");
		BOOST_CHECK_EQUAL (t.description(), "Slovenian, Gulf Arabic, Latgalian");
	}

	/* Language + script + region */

	{
		dcp::LanguageTag t;
		BOOST_CHECK_NO_THROW (t.set_language("zh"));
		BOOST_CHECK_NO_THROW (t.set_script("Hant"));
		BOOST_CHECK_NO_THROW (t.set_region("DE"));
		BOOST_CHECK_EQUAL (t.to_string(), "zh-Hant-DE");
		BOOST_CHECK_EQUAL (t.description(), "Chinese written using the Han (Traditional variant) script for Germany");
	}

	/* Language + script + region + variant */

	{
		dcp::LanguageTag t;
		BOOST_CHECK_NO_THROW (t.set_language("hy"));
		BOOST_CHECK_NO_THROW (t.set_script("Latn"));
		BOOST_CHECK_NO_THROW (t.set_region("IT"));
		BOOST_CHECK_NO_THROW (t.add_variant("arevela"));
		BOOST_CHECK_EQUAL (t.to_string(), "hy-Latn-IT-arevela");
		BOOST_CHECK_EQUAL (t.description(), "Eastern Armenian dialect of Armenian written using the Latin script for Italy");
	}

	/* Langauge + script + region + variant + extlang */

	{
		dcp::LanguageTag t;
		BOOST_CHECK_NO_THROW (t.set_language("hy"));
		BOOST_CHECK_NO_THROW (t.set_script("Latn"));
		BOOST_CHECK_NO_THROW (t.set_region("IT"));
		BOOST_CHECK_NO_THROW (t.add_variant("arevela"));
		BOOST_CHECK_NO_THROW (t.add_extlang("ltg"));
		BOOST_CHECK_EQUAL (t.to_string(), "hy-Latn-IT-arevela-ltg");
		BOOST_CHECK_EQUAL (t.description(), "Eastern Armenian dialect of Armenian written using the Latin script for Italy, Latgalian");
	}

}


BOOST_AUTO_TEST_CASE (language_tag_parse_test)
{
	BOOST_CHECK_THROW (dcp::LanguageTag(""), dcp::LanguageTagError);
	BOOST_CHECK_THROW (dcp::LanguageTag("...Aw498012351!"), dcp::LanguageTagError);
	BOOST_CHECK_THROW (dcp::LanguageTag("fish"), dcp::LanguageTagError);
	BOOST_CHECK_THROW (dcp::LanguageTag("de-Dogr-fish"), dcp::LanguageTagError);
	BOOST_CHECK_THROW (dcp::LanguageTag("de-Dogr-DE-aranes-fish"), dcp::LanguageTagError);

	BOOST_CHECK_EQUAL (dcp::LanguageTag("de").to_string(), "de");
	BOOST_CHECK_EQUAL (dcp::LanguageTag("de-Dogr").to_string(), "de-Dogr");
	BOOST_CHECK_EQUAL (dcp::LanguageTag("de-Dogr-DE").to_string(), "de-Dogr-DE");
	BOOST_CHECK_EQUAL (dcp::LanguageTag("de-Dogr-DE-aranes").to_string(), "de-Dogr-DE-aranes");
	BOOST_CHECK_EQUAL (dcp::LanguageTag("de-Dogr-DE-aranes-lemosin").to_string(), "de-Dogr-DE-aranes-lemosin");
	BOOST_CHECK_EQUAL (dcp::LanguageTag("de-Dogr-DE-aranes-lemosin-abv").to_string(), "de-Dogr-DE-aranes-lemosin-abv");
	BOOST_CHECK_EQUAL (dcp::LanguageTag("de-Dogr-DE-aranes-lemosin-abv-zsm").to_string(), "de-Dogr-DE-aranes-lemosin-abv-zsm");
}

