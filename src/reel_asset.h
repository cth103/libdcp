/*
    Copyright (C) 2014-2015 Carl Hetherington <cth@carlh.net>

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

/** @file  src/reel_asset.h
 *  @brief ReelAsset class.
 */

#ifndef LIBDCP_REEL_ASSET_H
#define LIBDCP_REEL_ASSET_H

#include "object.h"
#include "util.h"
#include "ref.h"
#include <boost/shared_ptr.hpp>

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
	ReelAsset ();
	ReelAsset (boost::shared_ptr<Asset> asset, Fraction edit_rate, int64_t intrinsic_duration, int64_t entry_point);
	ReelAsset (boost::shared_ptr<const cxml::Node>);

	virtual void write_to_cpl (xmlpp::Node* node, Standard standard) const;
	virtual bool equals (boost::shared_ptr<const ReelAsset>, EqualityOptions, NoteHandler) const;

	/** @return a Ref to our actual asset */
	Ref<Asset> const & asset_ref () const {
		return _asset_ref;
	}

	/** @return a Ref to our actual asset */
	Ref<Asset>& asset_ref () {
		return _asset_ref;
	}

	Fraction edit_rate () const {
		return _edit_rate;
	}

	int64_t intrinsic_duration () const {
		return _intrinsic_duration;
	}

	int64_t entry_point () const {
		return _entry_point;
	}

	int64_t duration () const {
		return _duration;
	}

protected:
	/** @return the node name that this asset uses in the CPL's &lt;Reel&gt; node
	 *  e.g. MainPicture, MainSound etc.
	 */
	virtual std::string cpl_node_name () const = 0;

	/** @return Any attribute that should be used on the asset's node in the
	 *  CPL.
	 */
	virtual std::pair<std::string, std::string> cpl_node_attribute (Standard) const;

	/** Reference to the asset (MXF or XML file) that this reel entry
	 *  applies to.
	 */
	Ref<Asset> _asset_ref;

private:
	std::string _annotation_text; ///< The &lt;AnnotationText&gt; from the reel's entry for this asset
	Fraction _edit_rate;          ///< The &lt;EditRate&gt; from the reel's entry for this asset
	int64_t _intrinsic_duration;  ///< The &lt;IntrinsicDuration&gt; from the reel's entry for this asset
	int64_t _entry_point;         ///< The &lt;EntryPoint&gt; from the reel's entry for this asset
	int64_t _duration;            ///< The &lt;Duration&gt; from the reel's entry for this asset
	std::string _hash;            ///< The &lt;Hash&gt; from the reel's entry for this asset
};

}

#endif
