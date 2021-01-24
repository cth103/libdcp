/*
    Copyright (C) 2018-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/subtitle_image.h
 *  @brief SubtitleImage class
 */


#ifndef LIBDCP_SUBTITLE_IMAGE_H
#define LIBDCP_SUBTITLE_IMAGE_H


#include "array_data.h"
#include "types.h"
#include "subtitle.h"
#include "dcp_time.h"
#include <boost/optional.hpp>
#include <string>


namespace dcp {


/** @class SubtitleImage
 *  @brief A bitmap subtitle with all the associated attributes
 */
class SubtitleImage : public Subtitle
{
public:
	SubtitleImage (
		ArrayData png_image,
		Time in,
		Time out,
		float h_position,
		HAlign h_align,
		float v_position,
		VAlign v_align,
		Time fade_up_time,
		Time fade_down_time
		);

	SubtitleImage (
		ArrayData png_image,
		std::string id,
		Time in,
		Time out,
		float h_position,
		HAlign h_align,
		float v_position,
		VAlign v_align,
		Time fade_up_time,
		Time fade_down_time
		);

	ArrayData png_image () const {
		return _png_image;
	}

	void set_png_image (ArrayData png) {
		_png_image = png;
	}

	void read_png_file (boost::filesystem::path file);
	void write_png_file (boost::filesystem::path file) const;

	std::string id () const {
		return _id;
	}

	/** @return the most recent disk file used to read or write this asset, if there is one */
	boost::optional<boost::filesystem::path> file () const {
		return _file;
	}

	bool equals (std::shared_ptr<dcp::SubtitleImage> other, EqualityOptions options, NoteHandler note);

private:
	ArrayData _png_image;
	std::string _id;
	mutable boost::optional<boost::filesystem::path> _file;
};


bool operator== (SubtitleImage const & a, SubtitleImage const & b);
bool operator!= (SubtitleImage const & a, SubtitleImage const & b);
std::ostream& operator<< (std::ostream& s, SubtitleImage const & sub);


}


#endif
