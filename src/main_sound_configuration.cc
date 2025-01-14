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


#include "dcp_assert.h"
#include "exceptions.h"
#include "main_sound_configuration.h"
#include <boost/algorithm/string.hpp>
#include <vector>


using std::string;
using std::vector;
using boost::optional;
using namespace dcp;



MainSoundConfiguration::MainSoundConfiguration (string s)
	: _configuration(s)
{
	vector<string> parts;
	boost::split (parts, s, boost::is_any_of("/"));
	if (parts.empty()) {
		_valid = false;
		return;
	}

	if (parts[0] == "51") {
		_field = MCASoundField::FIVE_POINT_ONE;
	} else if (parts[0] == "71") {
		_field = MCASoundField::SEVEN_POINT_ONE;
	} else {
		_field = MCASoundField::OTHER;
	}

	if (parts.size() < 2) {
		/* I think it's OK to just have the sound field descriptor with no channels, though
		 * to me it's not clear and I might be wrong.
		 */
		return;
	}

	vector<string> channels;
	boost::split (channels, parts[1], boost::is_any_of(","));

	if (channels.size() > 16) {
		_valid = false;
		return;
	}

	for (auto i: channels) {
		if (i == "-") {
			_channels.push_back(optional<Channel>());
		} else {
			try {
				_channels.push_back(mca_id_to_channel(i));
			} catch (UnknownChannelIdError&) {
				_valid = false;
			}
		}
	}
}


MainSoundConfiguration::MainSoundConfiguration (MCASoundField field, int channels)
	: _field (field)
{
	_channels.resize (channels);
	update_string();
}


void
MainSoundConfiguration::update_string()
{
	if (!_valid) {
		return;
	}

	switch (_field) {
	case MCASoundField::FIVE_POINT_ONE:
		_configuration = "51/";
		break;
	case MCASoundField::SEVEN_POINT_ONE:
		_configuration = "71/";
		break;
	default:
		DCP_ASSERT(false);
	}

	for (auto i: _channels) {
		if (!i) {
			_configuration += "-,";
		} else {
			_configuration += channel_to_mca_id(*i, _field) + ",";
		}
	}

	if (!_configuration.empty() > 0) {
		_configuration = _configuration.substr(0, _configuration.length() - 1);
	}
}


optional<Channel>
MainSoundConfiguration::mapping (int index) const
{
	throw_if_invalid();

	DCP_ASSERT (static_cast<size_t>(index) < _channels.size());
	return _channels[index];
}


void
MainSoundConfiguration::set_mapping (int index, Channel c)
{
	throw_if_invalid();

	DCP_ASSERT (static_cast<size_t>(index) < _channels.size());
	_channels[index] = c;

	update_string();
}


Channel
dcp::mca_id_to_channel (string id)
{
	transform(id.begin(), id.end(), id.begin(), ::tolower);

	if (id == "l") {
		return Channel::LEFT;
	} else if (id == "r") {
		return Channel::RIGHT;
	} else if (id == "c") {
		return Channel::CENTRE;
	} else if (id == "lfe") {
		return Channel::LFE;
	} else if (id == "ls" || id == "lss" || id == "lslss") {
		return Channel::LS;
	} else if (id == "rs" || id == "rss" || id == "rsrss") {
		return Channel::RS;
	} else if (id == "hi") {
		return Channel::HI;
	} else if (id == "vin" || id == "vi-n") {
		return Channel::VI;
	} else if (id == "lc") {
		return Channel::LC;
	} else if (id == "rc") {
		return Channel::RC;
	} else if (id == "lrs" || id == "lsr") {
		return Channel::BSL;
	} else if (id == "rrs" || id == "rsr") {
		return Channel::BSR;
	} else if (id == "dbox" || id == "dbox2" || id == "mtn") {
		return Channel::MOTION_DATA;
	} else if (id == "sync" || id == "fsksync") {
		return Channel::SYNC_SIGNAL;
	} else if (id == "slvs") {
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


