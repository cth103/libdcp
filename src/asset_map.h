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


#include "asset_list.h"
#include "object.h"
#include <libcxml/cxml.h>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <string>
#include <vector>


namespace dcp {


class AssetMap : public Object, public AssetList
{
public:
	AssetMap(Standard standard, boost::optional<std::string> annotation_text, std::string issue_date, std::string issuer, std::string creator)
		: AssetList(standard, annotation_text, issue_date, issuer, creator)
	{}

	explicit AssetMap(boost::filesystem::path path);

	boost::optional<boost::filesystem::path> file() const {
		return _file;
	}

	std::map<std::string, boost::filesystem::path> asset_ids_and_paths() const;

	std::vector<boost::filesystem::path> pkl_paths() const;

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

	std::vector<Asset> assets() const {
		return _assets;
	}

private:
	std::vector<Asset> _assets;
	mutable boost::optional<boost::filesystem::path> _file;
};


}

