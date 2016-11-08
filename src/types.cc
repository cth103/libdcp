/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#include "raw_convert.h"
#include "types.h"
#include "exceptions.h"
#include "compose.hpp"
#include <boost/algorithm/string.hpp>
#include <vector>
#include <cstdio>
#include <iomanip>

using namespace std;
using namespace dcp;
using namespace boost;

/** Construct a Fraction from a string of the form <numerator> <denominator>
 *  e.g. "1 3".
 */
Fraction::Fraction (string s)
{
	vector<string> b;
	split (b, s, is_any_of (" "));
	if (b.size() != 2) {
		boost::throw_exception (XMLError ("malformed fraction " + s + " in XML node"));
	}
	numerator = raw_convert<int> (b[0]);
	denominator = raw_convert<int> (b[1]);
}

string
Fraction::as_string () const
{
	return String::compose ("%1 %2", numerator, denominator);
}

bool
dcp::operator== (Fraction const & a, Fraction const & b)
{
	return (a.numerator == b.numerator && a.denominator == b.denominator);
}

bool
dcp::operator!= (Fraction const & a, Fraction const & b)
{
	return (a.numerator != b.numerator || a.denominator != b.denominator);
}

ostream&
dcp::operator<< (ostream& s, Fraction const & f)
{
	s << f.numerator << "/" << f.denominator;
	return s;
}

/** Construct a Colour, initialising it to black. */
Colour::Colour ()
	: r (0)
	, g (0)
	, b (0)
{

}

/** Construct a Colour from R, G and B.  The values run between
 *  0 and 255.
 */
Colour::Colour (int r_, int g_, int b_)
	: r (r_)
	, g (g_)
	, b (b_)
{

}

/** Construct a Colour from an ARGB hex string; the alpha value is ignored.
 *  @param argb_hex A string of the form AARRGGBB, where e.g. RR is a two-character
 *  hex value.
 */
Colour::Colour (string argb_hex)
{
	int alpha;
	if (sscanf (argb_hex.c_str(), "%2x%2x%2x%2x", &alpha, &r, &g, &b) != 4) {
		boost::throw_exception (XMLError ("could not parse colour string"));
	}
}

/** @return An ARGB string of the form AARRGGBB, where e.g. RR is a two-character
 *  hex value.  The alpha value will always be FF (ie 255; maximum alpha).
 */
string
Colour::to_argb_string () const
{
	char buffer[9];
	snprintf (buffer, sizeof(buffer), "FF%02X%02X%02X", r, g, b);
	return buffer;
}

/** @return An RGB string of the form RRGGBB, where e.g. RR is a two-character
 *  hex value.
 */
string
Colour::to_rgb_string () const
{
	char buffer[7];
	snprintf (buffer, sizeof(buffer), "%02X%02X%02X", r, g, b);
	return buffer;
}

/** operator== for Colours.
 *  @param a First colour to compare.
 *  @param b Second colour to compare.
 */
bool
dcp::operator== (Colour const & a, Colour const & b)
{
	return (a.r == b.r && a.g == b.g && a.b == b.b);
}

/** operator!= for Colours.
 *  @param a First colour to compare.
 *  @param b Second colour to compare.
 */
bool
dcp::operator!= (Colour const & a, Colour const & b)
{
	return !(a == b);
}

ostream &
dcp::operator<< (ostream& s, Colour const & c)
{
	s << "(" << c.r << ", " << c.g << ", " << c.b << ")";
	return s;
}

string
dcp::effect_to_string (Effect e)
{
	switch (e) {
	case NONE:
		return "none";
	case BORDER:
		return "border";
	case SHADOW:
		return "shadow";
	}

	boost::throw_exception (MiscError ("unknown effect type"));
}

Effect
dcp::string_to_effect (string s)
{
	if (s == "none") {
		return NONE;
	} else if (s == "border") {
		return BORDER;
	} else if (s == "shadow") {
		return SHADOW;
	}

	boost::throw_exception (DCPReadError ("unknown subtitle effect type"));
}

string
dcp::halign_to_string (HAlign h)
{
	switch (h) {
	case HALIGN_LEFT:
		return "left";
	case HALIGN_CENTER:
		return "center";
	case HALIGN_RIGHT:
		return "right";
	}

	boost::throw_exception (MiscError ("unknown subtitle halign type"));
}

HAlign
dcp::string_to_halign (string s)
{
	if (s == "left") {
		return HALIGN_LEFT;
	} else if (s == "center") {
		return HALIGN_CENTER;
	} else if (s == "right") {
		return HALIGN_RIGHT;
	}

	boost::throw_exception (DCPReadError ("unknown subtitle halign type"));
}

string
dcp::valign_to_string (VAlign v)
{
	switch (v) {
	case VALIGN_TOP:
		return "top";
	case VALIGN_CENTER:
		return "center";
	case VALIGN_BOTTOM:
		return "bottom";
	}

	boost::throw_exception (MiscError ("unknown subtitle valign type"));
}

VAlign
dcp::string_to_valign (string s)
{
	if (s == "top") {
		return VALIGN_TOP;
	} else if (s == "center") {
		return VALIGN_CENTER;
	} else if (s == "bottom") {
		return VALIGN_BOTTOM;
	}

	boost::throw_exception (DCPReadError ("unknown subtitle valign type"));
}

string
dcp::direction_to_string (Direction v)
{
	switch (v) {
	case DIRECTION_LTR:
		return "ltr";
	case DIRECTION_RTL:
		return "rtl";
	case DIRECTION_TTB:
		return "ttb";
	case DIRECTION_BTT:
		return "btt";
	}

	boost::throw_exception (MiscError ("unknown subtitle direction type"));
}

Direction
dcp::string_to_direction (string s)
{
	if (s == "ltr" || s == "horizontal") {
		return DIRECTION_LTR;
	} else if (s == "rtl") {
		return DIRECTION_RTL;
	} else if (s == "ttb" || s == "vertical") {
		return DIRECTION_TTB;
	} else if (s == "btt") {
		return DIRECTION_BTT;
	}

	boost::throw_exception (DCPReadError ("unknown subtitle direction type"));
}
