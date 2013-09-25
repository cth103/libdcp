/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_PICTURE_ASSET_H
#define LIBDCP_PICTURE_ASSET_H

/** @file  src/picture_asset.h
 *  @brief An asset made up of JPEG2000 data
 */

#include <openjpeg.h>
#include "mxf_asset.h"
#include "util.h"
#include "metadata.h"

namespace libdcp
{

class MonoPictureFrame;	
class StereoPictureFrame;
class PictureAssetWriter;

/** @brief An asset made up of JPEG2000 data */
class PictureAsset : public MXFAsset
{
public:
	/** Construct a PictureAsset.
	 *  This class will not write anything to disk in this constructor, but subclasses may.
	 *  
	 *  @param directory Directory where MXF file is.
	 *  @param mxf_name Name of MXF file.
	 */
	PictureAsset (boost::filesystem::path directory, std::string mxf_name);

	/** Start a progressive write to this asset.
	 *  @param overwrite true to overwrite an existing MXF file; in this mode, writing can be resumed to a partially-written MXF; false if the
	 *  MXF file does not exist.
	 */
	virtual boost::shared_ptr<PictureAssetWriter> start_write (bool overwrite) = 0;

	virtual void read () = 0;
	virtual void create (std::vector<boost::filesystem::path> const &) {}
	virtual void create (boost::function<boost::filesystem::path (int)>) {}
	
	bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, std::string)> note) const;

	Size size () const {
		return _size;
	}

	void set_size (Size s) {
		_size = s;
	}

	void write_to_cpl (xmlpp::Element *, bool) const;

protected:	

	bool frame_buffer_equals (
		int frame, EqualityOptions opt, boost::function<void (NoteType, std::string)> note,
		uint8_t const * data_A, unsigned int size_A, uint8_t const * data_B, unsigned int size_B
		) const;

	/** picture size in pixels */
	Size _size;

private:
	std::string key_type () const;
	virtual int edit_rate_factor () const = 0;
};

/** A 2D (monoscopic) picture asset */
class MonoPictureAsset : public PictureAsset
{
public:
	MonoPictureAsset (boost::filesystem::path directory, std::string mxf_name);

	void read ();
	void create (std::vector<boost::filesystem::path> const & files);
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

/** A 3D (stereoscopic) picture asset */	
class StereoPictureAsset : public PictureAsset
{
public:
	StereoPictureAsset (boost::filesystem::path directory, std::string mxf_name);

	void read ();
	
	/** Start a progressive write to a StereoPictureAsset */
	boost::shared_ptr<PictureAssetWriter> start_write (bool);

	boost::shared_ptr<const StereoPictureFrame> get_frame (int n) const;
	bool equals (boost::shared_ptr<const Asset> other, EqualityOptions opt, boost::function<void (NoteType, std::string)> note) const;

private:
	std::string cpl_node_name () const;
	std::pair<std::string, std::string> cpl_node_attribute (bool) const;
	int edit_rate_factor () const;
};
	

}

#endif
