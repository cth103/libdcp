/*
    Copyright (C) 2026 Carl Hetherington <cth@carlh.net>

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


#include <libcxml/cxml.h>
#include <boost/optional.hpp>
#include <string>


namespace dcp {


class MCASubDescriptor
{
public:
	explicit MCASubDescriptor(std::string tag_)
		: tag(std::move(tag_))
	{}
	explicit MCASubDescriptor(cxml::ConstNodePtr node);

	void as_xml(xmlpp::Element* node) const;

	std::string tag;
	std::string instance_id;
	std::string mca_label_dictionary_id;
	std::string mca_link_id;
	std::string mca_tag_symbol;
	boost::optional<std::string> mca_tag_name;
	boost::optional<std::string> mca_channel_id;
	boost::optional<std::string> rfc5646_spoken_language;
	boost::optional<std::string> soundfield_group_link_id;
};


}
