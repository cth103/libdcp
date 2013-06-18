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

/** @file  src/util.cc
 *  @brief Utility methods.
 */

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <openssl/sha.h>
#include <libxml++/nodes/element.h>
#include <libxml++/document.h>
#include <xmlsec/xmldsig.h>
#include <xmlsec/dl.h>
#include <xmlsec/app.h>
#include "KM_util.h"
#include "KM_fileio.h"
#include "AS_DCP.h"
#include "util.h"
#include "exceptions.h"
#include "types.h"
#include "argb_frame.h"
#include "certificates.h"
#include "gamma_lut.h"

using std::string;
using std::cout;
using std::stringstream;
using std::min;
using std::max;
using std::list;
using boost::shared_ptr;
using boost::lexical_cast;
using namespace libdcp;

/** Create a UUID.
 *  @return UUID.
 */
string
libdcp::make_uuid ()
{
	char buffer[64];
	Kumu::UUID id;
	Kumu::GenRandomValue (id);
	id.EncodeHex (buffer, 64);
	return string (buffer);
}


/** Create a digest for a file.
 *  @param filename File name.
 *  @return Digest.
 */
string
libdcp::make_digest (string filename)
{
	Kumu::FileReader reader;
	if (ASDCP_FAILURE (reader.OpenRead (filename.c_str ()))) {
		boost::throw_exception (FileError ("could not open file to compute digest", filename));
	}
	
	SHA_CTX sha;
	SHA1_Init (&sha);
	
	Kumu::ByteString read_buffer (65536);
	int done = 0;
	while (1) {
		ui32_t read = 0;
		Kumu::Result_t r = reader.Read (read_buffer.Data(), read_buffer.Capacity(), &read);
		
		if (r == Kumu::RESULT_ENDOFFILE) {
			break;
		} else if (ASDCP_FAILURE (r)) {
			boost::throw_exception (FileError ("could not read file to compute digest", filename));
		}
		
		SHA1_Update (&sha, read_buffer.Data(), read);
		done += read;
	}

	byte_t byte_buffer[20];
	SHA1_Final (byte_buffer, &sha);

	char digest[64];
	return Kumu::base64encode (byte_buffer, 20, digest, 64);
}

/** Convert a content kind to a string which can be used in a
 *  <ContentKind> node.
 *  @param kind ContentKind.
 *  @return string.
 */
string
libdcp::content_kind_to_string (ContentKind kind)
{
	switch (kind) {
	case FEATURE:
		return "feature";
	case SHORT:
		return "short";
	case TRAILER:
		return "trailer";
	case TEST:
		return "test";
	case TRANSITIONAL:
		return "transitional";
	case RATING:
		return "rating";
	case TEASER:
		return "teaser";
	case POLICY:
		return "policy";
	case PUBLIC_SERVICE_ANNOUNCEMENT:
		return "psa";
	case ADVERTISEMENT:
		return "advertisement";
	}

	assert (false);
}

/** Convert a string from a <ContentKind> node to a libdcp ContentKind.
 *  Reasonably tolerant about varying case.
 *  @param type Content kind string.
 *  @return libdcp ContentKind.
 */
libdcp::ContentKind
libdcp::content_kind_from_string (string type)
{
	/* XXX: should probably just convert type to lower-case and have done with it */
	
	if (type == "feature") {
		return FEATURE;
	} else if (type == "short") {
		return SHORT;
	} else if (type == "trailer" || type == "Trailer") {
		return TRAILER;
	} else if (type == "test") {
		return TEST;
	} else if (type == "transitional") {
		return TRANSITIONAL;
	} else if (type == "rating") {
		return RATING;
	} else if (type == "teaser" || type == "Teaser") {
		return TEASER;
	} else if (type == "policy") {
		return POLICY;
	} else if (type == "psa") {
		return PUBLIC_SERVICE_ANNOUNCEMENT;
	} else if (type == "advertisement") {
		return ADVERTISEMENT;
	}

	assert (false);
}

/** Decompress a JPEG2000 image to a bitmap.
 *  @param data JPEG2000 data.
 *  @param size Size of data in bytes.
 *  @param reduce A power of 2 by which to reduce the size of the decoded image;
 *  e.g. 0 reduces by (2^0 == 1), ie keeping the same size.
 *       1 reduces by (2^1 == 2), ie halving the size of the image.
 *  This is useful for scaling 4K DCP images down to 2K.
 *  @return openjpeg image, which the caller must call opj_image_destroy() on.
 */
