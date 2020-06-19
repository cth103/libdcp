/*
    Copyright (C) 2012-2019 Carl Hetherington <cth@carlh.net>

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

/** @file  src/reel_mxf.h
 *  @brief ReelMXF
 */

#ifndef LIBDCP_REEL_MXF_H
#define LIBDCP_REEL_MXF_H

#include "ref.h"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

namespace cxml {
	class Node;
}

namespace dcp {

/** @class ReelMXF
 *  @brief Part of a Reel's description which refers to an asset which can be encrypted.
 */
class ReelMXF
{
public:
	explicit ReelMXF (boost::shared_ptr<Asset> asset, boost::optional<std::string> key_id);
	explicit ReelMXF (boost::shared_ptr<const cxml::Node>);
	virtual ~ReelMXF () {}

	/** @return the 4-character key type for this MXF (MDIK, MDAK, etc.) */
	virtual std::string key_type () const = 0;

	/** @return a Ref to our actual asset */
	Ref const & asset_ref () const {
		return _asset_ref;
	}

	/** @return a Ref to our actual asset */
	Ref & asset_ref () {
		return _asset_ref;
	}

	/** @return the asset's hash, if this ReelMXF has been created from one,
	 *  otherwise the hash written to the CPL for this asset (if present).
	 */
	boost::optional<std::string> hash () const {
		return _hash;
	}

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

	bool mxf_equals (boost::shared_ptr<const ReelMXF> other, EqualityOptions opt, NoteHandler note) const;

protected:

	template <class T>
	boost::shared_ptr<T> asset_of_type () const {
		return boost::dynamic_pointer_cast<T> (_asset_ref.asset ());
	}

	template <class T>
	boost::shared_ptr<T> asset_of_type () {
		return boost::dynamic_pointer_cast<T> (_asset_ref.asset ());
	}

	void write_to_cpl_mxf (xmlpp::Node* node) const;

	/** Reference to the asset (MXF or XML file) that this reel entry
	 *  applies to.
	 */
	Ref _asset_ref;

private:
	boost::optional<std::string> _key_id; ///< The &lt;KeyId&gt; from the reel's entry for this asset, if there is one
	/** Either our asset's computed hash or the hash read in from the CPL, if it's present */
	boost::optional<std::string> _hash;
};

}

#endif
