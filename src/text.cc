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


/** @file  src/text.cc
 *  @brief Text class
 */


#include "compose.hpp"
#include "dcp_time.h"
#include "equality_options.h"
#include "text.h"


using std::shared_ptr;
using std::string;
using std::vector;
using boost::optional;
using namespace dcp;


/** @param v_position Vertical position as a fraction of the screen height (between 0 and 1) from v_align */
Text::Text(
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
	: _in (in)
	, _out (out)
	, _h_position (h_position)
	, _h_align (h_align)
	, _v_position (v_position)
	, _v_align (v_align)
	, _z_position(z_position)
	, _variable_z_positions(variable_z_positions)
	, _fade_up_time (fade_up_time)
	, _fade_down_time (fade_down_time)
{

}


bool
Text::equals(shared_ptr<const Text> other, EqualityOptions const& options, NoteHandler note) const
{
	bool same = true;

	if (in() != other->in()) {
		note(NoteType::ERROR, "text in times differ");
		same = false;
	}

	if (out() != other->out()) {
		note(NoteType::ERROR, "text out times differ");
		same = false;
	}

	if (h_position() != other->h_position()) {
		note(NoteType::ERROR, "text horizontal positions differ");
		same = false;
	}

	if (h_align() != other->h_align()) {
		note(NoteType::ERROR, "text horizontal alignments differ");
		same = false;
	}

	auto const vpos = std::abs(v_position() - other->v_position());
	if (vpos > options.max_text_vertical_position_error)  {
		note(
			NoteType::ERROR,
			String::compose("text vertical positions differ by %1 (more than the allowed difference of %2)", vpos, options.max_text_vertical_position_error)
		    );
		same = false;
	}

	if (v_align() != other->v_align()) {
		note(NoteType::ERROR, "text vertical alignments differ");
		same = false;
	}

	if (z_position() != other->z_position()) {
		note(NoteType::ERROR, "text Z positions differ");
		same = false;
	}

	if (variable_z_positions() != other->variable_z_positions()) {
		note(NoteType::ERROR, "text variable Z positions differ");
		same = false;
	}

	if (fade_up_time() != other->fade_up_time()) {
		note(NoteType::ERROR, "text fade-up times differ");
		same = false;
	}

	if (fade_down_time() != other->fade_down_time()) {
		note(NoteType::ERROR, "text fade-down times differ");
		same = false;
	}

	return same;
}


bool dcp::operator==(Text::VariableZPosition a, Text::VariableZPosition b)
{
	return a.position == b.position && a.duration == b.duration;
}

