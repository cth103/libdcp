/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_CPL_H
#define LIBDCP_CPL_H

#include "types.h"
#include "certificates.h"
#include "key.h"
#include "asset.h"
#include <libxml++/libxml++.h>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <list>

namespace dcp {
	
class Content;
class Reel;
class XMLMetadata;
class MXFMetadata;
class Signer;
class KDM;
	
/** @class CPL
 *  @brief A Composition Playlist.
 */
class CPL : public Asset
{
public:
	CPL (std::string annotation_text, ContentKind content_kind);
	CPL (boost::filesystem::path file);

	bool equals (
		CPL const & other,
		EqualityOptions options,
		boost::function<void (NoteType, std::string)> note
		) const;

	void add (boost::shared_ptr<Reel> reel);
	void add (KDM const &);

	/** @return contents of the <AnnotationText> node */
	std::string annotation_text () const {
		return _annotation_text;
	}
	
	/** @return contents of the <ContentTitleText> node */
	std::string content_title_text () const {
		return _content_title_text;
	}

	/** @return contents of the <Id> node within <ContentVersion> */
	void set_content_version_id (std::string id) {
		_content_version_id = id;
	}

	/** @return contents of the <LabelText> node within <ContentVersion> */
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

	/** @return the Content in this CPL across all its reels
	 *  (Content is picture, sound and subtitles)
	 */
	std::list<boost::shared_ptr<const Content> > content () const;

	bool encrypted () const;

	void set_mxf_keys (Key);

	void write_xml (
		boost::filesystem::path file,
		Standard standard,
		XMLMetadata,
		boost::shared_ptr<const Signer>
		) const;

	void resolve_refs (std::list<boost::shared_ptr<Object> >);

protected:
	/** @return type string for PKLs for this asset */
	std::string pkl_type () const {
		return "text/xml";
	}

private:
	std::string _annotation_text;               ///< <AnnotationText>
	std::string _issue_date;                    ///< <IssueDate>
	std::string _creator;                       ///< <Creator>
	std::string _content_title_text;            ///< <ContentTitleText>
	ContentKind _content_kind;                  ///< <ContentKind>
	std::string _content_version_id;            ///< <Id> in <ContentVersion>
	std::string _content_version_label_text;    ///< <LabelText> in <ContentVersion>
	std::list<boost::shared_ptr<Reel> > _reels;
};

}

#endif
