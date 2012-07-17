/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

/** @file  src/util.cc
 *  @brief Utility methods.
 */

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <boost/filesystem.hpp>
#include <openssl/sha.h>
#include "KM_util.h"
#include "KM_fileio.h"
#include "AS_DCP.h"
#include "util.h"
#include "exceptions.h"

using namespace std;
using namespace boost;

string
libdcp::make_uuid ()
{
	char buffer[64];
	Kumu::UUID id;
	Kumu::GenRandomValue (id);
	id.EncodeHex (buffer, 64);
	return string (buffer);
}

string
libdcp::make_digest (string filename, sigc::signal1<void, float>* progress)
{
	int const file_size = filesystem::file_size (filename);
	
	Kumu::FileReader reader;
	if (ASDCP_FAILURE (reader.OpenRead (filename.c_str ()))) {
		throw FileError ("could not open file to compute digest", filename);
	}
	
	SHA_CTX sha;
	SHA1_Init (&sha);
	
	Kumu::ByteString read_buffer (65536);
	int done = 0;
	while (1) {
		ui32_t read = 0;
		Kumu::Result_t r = reader.Read (read_buffer.Data(), read_buffer.Capacity(), &read);
		
		if (r == Kumu::RESULT_ENDOFFILE) {
			break;
		} else if (ASDCP_FAILURE (r)) {
			throw FileError ("could not read file to compute digest", filename);
		}
		
		SHA1_Update (&sha, read_buffer.Data(), read);
		done += read;

		if (progress) {
			(*progress) (0.5 + (0.5 * done / file_size));
		}
	}

	byte_t byte_buffer[20];
	SHA1_Final (byte_buffer, &sha);

	stringstream s;
	char digest[64];
	return Kumu::base64encode (byte_buffer, 20, digest, 64);
}
