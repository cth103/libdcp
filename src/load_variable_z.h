/*
    Copyright (C) 2025 Carl Hetherington <cth@carlh.net>

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


#ifndef LIBDCP_LOAD_VARIABLE_Z_H
#define LIBDCP_LOAD_VARIABLE_Z_H


#include "text.h"
#include <libcxml/cxml.h>
#include <string>


namespace xmlpp {
	class Element;
}


namespace dcp {


class LoadVariableZ
{
public:
	explicit LoadVariableZ(std::string id)
		: _id(id)
		, _valid(false)
	{}

	LoadVariableZ(std::string id, std::vector<Text::VariableZPosition> positions);

	explicit LoadVariableZ(xmlpp::Element const* node);

	void as_xml(xmlpp::Element* element) const;

	std::vector<Text::VariableZPosition> positions() const;
	void set_positions(std::vector<Text::VariableZPosition> positions);

	std::string id() const {
		return _id;
	}

	bool valid() const {
		return _valid;
	}

private:
	void throw_if_invalid() const;

	std::string _id;
	std::string _original_content;
	std::vector<Text::VariableZPosition> _positions;
	bool _valid = true;
};


}


#endif
