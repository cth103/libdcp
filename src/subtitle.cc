/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/subtitle.cc
 *  @brief Subtitle class
 */


#include "compose.hpp"
#include "dcp_time.h"
#include "equality_options.h"
#include "subtitle.h"


using std::shared_ptr;
using namespace dcp;


/** @param v_position Vertical position as a fraction of the screen height (between 0 and 1) from v_align */
Subtitle::Subtitle (
	Time in,
	Time out,
	float h_position,
	HAlign h_align,
	float v_position,
	VAlign v_align,
	float z_position,
	Time fade_up_time,
	Time fade_down_time
	)
	: _in (in)
	, _out (out)
	, _h_position (h_position)
	, _h_align (h_align)
	, _v_position (v_position)
	, _v_align (v_align)
	, _z_position(z_position)
	, _fade_up_time (fade_up_time)
	, _fade_down_time (fade_down_time)
{

}


bool
Subtitle::equals(shared_ptr<const Subtitle> other, EqualityOptions const& options, NoteHandler note) const
{
	bool same = true;

	if (in() != other->in()) {
		note(NoteType::ERROR, "subtitle in times differ");
		same = false;
	}

	if (out() != other->out()) {
		note(NoteType::ERROR, "subtitle out times differ");
		same = false;
	}

	if (h_position() != other->h_position()) {
		note(NoteType::ERROR, "subtitle horizontal positions differ");
		same = false;
	}

	if (h_align() != other->h_align()) {
		note(NoteType::ERROR, "subtitle horizontal alignments differ");
		same = false;
	}

	auto const vpos = std::abs(v_position() - other->v_position());
	if (vpos > options.max_subtitle_vertical_position_error)  {
		note(
			NoteType::ERROR,
			String::compose("subtitle vertical positions differ by %1 (more than the allowed difference of %2)", vpos, options.max_subtitle_vertical_position_error)
		    );
		same = false;
	}

	if (v_align() != other->v_align()) {
		note(NoteType::ERROR, "subtitle vertical alignments differ");
		same = false;
	}

	if (z_position() != other->z_position()) {
		note(NoteType::ERROR, "subtitle Z positions differ");
		same = false;
	}

	if (fade_up_time() != other->fade_up_time()) {
		note(NoteType::ERROR, "subtitle fade-up times differ");
		same = false;
	}

	if (fade_down_time() != other->fade_down_time()) {
		note(NoteType::ERROR, "subtitle fade-down times differ");
		same = false;
	}

	return same;
}
