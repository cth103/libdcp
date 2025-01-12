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


/** @file  src/types.cc
 *  @brief Miscellaneous types
 */


#include "compose.hpp"
#include "dcp_assert.h"
#include "exceptions.h"
#include "raw_convert.h"
#include "types.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <fmt/format.h>
#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <iomanip>


using std::string;
using std::ostream;
using std::vector;
using namespace dcp;
using namespace boost;


bool dcp::operator== (dcp::Size const & a, dcp::Size const & b)
{
	return (a.width == b.width && a.height == b.height);
}


bool dcp::operator!= (dcp::Size const & a, dcp::Size const & b)
{
	return !(a == b);
}


/** Construct a Fraction from a string of the form "numerator denominator"
 *  e.g. "1 3".
 */
Fraction::Fraction (string s)
{
	vector<string> b;
	split (b, s, is_any_of (" "));
	if (b.size() != 2) {
		boost::throw_exception (XMLError("malformed fraction " + s + " in XML node"));
	}
	numerator = raw_convert<int> (b[0]);
	denominator = raw_convert<int> (b[1]);
}


string
Fraction::as_string () const
{
	return String::compose ("%1 %2", numerator, denominator);
}


bool
dcp::operator== (Fraction const & a, Fraction const & b)
{
	return (a.numerator == b.numerator && a.denominator == b.denominator);
}


bool
dcp::operator!= (Fraction const & a, Fraction const & b)
{
	return (a.numerator != b.numerator || a.denominator != b.denominator);
}


Colour::Colour (int r_, int g_, int b_)
	: r (r_)
	, g (g_)
	, b (b_)
{

}


Colour::Colour (string argb_hex)
{
	int alpha;
	if (sscanf (argb_hex.c_str(), "%2x%2x%2x%2x", &alpha, &r, &g, &b) != 4) {
		boost::throw_exception (XMLError ("could not parse colour string"));
	}
}


string
Colour::to_argb_string () const
{
	char buffer[9];
	snprintf (buffer, sizeof(buffer), "FF%02X%02X%02X", r, g, b);
	return buffer;
}


string
Colour::to_rgb_string () const
{
	char buffer[7];
	snprintf (buffer, sizeof(buffer), "%02X%02X%02X", r, g, b);
	return buffer;
}


bool
dcp::operator== (Colour const & a, Colour const & b)
{
	return (a.r == b.r && a.g == b.g && a.b == b.b);
}


bool
dcp::operator!= (Colour const & a, Colour const & b)
{
	return !(a == b);
}


string
dcp::effect_to_string (Effect e)
{
	switch (e) {
	case Effect::NONE:
		return "none";
	case Effect::BORDER:
		return "border";
	case Effect::SHADOW:
		return "shadow";
	}

	boost::throw_exception (MiscError("unknown effect type"));
}


Effect
dcp::string_to_effect (string s)
{
	if (s == "none") {
		return Effect::NONE;
	} else if (s == "border") {
		return Effect::BORDER;
	} else if (s == "shadow") {
		return Effect::SHADOW;
	}

	boost::throw_exception (ReadError("unknown subtitle effect type"));
}


string
dcp::direction_to_string (Direction v)
{
	switch (v) {
	case Direction::LTR:
		return "ltr";
	case Direction::RTL:
		return "rtl";
	case Direction::TTB:
		return "ttb";
	case Direction::BTT:
		return "btt";
	}

	boost::throw_exception (MiscError("unknown subtitle direction type"));
}


Direction
dcp::string_to_direction (string s)
{
	if (s == "ltr" || s == "horizontal") {
		return Direction::LTR;
	} else if (s == "rtl") {
		return Direction::RTL;
	} else if (s == "ttb" || s == "vertical") {
		return Direction::TTB;
	} else if (s == "btt") {
		return Direction::BTT;
	}

	boost::throw_exception (ReadError("unknown subtitle direction type"));
}


string
dcp::marker_to_string (dcp::Marker m)
{
	switch (m) {
	case Marker::FFOC:
		return "FFOC";
	case Marker::LFOC:
		return "LFOC";
	case Marker::FFTC:
		return "FFTC";
	case Marker::LFTC:
		return "LFTC";
	case Marker::FFOI:
		return "FFOI";
	case Marker::LFOI:
		return "LFOI";
	case Marker::FFEC:
		return "FFEC";
	case Marker::LFEC:
		return "LFEC";
	case Marker::FFMC:
		return "FFMC";
	case Marker::LFMC:
		return "LFMC";
	case Marker::FFOB:
		return "FFOB";
	case Marker::LFOB:
		return "LFOB";
	}

	DCP_ASSERT (false);
}


