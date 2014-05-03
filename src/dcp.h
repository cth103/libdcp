/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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
 *  @brief DCP class.
 */

#ifndef LIBDCP_DCP_H
#define LIBDCP_DCP_H

#include "types.h"
#include "certificates.h"
#include "metadata.h"
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>
#include <string>
#include <vector>

namespace xmlpp {
	class Document;
	class Element;
}

/** @brief Namespace for everything in libdcp */
namespace dcp
{

class Content;	
class Reel;
class CPL;
class XMLMetadata;
class Signer;
class DecryptedKDM;
class Asset;
class DCPReadError;

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

	typedef std::list<boost::shared_ptr<DCPReadError> > ReadErrors;
	
	/** Read the DCP's structure into this object.
	 *  @param keep_going true to try to keep going in the face of (some) errors.
	 *  @param errors List of errors that will be added to if keep_going is true.
	 */
	void read (bool keep_going = false, ReadErrors* errors = 0);

	/** Compare this DCP with another, according to various options.
	 *  @param other DCP to compare this one to.
	 *  @param options Options to define what "equality" means.
	 *  @param note Functor to handle notes made by the equality operation.
	 *  @return true if the DCPs are equal according to EqualityOptions, otherwise false.
	 */
	bool equals (DCP const & other, EqualityOptions options, boost::function<void (NoteType, std::string)> note) const;

	void add (boost::shared_ptr<Asset> asset);

	std::list<boost::shared_ptr<CPL> > cpls () const;

	/** @return All this DCP's assets (note that CPLs are assets) */
	std::list<boost::shared_ptr<Asset> > assets () const {
		return _assets;
	}

	bool encrypted () const;

	void add (DecryptedKDM const &);

	void write_xml (
		Standard standard,
		XMLMetadata metadata = XMLMetadata (),
		boost::shared_ptr<const Signer> signer = boost::shared_ptr<const Signer> ()
	);

private:

	/** Write the PKL file.
	 *  @param pkl_uuid UUID to use.
	 */
	boost::filesystem::path write_pkl (
		Standard standard,
		std::string pkl_uuid,
		XMLMetadata metadata,
		boost::shared_ptr<const Signer> signer
		) const;
	
	void write_volindex (Standard standard) const;

	/** Write the ASSETMAP file.
	 *  @param pkl_uuid UUID of our PKL.
	 *  @param pkl_length Length of our PKL in bytes.
	 */
	void write_assetmap (Standard standard, std::string pkl_uuid, int pkl_length, XMLMetadata metadata) const;

	/** the directory that we are writing to */
	boost::filesystem::path _directory;
	/** the assets that make up this DCP */
	std::list<boost::shared_ptr<Asset> > _assets;
};

}

#endif
