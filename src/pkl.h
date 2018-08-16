/*
    Copyright (C) 2018 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_PKL_H
#define LIBDCP_PKL_H

#include "object.h"
#include "types.h"
#include "util.h"
#include "certificate_chain.h"
#include <libcxml/cxml.h>
#include <boost/filesystem.hpp>

namespace dcp {

class PKL : public Object
{
public:
	PKL (Standard standard, boost::optional<std::string> annotation_text, std::string issue_date, std::string issuer, std::string creator)
		: _standard (standard)
		, _annotation_text (annotation_text)
		, _issue_date (issue_date)
		, _issuer (issuer)
		, _creator (creator)
	{}

	explicit PKL (boost::filesystem::path file);

	Standard standard () const {
		return _standard;
	}

	std::string hash (std::string id) const;

	void add_asset (std::string id, boost::optional<std::string> annotation_text, std::string hash, int64_t size, std::string type);
	void write (boost::filesystem::path file, boost::shared_ptr<const CertificateChain> signer) const;

private:

	class Asset : public Object
	{
	public:
		Asset (cxml::ConstNodePtr node)
			: Object (remove_urn_uuid(node->string_child("Id")))
			, annotation_text (node->optional_string_child("AnnotationText"))
			, hash (node->string_child("Hash"))
			, size (node->number_child<int64_t>("Size"))
			, type (node->string_child("Type"))
		{}

		Asset (std::string id_, boost::optional<std::string> annotation_text_, std::string hash_, int64_t size_, std::string type_)
			: Object (id_)
			, annotation_text (annotation_text_)
			, hash (hash_)
			, size (size_)
			, type (type_)
		{}

		boost::optional<std::string> annotation_text;
		std::string hash;
		int64_t size;
		std::string type;
	};

	Standard _standard;
	boost::optional<std::string> _annotation_text;
	std::string _issue_date;
	std::string _issuer;
	std::string _creator;
	std::list<boost::shared_ptr<Asset> > _asset_list;
};

}

#endif
