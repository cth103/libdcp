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


#include "dcp_assert.h"
#include "exceptions.h"
#include "load_variable_z.h"
#include "raw_convert.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <libcxml/cxml.h>
#include <fmt/format.h>
#include <boost/algorithm/string.hpp>


using std::make_shared;
using std::string;
using std::vector;
using namespace dcp;


LoadVariableZ::LoadVariableZ(string id, vector<Text::VariableZPosition> positions)
	: _id(id)
	, _positions(positions)
{
	_valid = !positions.empty();
}


LoadVariableZ::LoadVariableZ(xmlpp::Element const* xml_node)
{
	auto node = make_shared<cxml::Node>(xml_node);
	_id = node->string_attribute("ID");

	_original_content = node->content();
	if (_original_content.empty()) {
		_valid = false;
		return;
	}

	vector<string> parts;
	boost::split(parts, _original_content, boost::is_any_of("\t\n\r "));

	for (auto const& part: parts) {
		if (part.empty()) {
			continue;
		}

		vector<string> halves;
		boost::split(halves, part, boost::is_any_of(":"));

		auto const allowed = string{"0123456789-."};

		for (auto i = 0U; i < halves[0].size(); ++i) {
			if (allowed.find(halves[0][i]) == std::string::npos) {
				_valid = false;
				return;
			}
		}

		auto const position = dcp::raw_convert<float>(halves[0]);

		if (halves.size() == 1) {
			_positions.push_back({position, 1L});
		} else if (halves.size() == 2) {
			auto duration = dcp::raw_convert<int>(halves[1]);
			if (duration <= 0) {
				_valid = false;
				return;
			}
			_positions.push_back({position, duration});
		} else {
			_valid = false;
			return;
		}
	}

	if (_positions.empty()) {
		_valid = false;
		return;
	}
}


void
LoadVariableZ::as_xml(xmlpp::Element* element) const
{
	element->set_attribute("ID", _id);

	string content;

	if (_valid) {
		for (auto const& position: _positions) {
			if (position.duration != 1) {
				content += fmt::format("{:.1f}:{} ", position.position, position.duration);
			} else {
				content += fmt::format("{:.1f} ", position.position);
			}
		}

		DCP_ASSERT(!content.empty());
		content = content.substr(0, content.length() - 1);
	} else {
		content = _original_content;
	}

	element->add_child_text(content);
}



void
LoadVariableZ::set_positions(vector<Text::VariableZPosition> positions)
{
	for (auto position: positions) {
		DCP_ASSERT(position.duration > 0);
	}
	DCP_ASSERT(!positions.empty());

	_positions = std::move(positions);
	_valid = true;
}


vector<Text::VariableZPosition>
LoadVariableZ::positions() const
{
	throw_if_invalid();
	return _positions;
}



void
LoadVariableZ::throw_if_invalid() const
{
	if (!_valid) {
		throw LoadVariableZError(_original_content);
	}
}