dcp::Marker
dcp::marker_from_string (string s)
{
	if (s == "FFOC") {
		return Marker::FFOC;
	} else if (s == "LFOC") {
		return Marker::LFOC;
	} else if (s == "FFTC") {
		return Marker::FFTC;
	} else if (s == "LFTC") {
		return Marker::LFTC;
	} else if (s == "FFOI") {
		return Marker::FFOI;
	} else if (s == "LFOI") {
		return Marker::LFOI;
	} else if (s == "FFEC") {
		return Marker::FFEC;
	} else if (s == "LFEC") {
		return Marker::LFEC;
	} else if (s == "FFMC") {
		return Marker::FFMC;
	} else if (s == "LFMC") {
		return Marker::LFMC;
	} else if (s == "FFOB") {
		return Marker::FFOB;
	} else if (s == "LFOB") {
		return Marker::LFOB;
	}

	DCP_ASSERT (false);
}


ContentVersion::ContentVersion ()
	: id ("urn:uuid:" + make_uuid())
{

}


ContentVersion::ContentVersion (cxml::ConstNodePtr node)
	: id(node->string_child("Id"))
	, label_text(node->string_child("LabelText"))
{

}


ContentVersion::ContentVersion (string label_text_)
	: id ("urn:uuid:" + make_uuid())
	, label_text (label_text_)
{

}


void
ContentVersion::as_xml (xmlpp::Element* parent) const
{
	auto cv = cxml::add_child(parent, "ContentVersion");
	cxml::add_text_child(cv, "Id", id);
	cxml::add_text_child(cv, "LabelText", label_text);
}


Luminance::Luminance (cxml::ConstNodePtr node)
	: _value(raw_convert<float>(node->content()))
	, _unit(string_to_unit(node->string_attribute("units")))
{

}


Luminance::Luminance (float value, Unit unit)
	: _unit (unit)
{
	set_value (value);
}


void
Luminance::set_value (float v)
{
	if (v < 0) {
		throw dcp::MiscError (String::compose("Invalid luminance value %1", v));
	}

	_value = v;
}


void
Luminance::as_xml (xmlpp::Element* parent, string ns) const
{
	auto lum = cxml::add_child(parent, "Luminance", ns);
	lum->set_attribute("units", unit_to_string(_unit));
	lum->add_child_text(fmt::format("{:.3}", _value));
}


string
Luminance::unit_to_string (Unit u)
{
	switch (u) {
	case Unit::CANDELA_PER_SQUARE_METRE:
		return "candela-per-square-metre";
	case Unit::FOOT_LAMBERT:
		return "foot-lambert";
	default:
		DCP_ASSERT (false);
	}

	return {};
}


Luminance::Unit
Luminance::string_to_unit (string u)
{
	if (u == "candela-per-square-metre") {
		return Unit::CANDELA_PER_SQUARE_METRE;
	} else if (u == "foot-lambert") {
		return Unit::FOOT_LAMBERT;
	}

	throw XMLError (String::compose("Invalid luminance unit %1", u));
}


float
Luminance::value_in_foot_lamberts () const
{
	switch (_unit) {
	case Unit::CANDELA_PER_SQUARE_METRE:
		return _value / 3.426;
	case Unit::FOOT_LAMBERT:
		return _value;
	default:
		DCP_ASSERT (false);
	}
}


bool
dcp::operator== (Luminance const& a, Luminance const& b)
{
	return fabs(a.value() - b.value()) < 0.001 && a.unit() == b.unit();
}


string
dcp::status_to_string (Status s)
{
	switch (s) {
	case Status::FINAL:
		return "final";
	case Status::TEMP:
		return "temp";
	case Status::PRE:
		return "pre";
	default:
		DCP_ASSERT (false);
	}
}


Status
dcp::string_to_status (string s)
{
	if (s == "final") {
		return Status::FINAL;
	} else if (s == "temp") {
		return Status::TEMP;
	} else if (s == "pre") {
		return Status::PRE;
	}

	DCP_ASSERT (false);
}


vector<dcp::Channel>
dcp::used_audio_channels ()
{
	return {
		Channel::LEFT,
		Channel::RIGHT,
		Channel::CENTRE,
		Channel::LFE,
		Channel::LS,
		Channel::RS,
		Channel::HI,
		Channel::VI,
		Channel::BSL,
		Channel::BSR,
		Channel::MOTION_DATA,
		Channel::SYNC_SIGNAL,
		Channel::SIGN_LANGUAGE
	};
}


string
dcp::formulation_to_string (dcp::Formulation formulation)
{
	switch (formulation) {
	case Formulation::MODIFIED_TRANSITIONAL_1:
		return "modified-transitional-1";
	case Formulation::MULTIPLE_MODIFIED_TRANSITIONAL_1:
		return "multiple-modified-transitional-1";
	case Formulation::DCI_ANY:
		return "dci-any";
	case Formulation::DCI_SPECIFIC:
		return "dci-specific";
	}

	DCP_ASSERT (false);
}


dcp::Formulation
dcp::string_to_formulation (string formulation)
{
	if (formulation == "modified-transitional-1") {
		return Formulation::MODIFIED_TRANSITIONAL_1;
	} else if (formulation == "multiple-modified-transitional-1") {
		return Formulation::MULTIPLE_MODIFIED_TRANSITIONAL_1;
	} else if (formulation == "dci-any") {
		return Formulation::DCI_ANY;
	} else if (formulation == "dci-specific") {
		return Formulation::DCI_SPECIFIC;
	}

	DCP_ASSERT (false);
}

