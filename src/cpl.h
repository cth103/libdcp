/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

    This program is free software; you -can redistribute it and/or modify
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

/** @file  src/cpl.h
 *  @brief CPL class.
 */

#ifndef LIBDCP_CPL_H
#define LIBDCP_CPL_H

#include "types.h"
#include "certificate.h"
#include "key.h"
#include "asset.h"
#include "metadata.h"
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <list>

namespace dcp {

class ReelAsset;
class Reel;
class XMLMetadata;
class MXFMetadata;
class CertificateChain;
class DecryptedKDM;

/** @class CPL
 *  @brief A Composition Playlist.
 */
class CPL : public Asset
{
public:
	CPL (std::string annotation_text, ContentKind content_kind);
	CPL (boost::filesystem::path file);

	bool equals (
		boost::shared_ptr<const Asset> other,
		EqualityOptions options,
		NoteHandler note
		) const;

	void add (boost::shared_ptr<Reel> reel);
	void add (DecryptedKDM const &);

	/** @return contents of the &lt;AnnotationText&gt; node */
	std::string annotation_text () const {
		return _annotation_text;
	}

	/** @return contents of the &lt;ContentTitleText&gt; node */
	std::string content_title_text () const {
		return _content_title_text;
	}

	/** @return contents of the &lt;Id&gt; node within &lt;ContentVersion&gt; */
	void set_content_version_id (std::string id) {
		_content_version_id = id;
	}

	/** @return contents of the &lt;LabelText&gt; node within &lt;ContentVersion&gt; */
	void set_content_version_label_text (std::string text) {
		_content_version_label_text = text;
	}

	/** @return the type of the content, used by media servers
	 *  to categorise things (e.g. feature, trailer, etc.)
	 */
	ContentKind content_kind () const {
		return _content_kind;
	}

	/** @return the reels in this CPL */
	std::list<boost::shared_ptr<Reel> > reels () const {
		return _reels;
	}

	/** @return the ReelAssets in this CPL in all reels.
	 */
	std::list<boost::shared_ptr<const ReelAsset> > reel_assets () const;

	bool encrypted () const;

	void set_metadata (XMLMetadata m) {
		_metadata = m;
	}

	void write_xml (
		boost::filesystem::path file,
		Standard standard,
		boost::shared_ptr<const CertificateChain>
		) const;

	void resolve_refs (std::list<boost::shared_ptr<Asset> >);

	int64_t duration () const;

protected:
	/** @return type string for PKLs for this asset */
	std::string pkl_type (Standard standard) const;

private:
	std::string _annotation_text;               ///< &lt;AnnotationText&gt;
	/** &lt;Issuer&gt;, &lt;Creator&gt; and &lt;IssueDate&gt;.  These are grouped
	 *  because they occur together in a few places.
	 */
	XMLMetadata _metadata;
	std::string _content_title_text;            ///< &lt;ContentTitleText&gt;
	ContentKind _content_kind;                  ///< &lt;ContentKind&gt;
	std::string _content_version_id;            ///< &lt;Id&gt; in &lt;ContentVersion&gt;
	std::string _content_version_label_text;    ///< &lt;LabelText&gt; in &lt;ContentVersion&gt;
	std::list<boost::shared_ptr<Reel> > _reels;
};

}

#endif
