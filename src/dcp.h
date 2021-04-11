/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/dcp.h
 *  @brief DCP class
 */


#ifndef LIBDCP_DCP_H
#define LIBDCP_DCP_H


#include "compose.hpp"
#include "types.h"
#include "util.h"
#include "certificate.h"
#include "metadata.h"
#include "name_format.h"
#include "verify.h"
#include "version.h"
#include <boost/signals2.hpp>
#include <memory>
#include <string>
#include <vector>


namespace xmlpp {
	class Document;
	class Element;
}


/** @brief Namespace for everything in libdcp */
namespace dcp
{


class PKL;
class Content;
class Reel;
class CPL;
class CertificateChain;
class DecryptedKDM;
class Asset;
class ReadError;


/** @class DCP
 *  @brief A class to create or read a DCP
 */
class DCP
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

	DCP (DCP const&) = delete;
	DCP& operator= (DCP const&) = delete;

	/** Read a DCP.  This method does not do any deep checking of the DCP's validity, but
	 *  if it comes across any bad things it will do one of two things.
	 *
	 *  Errors that are so serious that they prevent the method from working will result
	 *  in an exception being thrown.  For example, a missing ASSETMAP means that the DCP
	 *  can't be read without a lot of guesswork, so this will throw.
	 *
	 *  Errors that are not fatal will be added to notes, if it's non-null.  For example,
	 *  if the DCP contains a mixture of Interop and SMPTE elements this will result
	 *  in a note being added to the vector.
	 *
	 *  For more thorough checking of a DCP's contents, see dcp::verify().
	 *
	 *  @param notes List of notes that will be added to if non-0.
	 *  @param ignore_incorrect_picture_mxf_type true to try loading MXF files marked as monoscopic
	 *  as stereoscopic if the monoscopic load fails; fixes problems some 3D DCPs that (I think)
	 *  have an incorrect descriptor in their MXF.
	 */
	void read (std::vector<VerificationNote>* notes = nullptr, bool ignore_incorrect_picture_mxf_type = false);

	/** Compare this DCP with another, according to various options.
	 *  @param other DCP to compare this one to.
	 *  @param options Options to define what "equality" means.
	 *  @param note Functor to handle notes made by the equality operation.
	 *  @return true if the DCPs are equal according to EqualityOptions, otherwise false.
	 */
	bool equals (DCP const & other, EqualityOptions options, NoteHandler note) const;

	void add (std::shared_ptr<CPL> cpl);

	std::vector<std::shared_ptr<CPL>> cpls () const;

	/** @param ignore_unresolved true to silently ignore unresolved assets, otherwise
	 *  an exception is thrown if they are found.
	 *  @return All assets (including CPLs).
	 */
	std::vector<std::shared_ptr<Asset>> assets (bool ignore_unresolved = false) const;

	bool any_encrypted () const;
	bool all_encrypted () const;

	/** Add a KDM to decrypt this DCP.  This method must be called after DCP::read()
	 *  or the KDM you specify will be ignored.
	 *  @param kdm KDM to use.
	 */
	void add (DecryptedKDM const &);

	/** Write all the XML files for this DCP
	 *  @param standand INTEROP or SMPTE
	 *  @param issuer Value for the PKL and AssetMap <Issuer> tags
	 *  @param creator Value for the PKL and AssetMap <Creator> tags
	 *  @param issue_date Value for the CPL <IssueDate> tags
	 *  @param annotation_text Value for the CPL <AnnotationText> tags
	 *  @param signer Signer to use
	 *  @param name_format Name format to use for the CPL and PKL filenames
	 */
	void write_xml (
		std::string issuer = String::compose("libdcp %1", dcp::version),
		std::string creator = String::compose("libdcp %1", dcp::version),
		std::string issue_date = LocalTime().as_string(),
		std::string annotation_text = String::compose("Created by libdcp %1", dcp::version),
		std::shared_ptr<const CertificateChain> signer = std::shared_ptr<const CertificateChain>(),
		NameFormat name_format = NameFormat("%t")
	);

	void resolve_refs (std::vector<std::shared_ptr<Asset>> assets);

	/** @return Standard of a DCP that was read in */
	boost::optional<Standard> standard () const {
		return _standard;
	}

	boost::filesystem::path directory () const {
		return _directory;
	}

	/** @return PKLs if this DCP was read from an existing one, or if write_xml() has been called on it.
	 *  If neither is true, this method returns an empty vector.
	 */
	std::vector<std::shared_ptr<PKL>> pkls () const {
		return _pkls;
	}

	boost::optional<boost::filesystem::path> asset_map_path () {
		return _asset_map;
	}

	static std::vector<boost::filesystem::path> directories_from_files (std::vector<boost::filesystem::path> files);

private:

	void write_volindex (Standard standard) const;

	/** Write the ASSETMAP file.
	 *  @param pkl_uuid UUID of our PKL.
	 *  @param pkl_path Pathname of our PKL file.
	 */
	void write_assetmap (
		Standard standard, std::string pkl_uuid, boost::filesystem::path pkl_path,
		std::string issuer, std::string creator, std::string issue_date, std::string annotation_text
		) const;

	/** The directory that we are writing to */
	boost::filesystem::path _directory;
	/** The CPLs that make up this DCP */
	std::vector<std::shared_ptr<CPL>> _cpls;
	/** The PKLs that make up this DCP */
	std::vector<std::shared_ptr<PKL>> _pkls;
	/** File that the ASSETMAP was read from or last written to */
	mutable boost::optional<boost::filesystem::path> _asset_map;

	/** Standard of DCP that was read in */
	boost::optional<Standard> _standard;
};


}


#endif
