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


/** @file  src/text_image.cc
 *  @brief TextImage class
 */


#include "equality_options.h"
#include "filesystem.h"
#include "text_image.h"
#include "util.h"


using std::dynamic_pointer_cast;
using std::ostream;
using std::shared_ptr;
using std::string;
using std::vector;
using boost::optional;
using namespace dcp;


TextImage::TextImage(
	ArrayData png_image,
	Time in,
	Time out,
	float h_position,
	HAlign h_align,
	float v_position,
	VAlign v_align,
	float z_position,
	vector<VariableZPosition> variable_z_positions,
	Time fade_up_time,
	Time fade_down_time
	)
	: Text(in, out, h_position, h_align, v_position, v_align, z_position, variable_z_positions, fade_up_time, fade_down_time)
	, _png_image (png_image)
	, _id (make_uuid ())
{

}


TextImage::TextImage(
	ArrayData png_image,
	string id,
	Time in,
	Time out,
	float h_position,
	HAlign h_align,
	float v_position,
	VAlign v_align,
	float z_position,
	vector<VariableZPosition> variable_z_positions,
	Time fade_up_time,
	Time fade_down_time
	)
	: Text(in, out, h_position, h_align, v_position, v_align, z_position, variable_z_positions, fade_up_time, fade_down_time)
	, _png_image (png_image)
	, _id (id)
{

}


void
TextImage::read_png_file(boost::filesystem::path file)
{
	_file = file;
	_png_image = ArrayData (file);
}


void
TextImage::write_png_file(boost::filesystem::path file) const
{
	_file = file;
	png_image().write (file);
}


bool
dcp::operator==(TextImage const & a, TextImage const & b)
{
	return (
		a.png_image() == b.png_image() &&
		a.id() == b.id() &&
		a.in() == b.in() &&
		a.out() == b.out() &&
		a.h_position() == b.h_position() &&
		a.h_align() == b.h_align() &&
		a.v_position() == b.v_position() &&
		a.v_align() == b.v_align() &&
		a.z_position() == b.z_position() &&
		a.fade_up_time() == b.fade_up_time() &&
		a.fade_down_time() == b.fade_down_time()
		);
}


bool
dcp::operator!=(TextImage const & a, TextImage const & b)
{
	return !(a == b);
}


bool
TextImage::equals(shared_ptr<const Text> other_sub, EqualityOptions const& options, NoteHandler note) const
{
	if (!Text::equals(other_sub, options, note)) {
		return false;
	}

	auto other = dynamic_pointer_cast<const TextImage>(other_sub);
	if (!other) {
		note(NoteType::ERROR, "Text types differ: string vs image");
		return false;
	}

	if (png_image() != other->png_image()) {
		note (NoteType::ERROR, "text image PNG data differs");
		if (options.export_differing_texts) {
			string const base = "dcpdiff_text_";
			if (filesystem::exists(base + "A.png")) {
				note (NoteType::ERROR, "could not export text as " + base + "A.png already exists");
			} else {
				png_image().write(base + "A.png");
			}
			if (filesystem::exists(base + "B.png")) {
				note (NoteType::ERROR, "could not export text as " + base + "B.png already exists");
			} else {
				other->png_image().write(base + "B.png");
			}
		}
		return false;
	}

	return true;
}


ostream&
dcp::operator<<(ostream& s, TextImage const& text)
{
	s << "\n[IMAGE] from " << text.in() << " to " << text.out() << ";\n"
	  << "fade up " << text.fade_up_time() << ", fade down " << text.fade_down_time() << ";\n"
	  << "v pos " << text.v_position() << ", valign " << ((int) text.v_align())
	  << ", hpos " << text.h_position() << ", halign " << ((int) text.h_align())
	  << ", zpos " << text.z_position() << "\n";

	return s;
}

