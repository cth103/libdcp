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
	DCP_ASSERT (ASDCP::KeyLen == 16);

	char buffer[33];
#ifdef LIBDCP_WINDOWS
	gnu_snprintf (
#else
	snprintf (
#endif
		buffer, sizeof(buffer),
		"%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
		_value[0], _value[1], _value[2], _value[3], _value[4], _value[5], _value[6], _value[7],
		_value[8], _value[9], _value[10], _value[11], _value[12], _value[13], _value[14], _value[15]
		);

	return buffer;
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
