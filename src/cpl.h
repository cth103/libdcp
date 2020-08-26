/*
    Copyright (C) 2014-2019 Carl Hetherington <cth@carlh.net>

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

/** @file  src/cpl.h
 *  @brief CPL class.
 */

#ifndef LIBDCP_CPL_H
#define LIBDCP_CPL_H

#include "types.h"
#include "certificate.h"
#include "key.h"
#include "asset.h"
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <list>

namespace dcp {

class ReelMXF;
class Reel;
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
	explicit CPL (boost::filesystem::path file);

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

	void set_issuer (std::string issuer) {
		_issuer = issuer;
	}

	void set_creator (std::string creator) {
		_creator = creator;
	}

	void set_issue_date (std::string issue_date) {
		_issue_date = issue_date;
	}

	void set_annotation_text (std::string at) {
		_annotation_text = at;
	}

	/** @return contents of the &lt;ContentTitleText&gt; node */
	std::string content_title_text () const {
		return _content_title_text;
	}

	void set_content_title_text (std::string ct) {
		_content_title_text = ct;
	}

	/** Set the contents of the ContentVersion tag */
	void set_content_version (ContentVersion v) {
		_content_version = v;
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

	/** @return the ReelMXFs in this CPL in all reels.
	 */
	std::list<boost::shared_ptr<const ReelMXF> > reel_mxfs () const;
	std::list<boost::shared_ptr<ReelMXF> > reel_mxfs ();

	bool encrypted () const;

	void write_xml (
		boost::filesystem::path file,
		Standard standard,
		boost::shared_ptr<const CertificateChain>
		) const;

	void resolve_refs (std::list<boost::shared_ptr<Asset> >);

	int64_t duration () const;

	boost::optional<Standard> standard () const {
		return _standard;
	}

	std::list<Rating> ratings () const {
		return _ratings;
	}

	void set_ratings (std::list<Rating> r) {
		_ratings = r;
	}

	ContentVersion content_version () const {
		return _content_version;
	}

	static std::string static_pkl_type (Standard standard);

protected:
	/** @return type string for PKLs for this asset */
	std::string pkl_type (Standard standard) const;

private:
	std::string _issuer;
	std::string _creator;
	std::string _issue_date;
	std::string _annotation_text;
	std::string _content_title_text;            ///< &lt;ContentTitleText&gt;
	ContentKind _content_kind;                  ///< &lt;ContentKind&gt;
	ContentVersion _content_version;            ///< &lt;ContentVersion&gt;
	std::list<boost::shared_ptr<Reel> > _reels;
	std::list<Rating> _ratings;

	/** Standard of CPL that was read in */
	boost::optional<Standard> _standard;
};

}

#endif
