/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

#include <sstream>
#include <string>
#include <iomanip>
#include "AS_DCP.h"
#include "KM_prng.h"
#include "KM_util.h"
#include "key.h"

using std::string;
using std::stringstream;
using std::setw;
using std::setfill;
using namespace libdcp;

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
	stringstream g;
	
	for (unsigned int i = 0; i < ASDCP::KeyLen; ++i) {
		g << setw(2) << setfill('0') << std::hex << static_cast<int> (_value[i]);
	}

	return g.str ();
}

bool
libdcp::operator== (Key const & a, Key const & b)
{
	return memcmp (a.value(), b.value(), ASDCP::KeyLen) == 0;
}

bool
libdcp::operator!= (Key const & a, Key const & b)
{
	return !(a == b);
}

