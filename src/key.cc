/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

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

/** @file  src/key.cc
 *  @brief Key class.
 */

#include "key.h"
#include <locked_sstream.h>
#include <asdcp/AS_DCP.h>
#include <asdcp/KM_prng.h>
#include <asdcp/KM_util.h>
#include <string>
#include <iomanip>

using std::string;
using std::setw;
using std::setfill;
using namespace dcp;

Key::Key ()
	: _value (new uint8_t[ASDCP::KeyLen])
{
	Kumu::FortunaRNG rng;
	rng.FillRandom (_value, ASDCP::KeyLen);
}

Key::Key (uint8_t const * value)
	: _value (new uint8_t[ASDCP::KeyLen])
{
	memcpy (_value, value, ASDCP::KeyLen);
}

Key::Key (string value)
	: _value (new uint8_t[ASDCP::KeyLen])
{
	unsigned int length;
	Kumu::hex2bin (value.c_str(), _value, ASDCP::KeyLen, &length);
}

Key::Key (Key const & other)
	: _value (new uint8_t[ASDCP::KeyLen])
{
	memcpy (_value, other._value, ASDCP::KeyLen);
}

Key::~Key ()
{
	delete[] _value;
}

Key &
Key::operator= (Key const & other)
{
	if (this == &other) {
		return *this;
	}

	memcpy (_value, other._value, ASDCP::KeyLen);
	return *this;
}

string
Key::hex () const
{
	locked_stringstream g;

	for (unsigned int i = 0; i < ASDCP::KeyLen; ++i) {
		g << setw(2) << setfill('0') << std::hex << static_cast<int> (_value[i]);
	}

	return g.str ();
}

bool
dcp::operator== (Key const & a, Key const & b)
{
	return memcmp (a.value(), b.value(), ASDCP::KeyLen) == 0;
}

bool
dcp::operator!= (Key const & a, Key const & b)
{
	return !(a == b);
}
