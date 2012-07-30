#include <vector>
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
