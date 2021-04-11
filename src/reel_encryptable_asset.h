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


/** @file  src/reel_encryptable_asset.h
 *  @brief ReelEncryptableAsset class
 */


#ifndef LIBDCP_REEL_ENCRYPTABLE_ASSET_H
#define LIBDCP_REEL_ENCRYPTABLE_ASSET_H


#include "reel_file_asset.h"
#include "ref.h"
#include <boost/optional.hpp>
#include <memory>
#include <string>


namespace cxml {
	class Node;
}


namespace dcp {


/** @class ReelEncryptableAsset
 *  @brief Part of a Reel's description which refers to an asset which can be encrypted
 */
class ReelEncryptableAsset
{
public:
	explicit ReelEncryptableAsset (boost::optional<std::string> key_id);
	explicit ReelEncryptableAsset (std::shared_ptr<const cxml::Node>);
	virtual ~ReelEncryptableAsset () {}

	/** @return the 4-character key type for this MXF (MDIK, MDAK, etc.) */
	virtual std::string key_type () const = 0;

	/** @return true if a KeyId is specified for this asset, implying
	 *  that its content is encrypted.
	 */
	bool encrypted () const {
		return static_cast<bool>(_key_id);
	}

	/** @return Key ID to describe the key that encrypts this asset's
	 *  content, if there is one.
	 */
	boost::optional<std::string> key_id () const {
		return _key_id;
	}

protected:
	void write_to_cpl_encryptable (xmlpp::Node* node) const;

private:
	boost::optional<std::string> _key_id; ///< The &lt;KeyId&gt; from the reel's entry for this asset, if there is one
};


}


#endif