opj_image_t *
libdcp::decompress_j2k (uint8_t* data, int64_t size, int reduce)
{
	opj_dinfo_t* decoder = opj_create_decompress (CODEC_J2K);
	opj_dparameters_t parameters;
	opj_set_default_decoder_parameters (&parameters);
	parameters.cp_reduce = reduce;
	opj_setup_decoder (decoder, &parameters);
	opj_cio_t* cio = opj_cio_open ((opj_common_ptr) decoder, data, size);
	opj_image_t* image = opj_decode (decoder, cio);
	if (!image) {
		opj_destroy_decompress (decoder);
		opj_cio_close (cio);
		boost::throw_exception (DCPReadError ("could not decode JPEG2000 codestream of " + lexical_cast<string> (size) + " bytes."));
	}

	opj_cio_close (cio);

	image->x1 = rint (float(image->x1) / pow (2, reduce));
	image->y1 = rint (float(image->y1) / pow (2, reduce));
	return image;
}

/** Convert an openjpeg XYZ image to RGB.
 *  @param xyz_frame Frame in XYZ.
 *  @return RGB image.
 */
shared_ptr<ARGBFrame>
libdcp::xyz_to_rgb (opj_image_t* xyz_frame, shared_ptr<const GammaLUT> lut_in, shared_ptr<const GammaLUT> lut_out)
{
	float const dci_coefficient = 48.0 / 52.37;

        /* sRGB color matrix for XYZ -> RGB.  This is the same as the one used by the Fraunhofer
	   EasyDCP player, I think.
	*/

	float const colour_matrix[3][3] = {
		{  3.24096989631653,   -1.5373831987381,  -0.498610764741898 },
		{ -0.96924364566803,    1.87596750259399,  0.0415550582110882 },
		{  0.0556300804018974, -0.203976958990097, 1.05697154998779 }
	};

	int const max_colour = pow (2, lut_out->bit_depth()) - 1;

	struct {
		double x, y, z;
	} s;
	
	struct {
		double r, g, b;
	} d;
	
	int* xyz_x = xyz_frame->comps[0].data;
	int* xyz_y = xyz_frame->comps[1].data;
	int* xyz_z = xyz_frame->comps[2].data;

	shared_ptr<ARGBFrame> argb_frame (new ARGBFrame (Size (xyz_frame->x1, xyz_frame->y1)));

	uint8_t* argb = argb_frame->data ();
	
	for (int y = 0; y < xyz_frame->y1; ++y) {
		uint8_t* argb_line = argb;
		for (int x = 0; x < xyz_frame->x1; ++x) {

			assert (*xyz_x >= 0 && *xyz_y >= 0 && *xyz_z >= 0 && *xyz_x < 4096 && *xyz_x < 4096 && *xyz_z < 4096);
			
			/* In gamma LUT */
			s.x = lut_in->lut()[*xyz_x++];
			s.y = lut_in->lut()[*xyz_y++];
			s.z = lut_in->lut()[*xyz_z++];

			/* DCI companding */
			s.x /= dci_coefficient;
			s.y /= dci_coefficient;
			s.z /= dci_coefficient;
			
			/* XYZ to RGB */
			d.r = ((s.x * colour_matrix[0][0]) + (s.y * colour_matrix[0][1]) + (s.z * colour_matrix[0][2]));
			d.g = ((s.x * colour_matrix[1][0]) + (s.y * colour_matrix[1][1]) + (s.z * colour_matrix[1][2]));
			d.b = ((s.x * colour_matrix[2][0]) + (s.y * colour_matrix[2][1]) + (s.z * colour_matrix[2][2]));
			
			d.r = min (d.r, 1.0);
			d.r = max (d.r, 0.0);
			
			d.g = min (d.g, 1.0);
			d.g = max (d.g, 0.0);
			
			d.b = min (d.b, 1.0);
			d.b = max (d.b, 0.0);
			
			/* Out gamma LUT */
			*argb_line++ = lut_out->lut()[(int) (d.b * max_colour)] * 0xff;
			*argb_line++ = lut_out->lut()[(int) (d.g * max_colour)] * 0xff;
			*argb_line++ = lut_out->lut()[(int) (d.r * max_colour)] * 0xff;
			*argb_line++ = 0xff;
		}
		
		argb += argb_frame->stride ();
	}

	return argb_frame;
}

/** @param s A string.
 *  @return true if the string contains only space, newline or tab characters, or is empty.
 */
bool
libdcp::empty_or_white_space (string s)
{
	for (size_t i = 0; i < s.length(); ++i) {
		if (s[i] != ' ' && s[i] != '\n' && s[i] != '\t') {
			return false;
		}
	}

	return true;
}

void
libdcp::init ()
{
	if (xmlSecInit() < 0) {
		throw MiscError ("could not initialise xmlsec");
	}

#ifdef XMLSEC_CRYPTO_DYNAMIC_LOADING
	if (xmlSecCryptoDLLoadLibrary (BAD_CAST XMLSEC_CRYPTO) < 0) {
		throw MiscError ("unable to load default xmlsec-crypto library");
	}
#endif
	
	if (xmlSecCryptoAppInit (0) < 0) {
		throw MiscError ("could not initialise crypto library");
	}
	
	if (xmlSecCryptoInit() < 0) {
		throw MiscError ("could not initialise xmlsec-crypto");
	}
}

