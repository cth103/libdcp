/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

#include "data.h"
#include <boost/shared_ptr.hpp>
#include <stdint.h>

namespace dcp {

class OpenJPEGImage;

extern boost::shared_ptr<OpenJPEGImage> decompress_j2k (uint8_t* data, int64_t size, int reduce);
extern boost::shared_ptr<OpenJPEGImage> decompress_j2k (Data data, int reduce);
extern Data compress_j2k (boost::shared_ptr<const OpenJPEGImage>, int bandwith, int frames_per_second, bool threed, bool fourk);

}
