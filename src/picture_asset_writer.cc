/*
    Copyright (C) 2012-2013 Carl Hetherington <cth@carlh.net>

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

#include <inttypes.h>
#include <stdint.h>
#include "AS_DCP.h"
#include "KM_fileio.h"
#include "picture_asset_writer.h"
#include "exceptions.h"
#include "picture_asset.h"

using std::istream;
using std::ostream;
using std::string;
using boost::shared_ptr;
using namespace libdcp;

FrameInfo::FrameInfo (istream& s)
	: offset (0)
	, size (0)
{
	s >> offset >> size;

	if (!s.good ()) {
		/* Make sure we zero these if something bad happened, otherwise
		   the caller might try to alloc lots of RAM.
		*/
		offset = size = 0;
	}

	s >> hash;
}

FrameInfo::FrameInfo (FILE* f)
{
#ifdef LIBDCP_WINDOWS
	fscanf (f, "%I64u", &offset);
	fscanf (f, "%I64u", &size);
#else	
	fscanf (f, "%" SCNu64, &offset);
	fscanf (f, "%" SCNu64, &size);
#endif	

	if (ferror (f)) {
		offset = size = 0;
	}

	char hash_buffer[128];
	fscanf (f, "%s", hash_buffer);
	hash = hash_buffer;
}

void
FrameInfo::write (ostream& s) const
{
	s << offset << " " << size << " " << hash;
}

void
FrameInfo::write (FILE* f) const
{
#ifdef LIBDCP_WINDOWS	
	fprintf (f, "%I64u %I64u %s", offset, size, hash.c_str ());
#else	
	fprintf (f, "%" PRIu64 " %" PRIu64 " %s", offset, size, hash.c_str ());
#endif	
}


PictureAssetWriter::PictureAssetWriter (PictureAsset* asset, bool overwrite)
	: _asset (asset)
	, _frames_written (0)
	, _started (false)
	, _finalized (false)
	, _overwrite (overwrite)
{

}
