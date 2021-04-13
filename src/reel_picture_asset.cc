/*
    Copyright (C) 2014-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/reel_picture_asset.cc
 *  @brief ReelPictureAsset class
 */


#include "compose.hpp"
#include "dcp_assert.h"
#include "picture_asset.h"
#include "raw_convert.h"
#include "reel_picture_asset.h"
#include "warnings.h"
#include <libcxml/cxml.h>
LIBDCP_DISABLE_WARNINGS
#include <libxml++/libxml++.h>
LIBDCP_ENABLE_WARNINGS
#include <iomanip>
#include <cmath>


using std::bad_cast;
using std::string;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using boost::optional;
using namespace dcp;


ReelPictureAsset::ReelPictureAsset (shared_ptr<PictureAsset> asset, int64_t entry_point)
	: ReelFileAsset (asset, asset->key_id(), asset->id(), asset->edit_rate(), asset->intrinsic_duration(), entry_point)
	, _frame_rate (asset->frame_rate ())
	, _screen_aspect_ratio (asset->screen_aspect_ratio ())
{

}


ReelPictureAsset::ReelPictureAsset (shared_ptr<const cxml::Node> node)
	: ReelFileAsset (node)
{
	_frame_rate = Fraction (node->string_child ("FrameRate"));
	try {
		_screen_aspect_ratio = Fraction (node->string_child ("ScreenAspectRatio"));
	} catch (XMLError& e) {
		/* It's not a fraction */
		try {
			float f = node->number_child<float> ("ScreenAspectRatio");
			_screen_aspect_ratio = Fraction (f * 1000, 1000);
		} catch (bad_cast& e) {

		}
	}
}


xmlpp::Node*
ReelPictureAsset::write_to_cpl (xmlpp::Node* node, Standard standard) const
{
	auto asset = ReelFileAsset::write_to_cpl (node, standard);

	asset->add_child("FrameRate")->add_child_text(String::compose("%1 %2", _frame_rate.numerator, _frame_rate.denominator));

	if (standard == Standard::INTEROP) {

		/* Allowed values for this tag from the standard */
		float allowed[] = { 1.33, 1.66, 1.77, 1.85, 2.00, 2.39 };
		int const num_allowed = sizeof(allowed) / sizeof(float);

		/* Actual ratio */
		float ratio = float (_screen_aspect_ratio.numerator) / _screen_aspect_ratio.denominator;

		/* Pick the closest and use that */
		optional<float> closest;
		optional<float> error;
		for (int i = 0; i < num_allowed; ++i) {
			float const e = fabsf (allowed[i] - ratio);
			if (!closest || e < error.get()) {
				closest = allowed[i];
				error = e;
			}
		}

		asset->add_child("ScreenAspectRatio")->add_child_text(raw_convert<string>(closest.get(), 2, true));
	} else {
		asset->add_child("ScreenAspectRatio")->add_child_text(
			String::compose ("%1 %2", _screen_aspect_ratio.numerator, _screen_aspect_ratio.denominator)
			);
	}

	return asset;
}


bool
ReelPictureAsset::equals (shared_ptr<const ReelPictureAsset> other, EqualityOptions opt, NoteHandler note) const
{
	if (!asset_equals (other, opt, note)) {
		return false;
	}
	if (!file_asset_equals (other, opt, note)) {
		return false;
	}

	auto rpa = dynamic_pointer_cast<const ReelPictureAsset>(other);
	if (!rpa) {
		return false;
	}

	if (_frame_rate != rpa->_frame_rate) {
		note (NoteType::ERROR, "frame rates differ in reel");
		return false;
	}

	if (_screen_aspect_ratio != rpa->_screen_aspect_ratio) {
		note (NoteType::ERROR, "screen aspect ratios differ in reel");
		return false;
	}

	return true;
}
