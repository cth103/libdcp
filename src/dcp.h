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

#ifndef LIBDCP_DCP_H
#define LIBDCP_DCP_H

#include <string>
#include <list>
#include <boost/shared_ptr.hpp>
#include <sigc++/sigc++.h>

/** @brief Namespace for everything in libdcp */
namespace libdcp
{

class Asset;	

/** @class DCP dcp.h libdcp/dcp.h
 *  @brief A class to create a DCP.
 *
 *  Typical use might be:
 *  @code
 *  #include <libdcp/dcp.h>
 *  using namespace std;
 *
 *  libdcp::DCP dcp ("My Film DCP", "My Film", libdcp::DCP::FEATURE, 24, 50000);
 *
 *  list<string> j2k_files;
 *  j2k_files.push_back ("1.j2c");
 *  ...
 *  j2k_files.push_back ("50000.j2c");
 *
 *  // These images are 1998x1080 pixels (DCI Flat)
 *  dcp.add_picture_asset (j2k_files, 1998, 1080);
 *
 *  list<string> wav_files;
 *  wav_files.push_back ("L.wav");
 *  wav_files.push_back ("R.wav");
 *  wav_files.push_back ("C.wav");
 *  wav_files.push_back ("Lfe.wav");
 *  wav_files.push_back ("Ls.wav");
 *  wav_files.push_back ("Rs.wav");
 *  dcp.add_sound_asset (wav_files);
 *
 *  dcp.write_xml ();
 *
 *  @endcode
 *
 *  This will create a DCP at 24 frames per second with 50000 frames, writing
 *  data to a directory "My Film DCP", naming the DCP "My Film" and marking
 *  as a Feature.  We then add the picture and sound files (which creates
 *  MXF files inside the DCP directory) and then write the required XML files.
 *
 *  If you want to report progress for long jobs (add_picture_asset() can
 *  take a long time, in particular, as it must do a lot of disk I/O for
 *  large DCPs), connect to the libdcp::DCP::Progress signal and report its parameter
 *  to the user (it will range between 0 for 0% and 1 for 100%).
 */
 
class DCP
{
public:
	enum ContentType
	{
		FEATURE,
		SHORT,
		TRAILER,
		TEST,
		TRANSITIONAL,
		RATING,
		TEASER,
		POLICY,
		PUBLIC_SERVICE_ANNOUNCEMENT,
		ADVERTISEMENT
	};

	/** Construct a DCP.
	 *  @param directory Directory to write files to.
	 *  @param name Name.
	 *  @param content_type Content type.
	 *  @param fps Frames per second.
	 *  @param length Length in frames.
	 */
	DCP (std::string directory, std::string name, ContentType content_type, int fps, int length);

	/** Add a sound asset.
	 *  @param files Pathnames of WAV files to use in the order Left, Right,
	 *  Centre, Lfe (sub), Left surround, Right surround.
	 */
	void add_sound_asset (std::list<std::string> const & files);

	/** Add a picture asset.
	 *  @param files Pathnames of JPEG2000 files, in frame order.
	 *  @param width Width of images in pixels.
	 *  @param height Height of images in pixels.
	 */
	void add_picture_asset (std::list<std::string> const & files, int width, int height);

	/** Write the required XML files to the directory that was
	 *  passed into the constructor.
	 */
	void write_xml () const;

	/** Emitted with a parameter between 0 and 1 to indicate progress
	 *  for long jobs.
	 */
	sigc::signal1<void, float> Progress;

private:

	/** Write the CPL file.
	 *  @param cpl_uuid UUID to use.
	 *  @return CPL pathname.
	 */
	std::string write_cpl (std::string cpl_uuid) const;

	/** Write the PKL file.
	 *  @param pkl_uuid UUID to use.
	 *  @param cpl_uuid UUID of the CPL file.
	 *  @param cpl_digest SHA digest of the CPL file.
	 *  @param cpl_length Length of the CPL file in bytes.
	 */
	std::string write_pkl (std::string pkl_uuid, std::string cpl_uuid, std::string cpl_digest, int cpl_length) const;
	
	/** Write the VOLINDEX file */
	void write_volindex () const;

	/** Write the ASSETMAP file.
	 *  @param cpl_uuid UUID of our CPL.
	 *  @param cpl_length Length of our CPL in bytes.
	 *  @param pkl_uuid UUID of our PKL.
	 *  @param pkl_length Length of our PKL in bytes.
	 */
	void write_assetmap (std::string cpl_uuid, int cpl_length, std::string pkl_uuid, int pkl_length) const;

	/** @param type A content type.
	 *  @return A string representation suitable for use in a CPL.
	 */
	static std::string content_type_string (ContentType type);

	/** the directory that we are writing to */
	std::string _directory;
	/** the name of the DCP */
	std::string _name;
	/** the content type of the DCP */
	ContentType _content_type;
	/** frames per second */
	int _fps;
	/** length in frames */
	int _length;
	/** assets */
	std::list<boost::shared_ptr<Asset> > _assets;
};

}

#endif
