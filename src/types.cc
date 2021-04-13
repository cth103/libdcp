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


/** Construct a Fraction from a string of the form <numerator> <denominator>
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


Colour::Colour ()
{

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
dcp::halign_to_string (HAlign h)
{
	switch (h) {
	case HAlign::LEFT:
		return "left";
	case HAlign::CENTER:
		return "center";
	case HAlign::RIGHT:
		return "right";
	}

	boost::throw_exception (MiscError("unknown subtitle halign type"));
}


HAlign
dcp::string_to_halign (string s)
{
	if (s == "left") {
		return HAlign::LEFT;
	} else if (s == "center") {
		return HAlign::CENTER;
	} else if (s == "right") {
		return HAlign::RIGHT;
	}

	boost::throw_exception (ReadError("unknown subtitle halign type"));
}


string
dcp::valign_to_string (VAlign v)
{
	switch (v) {
	case VAlign::TOP:
		return "top";
	case VAlign::CENTER:
		return "center";
	case VAlign::BOTTOM:
		return "bottom";
	}

	boost::throw_exception (MiscError("unknown subtitle valign type"));
}


VAlign
dcp::string_to_valign (string s)
{
	if (s == "top") {
		return VAlign::TOP;
	} else if (s == "center") {
		return VAlign::CENTER;
	} else if (s == "bottom") {
		return VAlign::BOTTOM;
	}

	boost::throw_exception (ReadError("unknown subtitle valign type"));
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


/** Convert a content kind to a string which can be used in a
 *  &lt;ContentKind&gt; node
 *  @param kind ContentKind
 *  @return string
 */
string
dcp::content_kind_to_string (ContentKind kind)
{
	switch (kind) {
	case ContentKind::FEATURE:
		return "feature";
	case ContentKind::SHORT:
		return "short";
	case ContentKind::TRAILER:
		return "trailer";
	case ContentKind::TEST:
		return "test";
	case ContentKind::TRANSITIONAL:
		return "transitional";
	case ContentKind::RATING:
		return "rating";
	case ContentKind::TEASER:
		return "teaser";
	case ContentKind::POLICY:
		return "policy";
	case ContentKind::PUBLIC_SERVICE_ANNOUNCEMENT:
		return "psa";
	case ContentKind::ADVERTISEMENT:
		return "advertisement";
	case ContentKind::EPISODE:
		return "episode";
	case ContentKind::PROMO:
		return "promo";
	}

	DCP_ASSERT (false);
}


/** Convert a string from a &lt;ContentKind&gt; node to a libdcp ContentKind.
 *  Reasonably tolerant about varying case
 *  @param kind Content kind string
 *  @return libdcp ContentKind
 */
dcp::ContentKind
dcp::content_kind_from_string (string kind)
{
	transform (kind.begin(), kind.end(), kind.begin(), ::tolower);

	if (kind == "feature") {
		return ContentKind::FEATURE;
	} else if (kind == "short") {
		return ContentKind::SHORT;
	} else if (kind == "trailer") {
		return ContentKind::TRAILER;
	} else if (kind == "test") {
		return ContentKind::TEST;
	} else if (kind == "transitional") {
		return ContentKind::TRANSITIONAL;
	} else if (kind == "rating") {
		return ContentKind::RATING;
	} else if (kind == "teaser") {
		return ContentKind::TEASER;
	} else if (kind == "policy") {
		return ContentKind::POLICY;
	} else if (kind == "psa") {
		return ContentKind::PUBLIC_SERVICE_ANNOUNCEMENT;
	} else if (kind == "advertisement") {
		return ContentKind::ADVERTISEMENT;
	} else if (kind == "episode") {
		return ContentKind::EPISODE;
	} else if (kind == "promo") {
		return ContentKind::PROMO;
	}

	throw BadContentKindError (kind);
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
	}

	DCP_ASSERT (false);
}


