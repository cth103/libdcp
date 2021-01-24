/*
    Copyright (C) 2013-2021 Carl Hetherington <cth@carlh.net>

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
 *  @brief Key class
 */


#include "key.h"
#include "dcp_assert.h"
#include <asdcp/AS_DCP.h>
#include <asdcp/KM_prng.h>
#include <asdcp/KM_util.h>
#include <string>
#include <iomanip>


using std::string;
using std::setw;
using std::setfill;
using namespace dcp;


Key::Key (int length)
	: _value (new uint8_t[length])
	, _length (length)
{
	Kumu::FortunaRNG rng;
	rng.FillRandom (_value, _length);
}


Key::Key (uint8_t const * value, int length)
	: _value (new uint8_t[length])
	, _length (length)
{
	memcpy (_value, value, _length);
}


Key::Key (string value)
	: _value (new uint8_t[value.length() / 2])
	, _length (value.length() / 2)
{
	unsigned int length_done;
	Kumu::hex2bin (value.c_str(), _value, _length, &length_done);
}


Key::Key (Key const & other)
	: _value (new uint8_t[other._length])
	, _length (other._length)
{
	memcpy (_value, other._value, _length);
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

	_length = other._length;
	memcpy (_value, other._value, _length);
	return *this;
}


string
Key::hex () const
{
	char buffer[_length * 2 + 1];

	char* p = buffer;
	for (int i = 0; i < _length; ++i) {
#ifdef LIBDCP_WINDOWS
		__mingw_snprintf (p, 3, "%02hhx", _value[i]);
#else
		snprintf (p, 3, "%02hhx", _value[i]);
#endif
		p += 2;
	}

	return buffer;
}


bool
dcp::operator== (Key const & a, Key const & b)
{
	return a.length() == b.length() && memcmp(a.value(), b.value(), a.length()) == 0;
}


bool
dcp::operator!= (Key const & a, Key const & b)
{
	return !(a == b);
}
