#include <vector>
#include <cstdio>
#include <iomanip>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "types.h"
#include "exceptions.h"

using namespace std;
using namespace libdcp;
using namespace boost;

Fraction::Fraction (string s)
{
	vector<string> b;
	split (b, s, is_any_of (" "));
	if (b.size() != 2) {
		throw XMLError ("malformed fraction " + s + " in XML node");
	}
	numerator = lexical_cast<int> (b[0]);
	denominator = lexical_cast<int> (b[1]);
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

Color::Color (string argb_hex)
{
	int alpha;
	if (sscanf (argb_hex.c_str(), "%2x%2x%2x%2x", &alpha, &r, &g, &b) < 4) {
		throw XMLError ("could not parse colour string");
	}
}

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

bool
libdcp::operator== (Color const & a, Color const & b)
{
	return (a.r == b.r && a.g == b.g && a.b == b.b);
}

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

	throw MiscError ("unknown effect type");
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

	throw DCPReadError ("unknown subtitle effect type");
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

	throw MiscError ("unknown valign type");
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
	
	throw DCPReadError ("unknown subtitle valign type");
}

		
