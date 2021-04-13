/*
    Copyright (C) 2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/reel_interop_subtitle_asset.h
 *  @brief ReelInteropSubtitleAsset class
 */


#include "reel_subtitle_asset.h"
#include "smpte_subtitle_asset.h"


namespace dcp {


class SMPTESubtitleAsset;


/** @class ReelSMPTESubtitleAsset
 *  @brief Part of a Reel's description which refers to an SMPTE subtitle MXF file
 */
class ReelSMPTESubtitleAsset : public ReelSubtitleAsset
{
public:
	ReelSMPTESubtitleAsset (std::shared_ptr<SMPTESubtitleAsset> asset, Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point);
	explicit ReelSMPTESubtitleAsset (std::shared_ptr<const cxml::Node>);

	std::shared_ptr<const SMPTESubtitleAsset> smpte_asset () const {
		return asset_of_type<const SMPTESubtitleAsset>();
	}

	std::shared_ptr<SMPTESubtitleAsset> smpte_asset () {
		return asset_of_type<SMPTESubtitleAsset>();
	}

private:
	boost::optional<std::string> key_type () const override {
		return std::string("MDSK");
	}
};


}


