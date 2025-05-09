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


/** @file  src/reel_text_asset.h
 *  @brief ReelTextAsset class.
 */


#ifndef LIBDCP_REEL_TEXT_ASSET_H
#define LIBDCP_REEL_TEXT_ASSET_H


#include "language_tag.h"
#include "reel_asset.h"
#include "reel_file_asset.h"
#include "text_asset.h"
#include "text_type.h"


struct verify_invalid_language1;
struct verify_invalid_language2;


namespace dcp {


class TextAsset;


/** @class ReelTextAsset
 *  @brief Part of a Reel's description which refers to a subtitle or caption XML/MXF file
 */
class ReelTextAsset : public ReelFileAsset
{
public:
	ReelTextAsset(TextType type, std::shared_ptr<TextAsset> asset, Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point);
	explicit ReelTextAsset(std::shared_ptr<const cxml::Node>);

	std::shared_ptr<const TextAsset> asset() const {
		return asset_of_type<const TextAsset>();
	}

	std::shared_ptr<TextAsset> asset() {
		return asset_of_type<TextAsset>();
	}

	bool equals(std::shared_ptr<const ReelTextAsset>, EqualityOptions const&, NoteHandler) const;

	void set_language (dcp::LanguageTag language);

	boost::optional<std::string> language () const {
		return _language;
	}

	TextType type() const {
		return _type;
	}

protected:
	friend struct ::verify_invalid_language1;
	friend struct ::verify_invalid_language2;

	/** As in other places, this is stored and returned as a string so that
	 *  we can tolerate non-RFC-5646 strings, but must be set as a dcp::LanguageTag
	 *  to try to ensure that we create compliant output.
	 */
	boost::optional<std::string> _language;
	TextType _type;
};


}


#endif
