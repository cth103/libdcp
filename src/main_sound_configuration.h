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


#ifndef LIBDCP_MAIN_SOUND_CONFIGURATION_H
#define LIBDCP_MAIN_SOUND_CONFIGURATION_H


#include "exceptions.h"
#include "types.h"
#include <string>


namespace dcp {


enum class MCASoundField
{
	FIVE_POINT_ONE,
	SEVEN_POINT_ONE,
	OTHER
};


extern std::string channel_to_mca_id(Channel c, MCASoundField field);

/** @param id MCA channel ID
 *  @return Corresponding dcp::Channel, or empty if the id is not recognised.
 */
extern boost::optional<Channel> mca_id_to_channel(std::string id);

extern std::string channel_to_mca_name(Channel c, MCASoundField field);
extern ASDCP::UL channel_to_mca_universal_label(Channel c, MCASoundField field, ASDCP::Dictionary const* dict);




class MainSoundConfiguration
{
public:
	/** Set up a MainSoundConfiguration from a string.  If the string is valid, valid() will
	 *  subsequently return true and all accessors can be called.  Otherwise, all accessors
	 *  except as_string() will throw a MainSoundConfigurationError and as_string() will
	 *  return the original invalid string.
	 */
	explicit MainSoundConfiguration(std::string);
	MainSoundConfiguration(MCASoundField field_, int channels);

	MCASoundField field() const {
		throw_if_invalid();
		return _field;
	}

	int channels() const {
		throw_if_invalid();
		return _channels.size();
	}

	boost::optional<Channel> mapping(int index) const;
	void set_mapping(int index, Channel channel);

	std::string as_string() const {
		return _configuration;
	}

	bool valid() const {
		return _valid;
	}

private:
	void update_string();

	void throw_if_invalid() const {
		if (!_valid) {
			throw MainSoundConfigurationError(_configuration);
		}
	}

	std::string _configuration;
	mutable bool _valid = true;
	mutable MCASoundField _field;
	mutable std::vector<boost::optional<Channel>> _channels;
};


}


#endif

