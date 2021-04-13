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


/** @file  src/reel_file_asset.h
 *  @brief ReelFileAsset class
 */


#ifndef LIBDCP_REEL_FILE_ASSET_H
#define LIBDCP_REEL_FILE_ASSET_H


#include "reel_asset.h"
#include "ref.h"
#include <boost/optional.hpp>
#include <string>


namespace dcp {


class ReelFileAsset : public ReelAsset
{
public:
	ReelFileAsset (std::shared_ptr<Asset> asset, boost::optional<std::string> key_id, std::string id, Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point);
	explicit ReelFileAsset (std::shared_ptr<const cxml::Node> node);

	virtual xmlpp::Node* write_to_cpl (xmlpp::Node* node, Standard standard) const override;

	/** @return a Ref to our actual asset */
	Ref const & asset_ref () const {
		return _asset_ref;
	}

	/** @return a Ref to our actual asset */
	Ref & asset_ref () {
		return _asset_ref;
	}

	/** @return the asset's hash, if this ReelFileAsset has been created from one,
	 *  otherwise the hash written to the CPL for this asset (if present).
	 */
	boost::optional<std::string> hash () const {
		return _hash;
	}

	void set_hash (std::string h) {
		_hash = h;
	}

	bool file_asset_equals (std::shared_ptr<const ReelFileAsset> other, EqualityOptions opt, NoteHandler note) const;

	virtual boost::optional<std::string> key_type () const {
		return boost::none;
	}

	bool encryptable () const override {
		return static_cast<bool>(key_type());
	}

	boost::optional<std::string> key_id () const {
		return _key_id;
	}

	bool encrypted () const {
		return static_cast<bool>(key_id());
	}

protected:

	template <class T>
	std::shared_ptr<T> asset_of_type () const {
		return std::dynamic_pointer_cast<T>(_asset_ref.asset());
	}

	template <class T>
	std::shared_ptr<T> asset_of_type () {
		return std::dynamic_pointer_cast<T>(_asset_ref.asset());
	}

	/** Reference to the asset (MXF or XML file) that this reel entry
	 *  applies to.
	 */
	Ref _asset_ref;

	/** Either our asset's computed hash or the hash read in from the CPL, if it's present */
	boost::optional<std::string> _hash;
	boost::optional<std::string> _key_id; ///< The &lt;KeyId&gt; from the reel's entry for this asset, if there is one
};


}


#endif
