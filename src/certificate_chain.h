/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

/** @file  src/signer_chain.h
 *  @brief Functions to make signer chains.
 */

#include <boost/filesystem.hpp>

namespace dcp {

/** Create a chain of certificates for signing things.
 *  @param openssl Name of openssl binary (if it is on the path) or full path.
 *  @return Directory (which should be deleted by the caller) containing:
 *    - ca.self-signed.pem      self-signed root certificate
 *    - intermediate.signed.pem intermediate certificate
 *    - leaf.key                leaf certificate private key
 *    - leaf.signed.pem         leaf certificate
 */
boost::filesystem::path make_certificate_chain (boost::filesystem::path openssl);

}
