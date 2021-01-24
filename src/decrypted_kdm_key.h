/*
    Copyright (C) 2013-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/decrypted_kdm_key.h
 *  @brief DecryptedKDMKey class
 */


#ifndef LIBDCP_DECRYPTED_KDM_KEY_H
#define LIBDCP_DECRYPTED_KDM_KEY_H


#include "key.h"
#include "types.h"
#include <boost/optional.hpp>


namespace dcp {


/** @class DecryptedKDMKey
 *  @brief An un- or de-crypted key from a KDM
 */
class DecryptedKDMKey
{
public:
	DecryptedKDMKey (boost::optional<std::string> type, std::string id, Key key, std::string cpl_id, Standard standard)
		: _type (type)
		, _id (id)
		, _key (key)
		, _cpl_id (cpl_id)
		, _standard (standard)
	{}

	boost::optional<std::string> type () const {
		return _type;
	}

	std::string id () const {
		return _id;
	}

	Key key () const {
		return _key;
	}

	std::string cpl_id () const {
		return _cpl_id;
	}

	Standard standard () const {
		return _standard;
	}

private:
	boost::optional<std::string> _type;
	std::string _id;
	Key _key;
	std::string _cpl_id;
	Standard _standard;
};


bool operator== (DecryptedKDMKey const &, DecryptedKDMKey const &);


}


#endif
