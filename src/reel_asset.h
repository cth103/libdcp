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


/** @file  src/reel_asset.h
 *  @brief ReelAsset class
 */


#ifndef LIBDCP_REEL_ASSET_H
#define LIBDCP_REEL_ASSET_H


#include "object.h"
#include "util.h"
#include "ref.h"
#include <memory>


namespace cxml {
	class Node;
}


namespace xmlpp {
	class Node;
}


namespace dcp {


class Asset;


/** @class ReelAsset
 *  @brief An entry in a &lt;Reel&gt; which refers to a use of a piece of content.
 *
 *  This class encapsulates the XML that exists in a &lt;Reel&gt; to say
 *  that a piece of content is used in this reel.  It does not
 *  describe the content itself (but links to an Asset object which does).
 */
class ReelAsset : public Object
{
public:
	/** Construct a ReelAsset
	 *  @param id ID of this ReelAsset (which is that of the MXF, if there is one)
	 *  @param edit_rate Edit rate for the asset
	 *  @param intrinsic_duration Intrinsic duration of this asset
	 *  @param entry_point Entry point to use in that asset
	 */
	ReelAsset (std::string id, Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point);

	explicit ReelAsset (std::shared_ptr<const cxml::Node>);

	virtual xmlpp::Node* write_to_cpl (xmlpp::Node* node, Standard standard) const;

	virtual bool encryptable () const {
		return false;
	}

	Fraction edit_rate () const {
		return _edit_rate;
	}

	int64_t intrinsic_duration () const {
		return _intrinsic_duration;
	}

	void set_entry_point (int64_t e) {
		_entry_point = e;
	}

	void unset_entry_point () {
		_entry_point = boost::none;
	}

	boost::optional<int64_t> entry_point () const {
		return _entry_point;
	}

	void set_duration (int64_t d) {
		_duration = d;
	}

	boost::optional<int64_t> duration () const {
		return _duration;
	}

	/** @return <Duration>, or <IntrinsicDuration> - <EntryPoint> if <Duration> is not present */
	int64_t actual_duration () const;

	std::string annotation_text () const {
		return _annotation_text;
	}

	void set_annotation_text (std::string at) {
		_annotation_text = at;
	}

	bool asset_equals (std::shared_ptr<const ReelAsset>, EqualityOptions, NoteHandler) const;

protected:

	/** @return the node name that this asset uses in the CPL's &lt;Reel&gt; node
	 *  e.g. MainPicture, MainSound etc.
	 */
	virtual std::string cpl_node_name (Standard) const = 0;

	/** @return Any attribute that should be used on the asset's node in the CPL */
	virtual std::pair<std::string, std::string> cpl_node_attribute (Standard) const;

	/** @return Any namespace that should be used on the asset's node in the CPL */
	virtual std::pair<std::string, std::string> cpl_node_namespace () const;

	int64_t _intrinsic_duration = 0;       ///< The &lt;IntrinsicDuration&gt; from the reel's entry for this asset
	boost::optional<int64_t> _duration;    ///< The &lt;Duration&gt; from the reel's entry for this asset, if present

private:
	std::string _annotation_text;          ///< The &lt;AnnotationText&gt; from the reel's entry for this asset
	Fraction _edit_rate;                   ///< The &lt;EditRate&gt; from the reel's entry for this asset
	boost::optional<int64_t> _entry_point; ///< The &lt;EntryPoint&gt; from the reel's entry for this asset
};


}


#endif
