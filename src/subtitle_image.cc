/*
    Copyright (C) 2018 Carl Hetherington <cth@carlh.net>

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

#include "subtitle_image.h"

using std::ostream;
using namespace dcp;

SubtitleImage::SubtitleImage (
	Data png_image,
	Time in,
	Time out,
	float h_position,
	HAlign h_align,
	float v_position,
	VAlign v_align,
	Time fade_up_time,
	Time fade_down_time
	)
	: Subtitle (in, out, h_position, h_align, v_position, v_align, fade_up_time, fade_down_time)
	, _png_image (png_image)
{

}

bool
dcp::operator== (SubtitleImage const & a, SubtitleImage const & b)
{
	return (
		a.png_image() == b.png_image(),
		a.in() == b.in() &&
		a.out() == b.out() &&
		a.h_position() == b.h_position() &&
		a.h_align() == b.h_align() &&
		a.v_position() == b.v_position() &&
		a.v_align() == b.v_align() &&
		a.fade_up_time() == b.fade_up_time() &&
		a.fade_down_time() == b.fade_down_time()
		);
}

bool
dcp::operator!= (SubtitleImage const & a, SubtitleImage const & b)
{
	return !(a == b);
}

ostream&
dcp::operator<< (ostream& s, SubtitleImage const & sub)
{
	s << "\n[IMAGE] from " << sub.in() << " to " << sub.out() << ";\n"
	  << "fade up " << sub.fade_up_time() << ", fade down " << sub.fade_down_time() << ";\n"
	  << "v pos " << sub.v_position() << ", valign " << ((int) sub.v_align())
	  << ", hpos " << sub.h_position() << ", halign " << ((int) sub.h_align()) << "\n";

	return s;
}
