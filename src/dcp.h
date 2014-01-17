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
 *  @brief A class to create or read a DCP.
 */

#ifndef LIBDCP_DCP_H
#define LIBDCP_DCP_H

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>
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
class CPL;
class XMLMetadata;
class Signer;
class KDM;

namespace parse {
	class AssetMap;
}

/** @class DCP
 *  @brief A class to create or read a DCP.
 */
	
class DCP : public boost::noncopyable
{
public:
	/** Construct a DCP.  You can pass an existing DCP's directory
	 *  as the parameter, or a non-existant folder to create a new
	 *  DCP in.
	 *
	 *  @param directory Directory containing the DCP's files.
	 */
	DCP (boost::filesystem::path directory);

	void read (bool require_mxfs = true);

	/** Read an existing DCP's assets.
	 *
	 *  The DCP's XML metadata will be examined, and you can then look at the contents
	 *  of the DCP.
	 */
	void read_assets ();

	void read_cpls (bool require_mxfs = true);

	/** Write the required XML files to the directory that was
	 *  passed into the constructor.
	 */
	void write_xml (bool interop, XMLMetadata const &, boost::shared_ptr<const Signer> signer = boost::shared_ptr<const Signer> ()) const;

	/** Compare this DCP with another, according to various options.
	 *  @param other DCP to compare this one to.
	 *  @param options Options to define what "equality" means.
	 *  @return true if the DCPs are equal according to EqualityOptions, otherwise false.
	 */
	bool equals (DCP const & other, EqualityOptions options, boost::function<void (NoteType, std::string)> note) const;

	/** Add a CPL to this DCP.
	 *  @param cpl CPL to add.
	 */
	void add_cpl (boost::shared_ptr<CPL> cpl);

	/** @return The list of CPLs in this DCP */
	std::list<boost::shared_ptr<CPL> > cpls () const {
		return _cpls;
	}

	/** Add another DCP as a source of assets for this DCP.  This should be called before
	 *  ::read() on the DCP that needs the extra assets.  For example
	 *
	 *  DCP original_version ("my_dcp_OV");
	 *  DCP supplemental ("my_dcp_VF");
	 *  supplemental.add_assets_from (original_version);
	 *  supplemental.read ();
	 */
	void add_assets_from (libdcp::DCP const &);

	bool encrypted () const;

	void add_kdm (KDM const &);

	/** Emitted with a parameter between 0 and 1 to indicate progress
	 *  for long jobs.
	 */
	boost::signals2::signal<void (float)> Progress;

private:

	/** Write the PKL file.
	 *  @param pkl_uuid UUID to use.
	 */
	std::string write_pkl (std::string pkl_uuid, bool, XMLMetadata const &, boost::shared_ptr<const Signer>) const;
	
	/** Write the VOLINDEX file */
	void write_volindex (bool) const;

	/** Write the ASSETMAP file.
	 *  @param pkl_uuid UUID of our PKL.
	 *  @param pkl_length Length of our PKL in bytes.
	 */
	void write_assetmap (std::string pkl_uuid, int pkl_length, bool, XMLMetadata const &) const;

	/** @return Assets in all the CPLs in this DCP */
	std::list<boost::shared_ptr<const Asset> > assets () const;

	struct Files {
		std::list<std::string> cpls;
		std::string pkl;
		std::string asset_map;
	};

	Files _files;

	/** the directory that we are writing to */
	boost::filesystem::path _directory;
	/** our CPLs */
	std::list<boost::shared_ptr<CPL> > _cpls;

	std::list<PathAssetMap> _asset_maps;
};

}

#endif
