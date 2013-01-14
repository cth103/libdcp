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

/** @file  src/dcp.h
 *  @brief A class to create a DCP.
 */

#ifndef LIBDCP_DCP_H
#define LIBDCP_DCP_H

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "types.h"
#include "certificates.h"

namespace xmlpp {
	class Document;
	class Element;
}

/** @brief Namespace for everything in libdcp */
namespace libdcp
{

class Asset;	
class PictureAsset;
class SoundAsset;
class SubtitleAsset;
class Reel;
class AssetMap;

class Encryption
{
public:
	Encryption (CertificateChain c, std::string const & k)
		: certificates (c)
		, signer_key (k)
	{}

	CertificateChain certificates;
	std::string signer_key;
};

/** @brief A CPL within a DCP */
class CPL
{
public:
	CPL (std::string directory, std::string name, ContentKind content_kind, int length, int frames_per_second);
	CPL (std::string directory, std::string file, boost::shared_ptr<const AssetMap> asset_map, bool require_mxfs = true);

	void add_reel (boost::shared_ptr<const Reel> reel);
	
	/** @return the length in frames */
	int length () const {
		return _length;
	}

	/** @return the type of the content, used by media servers
	 *  to categorise things (e.g. feature, trailer, etc.)
	 */
	ContentKind content_kind () const {
		return _content_kind;
	}

	std::list<boost::shared_ptr<const Reel> > reels () const {
		return _reels;
	}

	/** @return the CPL's name, as will be presented on projector
	 *  media servers and theatre management systems.
	 */
	std::string name () const {
		return _name;
	}

	/** @return the number of frames per second */
	int frames_per_second () const {
		return _fps;
	}

	std::list<boost::shared_ptr<const Asset> > assets () const;
	
	bool equals (CPL const & other, EqualityOptions options, std::list<std::string>& notes) const;
	
	void write_xml (boost::shared_ptr<Encryption>) const;
	void write_to_assetmap (std::ostream& s) const;
	void write_to_pkl (xmlpp::Element* p) const;

	boost::shared_ptr<xmlpp::Document> make_kdm (
		CertificateChain const &,
		std::string const &,
		boost::shared_ptr<const Certificate>,
		boost::posix_time::ptime from,
		boost::posix_time::ptime until
		) const;
	
private:
	std::string _directory;
	/** the name of the DCP */
	std::string _name;
	/** the content kind of the CPL */
	ContentKind _content_kind;
	/** length in frames */
	mutable int _length;
	/** frames per second */
	int _fps;
	/** reels */
	std::list<boost::shared_ptr<const Reel> > _reels;

	/** our UUID */
	std::string _uuid;
	/** a SHA1 digest of our XML */
	mutable std::string _digest;
};

/** @class DCP dcp.h libdcp/dcp.h
 *  @brief A class to create or read a DCP.
 */
	
class DCP
{
public:
	/** Construct a DCP.  You can pass an existing DCP's directory
	 *  as the parameter, or a non-existant folder to create a new
	 *  DCP in.
	 *
	 *  @param directory Directory containing the DCP's files.
	 */
	DCP (std::string directory);

	/** Read an existing DCP's data.
	 *
	 *  The DCP's XML metadata will be examined, and you can then look at the contents
	 *  of the DCP.
	 *
	 *  @param require_mxfs true to throw an exception if MXF files are missing; setting to false
	 *  can be useful for testing, but normally it should be set to true.
	 */
	void read (bool require_mxfs = true);

	/** Write the required XML files to the directory that was
	 *  passed into the constructor.
	 */
	void write_xml (boost::shared_ptr<Encryption> crypt = boost::shared_ptr<Encryption> ()) const;

	/** Compare this DCP with another, according to various options.
	 *  @param other DCP to compare this one to.
	 *  @param options Options to define just what "equality" means.
	 *  @param notes Filled in with notes about differences.
	 *  @return true if the DCPs are equal according to EqualityOptions, otherwise false.
	 */
	bool equals (DCP const & other, EqualityOptions options, std::list<std::string>& notes) const;

	/** Add a CPL to this DCP.
	 *  @param cpl CPL to add.
	 */
	void add_cpl (boost::shared_ptr<CPL> cpl);

	/** @return The list of CPLs in this DCP */
	std::list<boost::shared_ptr<const CPL> > cpls () const {
		return _cpls;
	}

	/** Emitted with a parameter between 0 and 1 to indicate progress
	 *  for long jobs.
	 */
	boost::signals2::signal<void (float)> Progress;

private:

	/** Write the PKL file.
	 *  @param pkl_uuid UUID to use.
	 */
	std::string write_pkl (std::string pkl_uuid, boost::shared_ptr<Encryption>) const;
	
	/** Write the VOLINDEX file */
	void write_volindex () const;

	/** Write the ASSETMAP file.
	 *  @param pkl_uuid UUID of our PKL.
	 *  @param pkl_length Length of our PKL in bytes.
	 */
	void write_assetmap (std::string pkl_uuid, int pkl_length) const;

	/** @return Assets in all this CPLs in this DCP */
	std::list<boost::shared_ptr<const Asset> > assets () const;

	struct Files {
		std::list<std::string> cpls;
		std::string pkl;
		std::string asset_map;
	};

	/** the directory that we are writing to */
	std::string _directory;
	/** our CPLs */
	std::list<boost::shared_ptr<const CPL> > _cpls;
};

}

#endif