void
libdcp::add_signature_value (xmlpp::Element* parent, CertificateChain const & certificates, string const & signer_key, string const & ns)
{
	parent->add_child("SignatureValue", ns);
	
	xmlpp::Element* key_info = parent->add_child("KeyInfo", ns);
	list<shared_ptr<Certificate> > c = certificates.leaf_to_root ();
	for (list<shared_ptr<Certificate> >::iterator i = c.begin(); i != c.end(); ++i) {
		xmlpp::Element* data = key_info->add_child("X509Data", ns);
		
		{
			xmlpp::Element* serial = data->add_child("X509IssuerSerial", ns);
			serial->add_child("X509IssuerName", ns)->add_child_text(
				Certificate::name_for_xml ((*i)->issuer())
				);
			serial->add_child("X509SerialNumber", ns)->add_child_text((*i)->serial());
		}
		
		data->add_child("X509Certificate", ns)->add_child_text((*i)->certificate());
	}

	xmlSecKeysMngrPtr keys_manager = xmlSecKeysMngrCreate();
	if (!keys_manager) {
		throw MiscError ("could not create keys manager");
	}
	if (xmlSecCryptoAppDefaultKeysMngrInit (keys_manager) < 0) {
		throw MiscError ("could not initialise keys manager");
	}
	
	xmlSecKeyPtr const key = xmlSecCryptoAppKeyLoad (signer_key.c_str(), xmlSecKeyDataFormatPem, 0, 0, 0);
	if (key == 0) {
		throw MiscError ("could not load signer key");
		}
	
	if (xmlSecCryptoAppDefaultKeysMngrAdoptKey (keys_manager, key) < 0) {
		xmlSecKeyDestroy (key);
		throw MiscError ("could not use signer key");
	}
	
	xmlSecDSigCtx signature_context;
	
	if (xmlSecDSigCtxInitialize (&signature_context, keys_manager) < 0) {
		throw MiscError ("could not initialise XMLSEC context");
	}
	
	if (xmlSecDSigCtxSign (&signature_context, parent->cobj()) < 0) {
		throw MiscError ("could not sign");
	}
	
	xmlSecDSigCtxFinalize (&signature_context);
	xmlSecKeysMngrDestroy (keys_manager);
}


void
libdcp::add_signer (xmlpp::Element* parent, CertificateChain const & certificates, string const & ns)
{
	xmlpp::Element* signer = parent->add_child("Signer");

	{
		xmlpp::Element* data = signer->add_child("X509Data", ns);
		
		{
			xmlpp::Element* serial_element = data->add_child("X509IssuerSerial", ns);
			serial_element->add_child("X509IssuerName", ns)->add_child_text (
				Certificate::name_for_xml (certificates.leaf()->issuer())
				);
			serial_element->add_child("X509SerialNumber", ns)->add_child_text (
				certificates.leaf()->serial()
				);
		}
		
		data->add_child("X509SubjectName", ns)->add_child_text (Certificate::name_for_xml (certificates.leaf()->subject()));
	}
}

void
libdcp::sign (xmlpp::Element* parent, CertificateChain const & certificates, string const & signer_key)
{
	add_signer (parent, certificates, "dsig");

	xmlpp::Element* signature = parent->add_child("Signature", "dsig");
	
	{
		xmlpp::Element* signed_info = signature->add_child ("SignedInfo", "dsig");
		signed_info->add_child("CanonicalizationMethod", "dsig")->set_attribute ("Algorithm", "http://www.w3.org/TR/2001/REC-xml-c14n-20010315");
		signed_info->add_child("SignatureMethod", "dsig")->set_attribute("Algorithm", "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256");
		{
			xmlpp::Element* reference = signed_info->add_child("Reference", "dsig");
			reference->set_attribute ("URI", "");
			{
				xmlpp::Element* transforms = reference->add_child("Transforms", "dsig");
				transforms->add_child("Transform", "dsig")->set_attribute (
					"Algorithm", "http://www.w3.org/2000/09/xmldsig#enveloped-signature"
					);
			}
			reference->add_child("DigestMethod", "dsig")->set_attribute("Algorithm", "http://www.w3.org/2000/09/xmldsig#sha1");
			/* This will be filled in by the signing later */
			reference->add_child("DigestValue", "dsig");
		}
	}
	
	add_signature_value (signature, certificates, signer_key, "dsig");
}

bool libdcp::operator== (libdcp::Size const & a, libdcp::Size const & b)
{
	return (a.width == b.width && a.height == b.height);
}

bool libdcp::operator!= (libdcp::Size const & a, libdcp::Size const & b)
{
	return !(a == b);
}

