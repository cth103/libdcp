#include <vector>
#include <cstdio>
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


bool
libdcp::operator== (Color const & a, Color const & b)
{
	return (a.r == b.r && a.g == b.g && a.b == b.b);
}

ostream &
libdcp::operator<< (ostream& s, Color const & c)
{
	s << "(" << c.r << ", " << c.g << ", " << c.b << ")";
	return s;
}
