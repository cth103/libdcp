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


#ifndef DCP_ASSET_LIST_H
#define DCP_ASSET_LIST_H


#include "types.h"


namespace dcp {


/** Class to extract some boilerplate from AssetMap and PKL */
class AssetList
{
public:
	AssetList() {}
	AssetList(Standard standard, boost::optional<std::string> annotation_text, std::string issue_date, std::string issuer, std::string creator)
		: _standard(standard)
		, _annotation_text(annotation_text)
		, _issue_date(issue_date)
		, _issuer(issuer)
		, _creator(creator)
	{}

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

	boost::optional<std::string> annotation_text() const {
		return _annotation_text;
	}

protected:
	dcp::Standard _standard = dcp::Standard::SMPTE;
	boost::optional<std::string> _annotation_text;
        std::string _issue_date;
        std::string _issuer;
        std::string _creator;
};


}


#endif
