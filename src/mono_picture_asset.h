/*
    Copyright (C) 2012-2013 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_MONO_PICTURE_ASSET_H
#define LIBDCP_MONO_PICTURE_ASSET_H

#include "picture_asset.h"

namespace libdcp {

/** A 2D (monoscopic) picture asset */
class MonoPictureAsset : public PictureAsset
{
public:
	MonoPictureAsset (boost::filesystem::path directory, boost::filesystem::path mxf_name);

	void read ();

	/** The following parameters must be set up (if required) before calling this:
	 *      Interop mode (set_interop)
	 *      Edit rate    (set_edit_rate)
	 *      MXF Metadata (set_metadata)
	 */
	void create (std::vector<boost::filesystem::path> const & files);

	/** The following parameters must be set up (if required) before calling this:
	 *      Interop mode (set_interop)
	 *      Edit rate    (set_edit_rate)
	 *      MXF Metadata (set_metadata)
	 */
	void create (boost::function<boost::filesystem::path (int)> get_path);

	/** Start a progressive write to a MonoPictureAsset */
	boost::shared_ptr<PictureAssetWriter> start_write (bool);

	boost::shared_ptr<const MonoPictureFrame> get_frame (int n) const;
	bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, std::string)> note) const;

private:
	boost::filesystem::path path_from_list (int f, std::vector<boost::filesystem::path> const & files) const;
	void construct (boost::function<boost::filesystem::path (int)>, bool, MXFMetadata const &);
	std::string cpl_node_name () const;
	int edit_rate_factor () const;
};

}	

#endif
