/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <vector>
#include <cstdio>
#include <iomanip>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "types.h"
#include "exceptions.h"
#include "raw_convert.h"

using namespace std;
using namespace libdcp;
using namespace boost;

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

bool
libdcp::operator== (Fraction const & a, Fraction const & b)
{
	return (a.numerator == b.numerator && a.denominator == b.denominator);
}

bool
libdcp::operator!= (Fraction const & a, Fraction const & b)
{
	return (a.numerator != b.numerator || a.denominator != b.denominator);
}

Color::Color ()
	: r (0)
	, g (0)
	, b (0)
{

}

Color::Color (int r_, int g_, int b_)
	: r (r_)
	, g (g_)
	, b (b_)
{

}

/** Construct a Color from an ARGB hex string; the alpha value is ignored.
 *  @param argb_hex A string of the form AARRGGBB, where e.g. RR is a two-character
 *  hex value.
 */
Color::Color (string argb_hex)
{
	int alpha;
	if (sscanf (argb_hex.c_str(), "%2x%2x%2x%2x", &alpha, &r, &g, &b) < 4) {
		boost::throw_exception (XMLError ("could not parse colour string"));
	}
}

/** @return An ARGB string of the form AARRGGBB, where e.g. RR is a two-character
 *  hex value.  The alpha value will always be FF (ie 255; maximum alpha).
 */
string
Color::to_argb_string () const
{
	stringstream s;
	s << "FF";
	s << hex
	  << setw(2) << setfill('0') << r
	  << setw(2) << setfill('0') << g
	  << setw(2) << setfill('0') << b;

	string t = s.str();
	to_upper (t);
	return t;
}

/** operator== for Colors.
 *  @param a First color to compare.
 *  @param b Second color to compare.
 */
bool
libdcp::operator== (Color const & a, Color const & b)
{
	return (a.r == b.r && a.g == b.g && a.b == b.b);
}

/** operator!= for Colors.
 *  @param a First color to compare.
 *  @param b Second color to compare.
 */
bool
libdcp::operator!= (Color const & a, Color const & b)
{
	return !(a == b);
}

ostream &
libdcp::operator<< (ostream& s, Color const & c)
{
	s << "(" << c.r << ", " << c.g << ", " << c.b << ")";
	return s;
}

string
libdcp::effect_to_string (Effect e)
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
libdcp::string_to_effect (string s)
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
libdcp::valign_to_string (VAlign v)
{
	switch (v) {
	case TOP:
		return "top";
	case CENTER:
		return "center";
	case BOTTOM:
		return "bottom";
	}

	boost::throw_exception (MiscError ("unknown valign type"));
}

VAlign
libdcp::string_to_valign (string s)
{
	if (s == "top") {
		return TOP;
	} else if (s == "center") {
		return CENTER;
	} else if (s == "bottom") {
		return BOTTOM;
	}
	
	boost::throw_exception (DCPReadError ("unknown subtitle valign type"));
}

		
