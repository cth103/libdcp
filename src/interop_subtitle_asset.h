/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

/** @file  src/interop_subtitle_asset.h
 *  @brief InteropSubtitleAsset class.
 */

#include "subtitle_asset.h"
#include <boost/filesystem.hpp>

namespace dcp {

class InteropLoadFontNode;

/** @class InteropSubtitleAsset
 *  @brief A set of subtitles to be read and/or written in the Inter-Op format.
 *
 *  Inter-Op subtitles are sometimes known as CineCanvas.
 */
class InteropSubtitleAsset : public SubtitleAsset
{
public:
	InteropSubtitleAsset ();
	explicit InteropSubtitleAsset (boost::filesystem::path file);

	bool equals (
		boost::shared_ptr<const Asset>,
		EqualityOptions,
		NoteHandler note
		) const;

	void write_to_assetmap (xmlpp::Node* node, boost::filesystem::path root) const;
	void add_to_pkl (boost::shared_ptr<PKL> pkl, boost::filesystem::path root) const;

	std::list<boost::shared_ptr<LoadFontNode> > load_font_nodes () const;

	void add_font (std::string load_id, boost::filesystem::path file);

	std::string xml_as_string () const;
	void write (boost::filesystem::path path) const;
	void resolve_fonts (std::list<boost::shared_ptr<Asset> > assets);
	void add_font_assets (std::list<boost::shared_ptr<Asset> >& assets);

	/** Set the reel number or sub-element identifier
	 *  of these subtitles.
	 *  @param n New reel number.
	 */
	void set_reel_number (std::string n) {
		_reel_number = n;
	}

	/** Set the language tag of these subtitles.
	 *  @param l New language.
	 */
	void set_language (std::string l) {
		_language = l;
	}

	/** @return title of the movie that the subtitles are for */
	void set_movie_title (std::string m) {
		_movie_title = m;
	}

	/** @return reel number or sub-element of a programme that
	 *  these subtitles refer to.
	 */
	std::string reel_number () const {
		return _reel_number;
	}

	/** @return language used in the subtitles */
	std::string language () const {
		return _language;
	}

	/** @return movie title that these subtitles are for */
	std::string movie_title () const {
		return _movie_title;
	}

	static std::string static_pkl_type (Standard) {
		return "text/xml;asdcpKind=Subtitle";
	}

protected:

	std::string pkl_type (Standard s) const {
		return static_pkl_type (s);
	}

private:
	std::string _reel_number;
	std::string _language;
	std::string _movie_title;
	std::list<boost::shared_ptr<InteropLoadFontNode> > _load_font_nodes;
};

}
