/*
    Copyright (C) 2018-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/pkl.cc
 *  @brief PKL class
 */


#ifndef LIBDCP_PKL_H
#define LIBDCP_PKL_H


#include "asset_list.h"
#include "object.h"
#include "util.h"
#include "certificate_chain.h"
#include <libcxml/cxml.h>
#include <boost/filesystem.hpp>


namespace dcp {


class VerificationNote;


class PKL : public Object, public AssetList
{
public:
	PKL (Standard standard, boost::optional<std::string> annotation_text, std::string issue_date, std::string issuer, std::string creator)
		: AssetList(standard, annotation_text, issue_date, issuer, creator)
	{}

	explicit PKL(boost::filesystem::path file, std::vector<dcp::VerificationNote>* notes = nullptr);

	boost::optional<std::string> hash (std::string id) const;
	boost::optional<std::string> type (std::string id) const;

	void clear_assets();
	void add_asset(std::string id, boost::optional<std::string> annotation_text, std::string hash, int64_t size, std::string type, std::string original_filename);
	void write_xml (boost::filesystem::path file, std::shared_ptr<const CertificateChain> signer) const;

	/** @return the most recent disk file used to read or write this PKL, if there is one */
	boost::optional<boost::filesystem::path> file () const {
		return _file;
	}

	class Asset : public Object
	{
	public:
		Asset (cxml::ConstNodePtr node)
			: Object (remove_urn_uuid(node->string_child("Id")))
			, _annotation_text (node->optional_string_child("AnnotationText"))
			, _hash (node->string_child("Hash"))
			, _size (node->number_child<int64_t>("Size"))
			, _type (node->string_child("Type"))
			, _original_filename(node->optional_string_child("OriginalFileName"))
		{}

		Asset(std::string id, boost::optional<std::string> annotation_text, std::string hash, int64_t size, std::string type, std::string original_filename)
			: Object (id)
			, _annotation_text (annotation_text)
			, _hash (hash)
			, _size (size)
			, _type (type)
			, _original_filename(original_filename)
		{}

		boost::optional<std::string> annotation_text () const {
			return _annotation_text;
		}

		std::string hash () const {
			return _hash;
		}

		int64_t size () const {
			return _size;
		}

		std::string type () const {
			return _type;
		}

		boost::optional<std::string> original_filename() const {
			return _original_filename;
		}

	private:
		boost::optional<std::string> _annotation_text;
		std::string _hash;
		int64_t _size = 0;
		std::string _type;
		boost::optional<std::string> _original_filename;
	};

	std::vector<std::shared_ptr<Asset>> assets() const {
		return _assets;
	}

private:
	std::vector<std::shared_ptr<Asset>> _assets;
	/** The most recent disk file used to read or write this PKL */
	mutable boost::optional<boost::filesystem::path> _file;
};


}


#endif
