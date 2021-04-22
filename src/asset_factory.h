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


/** @file  src/asset_factory.h
 *  @brief asset_factory() method
 */


#include <boost/filesystem.hpp>
#include <memory>


namespace dcp {


class Asset;


/** Create an Asset from a file.
 *  @param ignore_incorrect_picture_mxf_type true to ignore cases where a stereo picture asset is marked
 *  as 2D; if this is false an exception will be thrown in that case.
 *  @param ignored_incorrect_picture_mxf_type if this is non-null it will be set to true if a 3D asset was
 *  marked as 2D, otherwise it will be left alone.
 */
std::shared_ptr<Asset> asset_factory (boost::filesystem::path path, bool ignore_incorrect_picture_mxf_type, bool* found_threed_marked_as_twod = nullptr);


}
