/*
    Copyright (C) 2012-2019 Carl Hetherington <cth@carlh.net>

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

#include "raw_convert.h"
#include "types.h"
#include "exceptions.h"
#include "compose.hpp"
#include "dcp_assert.h"
#include <libxml++/libxml++.h>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
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

ostream& dcp::operator<< (ostream& s, dcp::Size const & a)
{
	s << a.width << "x" << a.height;
	return s;
}

/** Construct a Fraction from a string of the form <numerator> <denominator>
 *  e.g. "1 3".
 */
Fraction::Fraction (string s)
{
	vector<string> b;
	split (b, s, is_any_of (" "));
	if (b.size() != 2) {
		boost::throw_exception (XMLError ("malformed fraction " + s + " in XML node"));
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

ostream&
dcp::operator<< (ostream& s, Fraction const & f)
{
	s << f.numerator << "/" << f.denominator;
	return s;
}

/** Construct a Colour, initialising it to black. */
Colour::Colour ()
	: r (0)
	, g (0)
	, b (0)
{

}

/** Construct a Colour from R, G and B.  The values run between
 *  0 and 255.
 */
Colour::Colour (int r_, int g_, int b_)
	: r (r_)
	, g (g_)
	, b (b_)
{

}

/** Construct a Colour from an ARGB hex string; the alpha value is ignored.
 *  @param argb_hex A string of the form AARRGGBB, where e.g. RR is a two-character
 *  hex value.
 */
Colour::Colour (string argb_hex)
{
	int alpha;
	if (sscanf (argb_hex.c_str(), "%2x%2x%2x%2x", &alpha, &r, &g, &b) != 4) {
		boost::throw_exception (XMLError ("could not parse colour string"));
	}
}

/** @return An ARGB string of the form AARRGGBB, where e.g. RR is a two-character
 *  hex value.  The alpha value will always be FF (ie 255; maximum alpha).
 */
string
Colour::to_argb_string () const
{
	char buffer[9];
	snprintf (buffer, sizeof(buffer), "FF%02X%02X%02X", r, g, b);
	return buffer;
}

/** @return An RGB string of the form RRGGBB, where e.g. RR is a two-character
 *  hex value.
 */
string
Colour::to_rgb_string () const
{
	char buffer[7];
	snprintf (buffer, sizeof(buffer), "%02X%02X%02X", r, g, b);
	return buffer;
}

/** operator== for Colours.
 *  @param a First colour to compare.
 *  @param b Second colour to compare.
 */
bool
dcp::operator== (Colour const & a, Colour const & b)
{
	return (a.r == b.r && a.g == b.g && a.b == b.b);
}

/** operator!= for Colours.
 *  @param a First colour to compare.
 *  @param b Second colour to compare.
 */
bool
dcp::operator!= (Colour const & a, Colour const & b)
{
	return !(a == b);
}

ostream &
dcp::operator<< (ostream& s, Colour const & c)
{
	s << "(" << c.r << ", " << c.g << ", " << c.b << ")";
	return s;
}

string
dcp::effect_to_string (Effect e)
{
	switch (e) {
	case NONE:
		return "none";
	case BORDER:
		return "border";
	case SHADOW:
		return "shadow";
	}

	boost::throw_exception (MiscError ("unknown effect type"));
}

Effect
dcp::string_to_effect (string s)
{
	if (s == "none") {
		return NONE;
	} else if (s == "border") {
		return BORDER;
	} else if (s == "shadow") {
		return SHADOW;
	}

	boost::throw_exception (ReadError ("unknown subtitle effect type"));
}

string
dcp::halign_to_string (HAlign h)
{
	switch (h) {
	case HALIGN_LEFT:
		return "left";
	case HALIGN_CENTER:
		return "center";
	case HALIGN_RIGHT:
		return "right";
	}

	boost::throw_exception (MiscError ("unknown subtitle halign type"));
}

HAlign
dcp::string_to_halign (string s)
{
	if (s == "left") {
		return HALIGN_LEFT;
	} else if (s == "center") {
		return HALIGN_CENTER;
	} else if (s == "right") {
		return HALIGN_RIGHT;
	}

	boost::throw_exception (ReadError ("unknown subtitle halign type"));
}

string
dcp::valign_to_string (VAlign v)
{
	switch (v) {
	case VALIGN_TOP:
		return "top";
	case VALIGN_CENTER:
		return "center";
	case VALIGN_BOTTOM:
		return "bottom";
	}

	boost::throw_exception (MiscError ("unknown subtitle valign type"));
}

VAlign
dcp::string_to_valign (string s)
{
	if (s == "top") {
		return VALIGN_TOP;
	} else if (s == "center") {
		return VALIGN_CENTER;
	} else if (s == "bottom") {
		return VALIGN_BOTTOM;
	}

	boost::throw_exception (ReadError ("unknown subtitle valign type"));
}

string
dcp::direction_to_string (Direction v)
{
	switch (v) {
	case DIRECTION_LTR:
		return "ltr";
	case DIRECTION_RTL:
		return "rtl";
	case DIRECTION_TTB:
		return "ttb";
	case DIRECTION_BTT:
		return "btt";
	}

	boost::throw_exception (MiscError ("unknown subtitle direction type"));
}

Direction
dcp::string_to_direction (string s)
{
	if (s == "ltr" || s == "horizontal") {
		return DIRECTION_LTR;
	} else if (s == "rtl") {
		return DIRECTION_RTL;
	} else if (s == "ttb" || s == "vertical") {
		return DIRECTION_TTB;
	} else if (s == "btt") {
		return DIRECTION_BTT;
	}

	boost::throw_exception (ReadError ("unknown subtitle direction type"));
}

/** Convert a content kind to a string which can be used in a
 *  &lt;ContentKind&gt; node.
 *  @param kind ContentKind.
 *  @return string.
 */
string
dcp::content_kind_to_string (ContentKind kind)
{
	switch (kind) {
	case FEATURE:
		return "feature";
	case SHORT:
		return "short";
	case TRAILER:
		return "trailer";
	case TEST:
		return "test";
	case TRANSITIONAL:
		return "transitional";
	case RATING:
		return "rating";
	case TEASER:
		return "teaser";
	case POLICY:
		return "policy";
	case PUBLIC_SERVICE_ANNOUNCEMENT:
		return "psa";
	case ADVERTISEMENT:
		return "advertisement";
	case EPISODE:
		return "episode";
	case PROMO:
		return "promo";
	}

	DCP_ASSERT (false);
}

/** Convert a string from a &lt;ContentKind&gt; node to a libdcp ContentKind.
 *  Reasonably tolerant about varying case.
 *  @param kind Content kind string.
 *  @return libdcp ContentKind.
 */
dcp::ContentKind
dcp::content_kind_from_string (string kind)
{
	transform (kind.begin(), kind.end(), kind.begin(), ::tolower);

	if (kind == "feature") {
		return FEATURE;
	} else if (kind == "short") {
		return SHORT;
	} else if (kind == "trailer") {
		return TRAILER;
	} else if (kind == "test") {
		return TEST;
	} else if (kind == "transitional") {
		return TRANSITIONAL;
	} else if (kind == "rating") {
		return RATING;
	} else if (kind == "teaser") {
		return TEASER;
	} else if (kind == "policy") {
		return POLICY;
	} else if (kind == "psa") {
		return PUBLIC_SERVICE_ANNOUNCEMENT;
	} else if (kind == "advertisement") {
		return ADVERTISEMENT;
	} else if (kind == "episode") {
		return EPISODE;
	} else if (kind == "promo") {
		return PROMO;
	}

	throw BadContentKindError (kind);
}

string
dcp::marker_to_string (dcp::Marker m)
{
	switch (m) {
	case FFOC:
		return "FFOC";
	case LFOC:
		return "LFOC";
	case FFTC:
		return "FFTC";
	case LFTC:
		return "LFTC";
	case FFOI:
		return "FFOI";
	case LFOI:
		return "LFOI";
	case FFEC:
		return "FFEC";
	case LFEC:
		return "LFEC";
	case FFMC:
		return "FFMC";
	case LFMC:
		return "LFMC";
	}

	DCP_ASSERT (false);
}

dcp::Marker
dcp::marker_from_string (string s)
{
	if (s == "FFOC") {
		return FFOC;
	} else if (s == "LFOC") {
		return LFOC;
	} else if (s == "FFTC") {
		return FFTC;
	} else if (s == "LFTC") {
		return LFTC;
	} else if (s == "FFOI") {
		return FFOI;
	} else if (s == "LFOI") {
		return LFOI;
	} else if (s == "FFEC") {
		return FFEC;
	} else if (s == "LFEC") {
		return LFEC;
	} else if (s == "FFMC") {
		return FFMC;
	} else if (s == "LFMC") {
		return LFMC;
	}

	DCP_ASSERT (false);
}

Rating::Rating (cxml::ConstNodePtr node)
{
	agency = node->string_child("Agency");
	label = node->string_child("Label");
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

ostream &
dcp::operator<< (ostream& s, Rating const & r)
{
	s << r.agency << " " << r.label;
	return s;
}


ContentVersion::ContentVersion ()
	: id ("urn:uuid:" + make_uuid())
{

}


ContentVersion::ContentVersion (cxml::ConstNodePtr node)
{
	id = node->string_child("Id");
	label_text = node->string_child("LabelText");
}


ContentVersion::ContentVersion (string label_text_)
	: id ("urn:uuid:" + make_uuid())
	, label_text (label_text_)
{

}


void
ContentVersion::as_xml (xmlpp::Element* parent) const
{
	xmlpp::Node* cv = parent->add_child("ContentVersion");
	cv->add_child("Id")->add_child_text(id);
	cv->add_child("LabelText")->add_child_text(label_text);
}


Luminance::Luminance (cxml::ConstNodePtr node)
{
	_unit = string_to_unit (node->string_attribute("units"));
	_value = raw_convert<float> (node->content());
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
	xmlpp::Element* lum = parent->add_child("Luminance", ns);
	lum->set_attribute("units", unit_to_string(_unit));
	lum->add_child_text(raw_convert<string>(_value, 3));
}


string
Luminance::unit_to_string (Unit u)
{
	switch (u) {
	case CANDELA_PER_SQUARE_METRE:
		return "candela-per-square-metre";
	case FOOT_LAMBERT:
		return "foot-lambert";
	default:
		DCP_ASSERT (false);
	}

	return "";
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
		_field = FIVE_POINT_ONE;
	} else if (parts[0] == "71") {
		_field = SEVEN_POINT_ONE;
	} else {
		throw MainSoundConfigurationError (s);
	}

	vector<string> channels;
	boost::split (channels, parts[1], boost::is_any_of(","));

	if (channels.size() > 16) {
		throw MainSoundConfigurationError (s);
	}

	BOOST_FOREACH (string i, channels) {
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
	if (_field == FIVE_POINT_ONE) {
		c = "51/";
	} else {
		c = "71/";
	}

	BOOST_FOREACH (optional<Channel> i, _channels) {
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
	case FINAL:
		return "final";
	case TEMP:
		return "temp";
	case PRE:
		return "pre";
	default:
		DCP_ASSERT (false);
	}

}


Status
dcp::string_to_status (string s)
{
	if (s == "final") {
		return FINAL;
	} else if (s == "temp") {
		return TEMP;
	} else if (s == "pre") {
		return PRE;
	}

	DCP_ASSERT (false);
}


Channel
dcp::mca_id_to_channel (string id)
{
	if (id == "L") {
		return LEFT;
	} else if (id == "R") {
		return RIGHT;
	} else if (id == "C") {
		return CENTRE;
	} else if (id == "LFE") {
		return LFE;
	} else if (id == "Ls" || id == "Lss") {
		return LS;
	} else if (id == "Rs" || id == "Rss") {
		return RS;
	} else if (id == "HI") {
		return HI;
	} else if (id == "VIN") {
		return VI;
	} else if (id == "Lrs") {
		return BSL;
	} else if (id == "Rrs") {
		return BSR;
	} else if (id == "DBOX") {
		return MOTION_DATA;
	} else if (id == "FSKSync") {
		return SYNC_SIGNAL;
	} else if (id == "SLVS") {
		return SIGN_LANGUAGE;
	}

	throw UnknownChannelIdError (id);
}


string
dcp::channel_to_mca_id (Channel c, MCASoundField field)
{
	switch (c) {
	case LEFT:
		return "L";
	case RIGHT:
		return "R";
	case CENTRE:
		return "C";
	case LFE:
		return "LFE";
	case LS:
		return field == FIVE_POINT_ONE ? "Ls" : "Lss";
	case RS:
		return field == FIVE_POINT_ONE ? "Rs" : "Rss";
	case HI:
		return "HI";
	case VI:
		return "VIN";
	case BSL:
		return "Lrs";
	case BSR:
		return "Rrs";
	case MOTION_DATA:
		return "DBOX";
	case SYNC_SIGNAL:
		return "FSKSync";
	case SIGN_LANGUAGE:
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
	case LEFT:
		return "Left";
	case RIGHT:
		return "Right";
	case CENTRE:
		return "Center";
	case LFE:
		return "LFE";
	case LS:
		return field == FIVE_POINT_ONE ? "Left Surround" : "Left Side Surround";
	case RS:
		return field == FIVE_POINT_ONE ? "Right Surround" : "Right Side Surround";
	case HI:
		return "Hearing Impaired";
	case VI:
		return "Visually Impaired-Narrative";
	case BSL:
		return "Left Rear Surround";
	case BSR:
		return "Right Rear Surround";
	case MOTION_DATA:
		return "D-BOX Motion Code Primary Stream";
	case SYNC_SIGNAL:
		return "FSK Sync";
	case SIGN_LANGUAGE:
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
	case LEFT:
		return dict->ul(ASDCP::MDD_DCAudioChannel_L);
	case RIGHT:
		return dict->ul(ASDCP::MDD_DCAudioChannel_R);
	case CENTRE:
		return dict->ul(ASDCP::MDD_DCAudioChannel_C);
	case LFE:
		return dict->ul(ASDCP::MDD_DCAudioChannel_LFE);
	case LS:
		return dict->ul(field == FIVE_POINT_ONE ? ASDCP::MDD_DCAudioChannel_Ls : ASDCP::MDD_DCAudioChannel_Lss);
	case RS:
		return dict->ul(field == FIVE_POINT_ONE ? ASDCP::MDD_DCAudioChannel_Rs : ASDCP::MDD_DCAudioChannel_Rss);
	case HI:
		return dict->ul(ASDCP::MDD_DCAudioChannel_HI);
	case VI:
		return dict->ul(ASDCP::MDD_DCAudioChannel_VIN);
	case BSL:
		return dict->ul(ASDCP::MDD_DCAudioChannel_Lrs);
	case BSR:
		return dict->ul(ASDCP::MDD_DCAudioChannel_Rrs);
	case MOTION_DATA:
		return dict->ul(ASDCP::MDD_DBOXMotionCodePrimaryStream);
	case SYNC_SIGNAL:
		return ASDCP::UL(sync_signal);
	case SIGN_LANGUAGE:
		return ASDCP::UL(sign_language);
	default:
		break;
	}

	DCP_ASSERT (false);
}


vector<dcp::Channel>
dcp::used_audio_channels ()
{
	vector<dcp::Channel> c;
	c.push_back (LEFT);
	c.push_back (RIGHT);
	c.push_back (CENTRE);
	c.push_back (LFE);
	c.push_back (LS);
	c.push_back (RS);
	c.push_back (HI);
	c.push_back (VI);
	c.push_back (BSL);
	c.push_back (BSR);
	c.push_back (MOTION_DATA);
	c.push_back (SYNC_SIGNAL);
	c.push_back (SIGN_LANGUAGE);
	return c;
}

