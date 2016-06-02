/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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
*/

/** @file  src/dcp.h
 *  @brief DCP class.
 */

#ifndef LIBDCP_DCP_H
#define LIBDCP_DCP_H

#include "types.h"
#include "certificate.h"
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
class CertificateChain;
class DecryptedKDM;
class Asset;
class DCPReadError;

/** @class DCP
 *  @brief A class to create or read a DCP.
 */

class DCP : public boost::noncopyable
{
public:
	/** Construct a DCP.  You can pass an existing DCP's directory
	 *  as the parameter; alternatively, directory will be created
	 *  if it does not exist.  Note that if you pass an existing DCP
	 *  into this constructor it will not be read until you call ::read().
	 *
	 *  @param directory Directory containing the DCP's files.
	 */
	explicit DCP (boost::filesystem::path directory);

	typedef std::list<boost::shared_ptr<DCPReadError> > ReadErrors;

	/** Read the DCP's structure into this object.
	 *  @param keep_going true to try to keep going in the face of (some) errors.
	 *  @param errors List of errors that will be added to if keep_going is true.
	 *  @param ignore_incorrect_picture_mxf_type true to try loading MXF files marked as monoscopic
	 *  as stereoscopic if the monoscopic load fails; fixes problems some 3D DCPs that (I think)
	 *  have an incorrect descriptor in their MXF.
	 */
	void read (bool keep_going = false, ReadErrors* errors = 0, bool ignore_incorrect_picture_mxf_type = false);

	/** Compare this DCP with another, according to various options.
	 *  @param other DCP to compare this one to.
	 *  @param options Options to define what "equality" means.
	 *  @param note Functor to handle notes made by the equality operation.
	 *  @return true if the DCPs are equal according to EqualityOptions, otherwise false.
	 */
	bool equals (DCP const & other, EqualityOptions options, NoteHandler note) const;

	void add (boost::shared_ptr<CPL> cpl);

	std::list<boost::shared_ptr<CPL> > cpls () const;
	std::list<boost::shared_ptr<Asset> > assets () const;

	bool encrypted () const;

	void add (DecryptedKDM const &);

	void write_xml (
		Standard standard,
		XMLMetadata metadata = XMLMetadata (),
		boost::shared_ptr<const CertificateChain> signer = boost::shared_ptr<const CertificateChain> ()
	);

	void resolve_refs (std::list<boost::shared_ptr<Asset> > assets);

	/** @return Standard of a DCP that was read in */
	boost::optional<Standard> standard () const {
		return _standard;
	}

private:

	/** Write the PKL file.
	 *  @param pkl_uuid UUID to use.
	 */
	boost::filesystem::path write_pkl (
		Standard standard,
		std::string pkl_uuid,
		XMLMetadata metadata,
		boost::shared_ptr<const CertificateChain> signer
		) const;

	void write_volindex (Standard standard) const;

	/** Write the ASSETMAP file.
	 *  @param pkl_uuid UUID of our PKL.
	 *  @param pkl_length Length of our PKL in bytes.
	 */
	void write_assetmap (Standard standard, std::string pkl_uuid, int pkl_length, XMLMetadata metadata) const;

	/** the directory that we are writing to */
	boost::filesystem::path _directory;
	/** the CPLs that make up this DCP */
	std::list<boost::shared_ptr<CPL> > _cpls;

	/** Standard of DCP that was read in */
	boost::optional<Standard> _standard;
};

}

#endif