Rating::Rating (cxml::ConstNodePtr node)
	: agency(node->string_child("Agency"))
	, label(node->string_child("Label"))
{
	node->done ();
}


void
Rating::as_xml (xmlpp::Element* parent) const
{
	parent->add_child("Agency")->add_child_text(agency);
	parent->add_child("Label")->add_child_text(label);
}


bool
dcp::operator== (Rating const & a, Rating const & b)
{
	return a.agency == b.agency && a.label == b.label;
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
	auto cv = parent->add_child("ContentVersion");
	cv->add_child("Id")->add_child_text(id);
	cv->add_child("LabelText")->add_child_text(label_text);
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
	auto lum = parent->add_child("Luminance", ns);
	lum->set_attribute("units", unit_to_string(_unit));
	lum->add_child_text(raw_convert<string>(_value, 3));
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


MainSoundConfiguration::MainSoundConfiguration (string s)
{
	vector<string> parts;
	boost::split (parts, s, boost::is_any_of("/"));
	if (parts.size() != 2) {
		throw MainSoundConfigurationError (s);
	}

	if (parts[0] == "51") {
		_field = MCASoundField::FIVE_POINT_ONE;
	} else if (parts[0] == "71") {
		_field = MCASoundField::SEVEN_POINT_ONE;
	} else {
		throw MainSoundConfigurationError (s);
	}

	vector<string> channels;
	boost::split (channels, parts[1], boost::is_any_of(","));

	if (channels.size() > 16) {
		throw MainSoundConfigurationError (s);
	}

	for (auto i: channels) {
		if (i == "-") {
			_channels.push_back(optional<Channel>());
		} else {
			_channels.push_back(mca_id_to_channel(i));
		}
	}
}


MainSoundConfiguration::MainSoundConfiguration (MCASoundField field, int channels)
	: _field (field)
{
	_channels.resize (channels);
}


string
MainSoundConfiguration::to_string () const
{
	string c;
	if (_field == MCASoundField::FIVE_POINT_ONE) {
		c = "51/";
	} else {
		c = "71/";
	}

	for (auto i: _channels) {
		if (!i) {
			c += "-,";
		} else {
			c += channel_to_mca_id(*i, _field) + ",";
		}
	}

	if (c.length() > 0) {
		c = c.substr(0, c.length() - 1);
	}

	return c;
}


optional<Channel>
MainSoundConfiguration::mapping (int index) const
{
	DCP_ASSERT (static_cast<size_t>(index) < _channels.size());
	return _channels[index];
}


void
MainSoundConfiguration::set_mapping (int index, Channel c)
{
	DCP_ASSERT (static_cast<size_t>(index) < _channels.size());
	_channels[index] = c;
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


Channel
dcp::mca_id_to_channel (string id)
{
	if (id == "L") {
		return Channel::LEFT;
	} else if (id == "R") {
		return Channel::RIGHT;
	} else if (id == "C") {
		return Channel::CENTRE;
	} else if (id == "LFE") {
		return Channel::LFE;
	} else if (id == "Ls" || id == "Lss") {
		return Channel::LS;
	} else if (id == "Rs" || id == "Rss") {
		return Channel::RS;
	} else if (id == "HI") {
		return Channel::HI;
	} else if (id == "VIN") {
		return Channel::VI;
	} else if (id == "Lrs") {
		return Channel::BSL;
	} else if (id == "Rrs") {
		return Channel::BSR;
	} else if (id == "DBOX") {
		return Channel::MOTION_DATA;
	} else if (id == "FSKSync") {
		return Channel::SYNC_SIGNAL;
	} else if (id == "SLVS") {
		return Channel::SIGN_LANGUAGE;
	}

	throw UnknownChannelIdError (id);
}


string
dcp::channel_to_mca_id (Channel c, MCASoundField field)
{
	switch (c) {
	case Channel::LEFT:
		return "L";
	case Channel::RIGHT:
		return "R";
	case Channel::CENTRE:
		return "C";
	case Channel::LFE:
		return "LFE";
	case Channel::LS:
		return field == MCASoundField::FIVE_POINT_ONE ? "Ls" : "Lss";
	case Channel::RS:
		return field == MCASoundField::FIVE_POINT_ONE ? "Rs" : "Rss";
	case Channel::HI:
		return "HI";
	case Channel::VI:
		return "VIN";
	case Channel::BSL:
		return "Lrs";
	case Channel::BSR:
		return "Rrs";
	case Channel::MOTION_DATA:
		return "DBOX";
	case Channel::SYNC_SIGNAL:
		return "FSKSync";
	case Channel::SIGN_LANGUAGE:
		return "SLVS";
	default:
		break;
	}

	DCP_ASSERT (false);
}


string
dcp::channel_to_mca_name (Channel c, MCASoundField field)
{
	switch (c) {
	case Channel::LEFT:
		return "Left";
	case Channel::RIGHT:
		return "Right";
	case Channel::CENTRE:
		return "Center";
	case Channel::LFE:
		return "LFE";
	case Channel::LS:
		return field == MCASoundField::FIVE_POINT_ONE ? "Left Surround" : "Left Side Surround";
	case Channel::RS:
		return field == MCASoundField::FIVE_POINT_ONE ? "Right Surround" : "Right Side Surround";
	case Channel::HI:
		return "Hearing Impaired";
	case Channel::VI:
		return "Visually Impaired-Narrative";
	case Channel::BSL:
		return "Left Rear Surround";
	case Channel::BSR:
		return "Right Rear Surround";
	case Channel::MOTION_DATA:
		return "D-BOX Motion Code Primary Stream";
	case Channel::SYNC_SIGNAL:
		return "FSK Sync";
	case Channel::SIGN_LANGUAGE:
		return "Sign Language Video Stream";
	default:
		break;
	}

	DCP_ASSERT (false);
}


ASDCP::UL
dcp::channel_to_mca_universal_label (Channel c, MCASoundField field, ASDCP::Dictionary const* dict)
{
	static byte_t sync_signal[] = {
		0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d, 0x03, 0x02, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00
	};

	static byte_t sign_language[] = {
		0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d, 0x0d, 0x0f, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00
	};

	switch (c) {
	case Channel::LEFT:
		return dict->ul(ASDCP::MDD_DCAudioChannel_L);
	case Channel::RIGHT:
		return dict->ul(ASDCP::MDD_DCAudioChannel_R);
	case Channel::CENTRE:
		return dict->ul(ASDCP::MDD_DCAudioChannel_C);
	case Channel::LFE:
		return dict->ul(ASDCP::MDD_DCAudioChannel_LFE);
	case Channel::LS:
		return dict->ul(field == MCASoundField::FIVE_POINT_ONE ? ASDCP::MDD_DCAudioChannel_Ls : ASDCP::MDD_DCAudioChannel_Lss);
	case Channel::RS:
		return dict->ul(field == MCASoundField::FIVE_POINT_ONE ? ASDCP::MDD_DCAudioChannel_Rs : ASDCP::MDD_DCAudioChannel_Rss);
	case Channel::HI:
		return dict->ul(ASDCP::MDD_DCAudioChannel_HI);
	case Channel::VI:
		return dict->ul(ASDCP::MDD_DCAudioChannel_VIN);
	case Channel::BSL:
		return dict->ul(ASDCP::MDD_DCAudioChannel_Lrs);
	case Channel::BSR:
		return dict->ul(ASDCP::MDD_DCAudioChannel_Rrs);
	case Channel::MOTION_DATA:
		return dict->ul(ASDCP::MDD_DBOXMotionCodePrimaryStream);
	case Channel::SYNC_SIGNAL:
		return ASDCP::UL(sync_signal);
	case Channel::SIGN_LANGUAGE:
		return ASDCP::UL(sign_language);
	default:
		break;
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

