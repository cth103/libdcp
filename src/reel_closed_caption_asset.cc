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


/** @file  src/reel_closed_caption_asset.cc
 *  @brief ReelClosedCaptionAsset class
 */


#include "dcp_assert.h"
#include "reel_closed_caption_asset.h"
#include "smpte_subtitle_asset.h"
#include "subtitle_asset.h"
#include "warnings.h"
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS


using std::string;
using std::pair;
using std::make_pair;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using boost::optional;
using namespace dcp;


ReelClosedCaptionAsset::ReelClosedCaptionAsset (std::shared_ptr<SubtitleAsset> asset, Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point)
	: ReelFileAsset (
		asset,
		dynamic_pointer_cast<SMPTESubtitleAsset>(asset) ? dynamic_pointer_cast<SMPTESubtitleAsset>(asset)->key_id() : boost::none,
		asset->id(),
		edit_rate,
		intrinsic_duration,
		entry_point
		)
{

}


ReelClosedCaptionAsset::ReelClosedCaptionAsset (std::shared_ptr<const cxml::Node> node)
	: ReelFileAsset (node)
{
	_language = node->optional_string_child ("Language");
}


bool
ReelClosedCaptionAsset::equals (shared_ptr<const ReelClosedCaptionAsset> other, EqualityOptions opt, NoteHandler note) const
{
	if (!asset_equals (other, opt, note)) {
		return false;
	}
	if (!file_asset_equals (other, opt, note)) {
		return false;
	}

	return true;
}


