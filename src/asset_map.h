/*
    Copyright (C) 2012-2022 Carl Hetherington <cth@carlh.net>

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


#include "object.h"
#include "types.h"
#include <libcxml/cxml.h>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <string>
#include <vector>


namespace dcp {


class AssetMap : public Object
{
public:
	AssetMap(Standard standard, boost::optional<std::string> annotation_text, std::string issue_date, std::string issuer, std::string creator)
		: _standard(standard)
		, _annotation_text(annotation_text)
		, _issue_date(issue_date)
		, _issuer(issuer)
		, _creator(creator)
        {}

	explicit AssetMap(boost::filesystem::path path);

	boost::optional<boost::filesystem::path> path() const {
		return _path;
	}

	std::map<std::string, boost::filesystem::path> asset_ids_and_paths() const;

	std::vector<boost::filesystem::path> pkl_paths() const;

	dcp::Standard standard() const {
		return _standard;
	}

	void set_annotation_text(std::string annotation_text) {
		_annotation_text = annotation_text;
	}

	void set_issue_date(std::string issue_date) {
		_issue_date = issue_date;
	}

	void set_issuer(std::string issuer) {
		_issuer = issuer;
	}

	void set_creator(std::string creator) {
		_creator = creator;
	}

	void clear_assets();
	void add_asset(std::string id, boost::filesystem::path path, bool pkl);

	void write_xml(boost::filesystem::path path) const;

	class Asset : public Object
	{
	public:
		Asset(std::string id, boost::filesystem::path path, bool pkl)
			: Object(id)
			, _path(path)
			, _pkl(pkl)
		{}

		Asset(cxml::ConstNodePtr node, boost::filesystem::path root, dcp::Standard standard);

		boost::filesystem::path path() const {
			return _path;
		}

		bool pkl() const {
			return _pkl;
		}

		void write_xml(xmlpp::Element* asset_list, boost::filesystem::path dcp_root_directory) const;

	private:
		boost::filesystem::path _path;
		bool _pkl = false;
	};

private:
	dcp::Standard _standard;
	boost::optional<std::string> _annotation_text;
        std::string _issue_date;
        std::string _issuer;
        std::string _creator;
	std::vector<Asset> _assets;
	mutable boost::optional<boost::filesystem::path> _path;
};


}

