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

#include <stdint.h>
#include <string>
#include <fstream>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include "metadata.h"
#include "types.h"

namespace libdcp {

class PictureAsset;	

/** Information about a single frame (either a monoscopic frame or a left *or* right eye stereoscopic frame) */	
struct FrameInfo
{
	FrameInfo (uint64_t o, uint64_t s, std::string h)
		: offset (o)
		, size (s)
		, hash (h)
	{}

	FrameInfo (std::istream& s);
	FrameInfo (FILE *);

	void write (std::ostream& s) const;
	void write (FILE *) const;
	
	uint64_t offset;
	uint64_t size;
	std::string hash;
};

class PictureAssetWriter : public boost::noncopyable
{
public:
	virtual ~PictureAssetWriter () {}
	virtual FrameInfo write (uint8_t *, int) = 0;
	virtual void finalize () = 0;
	virtual void fake_write (int) = 0;
	
protected:
	template <class P, class Q>
	friend void start (PictureAssetWriter *, boost::shared_ptr<P>, Q *, uint8_t *, int);

	PictureAssetWriter (PictureAsset *, bool);

	PictureAsset* _asset;
	
	/** Number of picture frames written to the asset so far.  For stereo assets
	 *  this will be incremented for each eye (i.e. there will be twice the number
	 *  of frames as in a mono asset).
	 */
	int _frames_written;
	bool _started;
	/** true if finalize() has been called */
	bool _finalized;
	bool _overwrite;
};

}
