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

/** @file  src/util.cc
 *  @brief Utility methods.
 */

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <openssl/sha.h>
#include <libxml++/nodes/element.h>
#include <libxml++/document.h>
#include <xmlsec/xmldsig.h>
#include <xmlsec/dl.h>
#include <xmlsec/app.h>
#include <xmlsec/crypto.h>
#include "KM_util.h"
#include "KM_fileio.h"
#include "AS_DCP.h"
#include "util.h"
#include "exceptions.h"
#include "types.h"
#include "argb_frame.h"
#include "certificates.h"
#include "gamma_lut.h"
#include "xyz_frame.h"

using std::string;
using std::wstring;
using std::cout;
using std::stringstream;
using std::min;
using std::max;
using std::list;
using std::setw;
using std::setfill;
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
 *  @param progress Pointer to a progress reporting function, or 0.  The function will be called
 *  with a progress value between 0 and 1.
 *  @return Digest.
 */
string
libdcp::make_digest (string filename, boost::function<void (float)>* progress)
{
	Kumu::FileReader reader;
	Kumu::Result_t r = reader.OpenRead (filename.c_str ());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (FileError ("could not open file to compute digest", filename, r));
	}
	
	SHA_CTX sha;
	SHA1_Init (&sha);

	int const buffer_size = 65536;
	Kumu::ByteString read_buffer (buffer_size);

	Kumu::fsize_t done = 0;
	Kumu::fsize_t const size = reader.Size ();
	while (1) {
		ui32_t read = 0;
		Kumu::Result_t r = reader.Read (read_buffer.Data(), read_buffer.Capacity(), &read);
		
		if (r == Kumu::RESULT_ENDOFFILE) {
			break;
		} else if (ASDCP_FAILURE (r)) {
			boost::throw_exception (FileError ("could not read file to compute digest", filename, r));
		}
		
		SHA1_Update (&sha, read_buffer.Data(), read);

		if (progress) {
			(*progress) (float (done) / size);
			done += read;
		}
	}

	byte_t byte_buffer[SHA_DIGEST_LENGTH];
	SHA1_Final (byte_buffer, &sha);

	char digest[64];
	return Kumu::base64encode (byte_buffer, SHA_DIGEST_LENGTH, digest, 64);
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
	transform (type.begin(), type.end(), type.begin(), ::tolower);
	
	if (type == "feature") {
		return FEATURE;
	} else if (type == "short") {
		return SHORT;
	} else if (type == "trailer") {
		return TRAILER;
	} else if (type == "test") {
		return TEST;
	} else if (type == "transitional") {
		return TRANSITIONAL;
	} else if (type == "rating") {
		return RATING;
	} else if (type == "teaser") {
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
 *  @return XYZ image.
 */
shared_ptr<libdcp::XYZFrame>
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

	opj_destroy_decompress (decoder);
	opj_cio_close (cio);

	image->x1 = rint (float(image->x1) / pow (2, reduce));
	image->y1 = rint (float(image->y1) / pow (2, reduce));
	return shared_ptr<XYZFrame> (new XYZFrame (image));
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
	if (xmlSecCryptoDLLoadLibrary(BAD_CAST XMLSEC_CRYPTO) < 0) {
		throw MiscError ("unable to load default xmlsec-crypto library");
	}
#endif	

	if (xmlSecCryptoAppInit(0) < 0) {
		throw MiscError ("could not initialise crypto");
	}

	if (xmlSecCryptoInit() < 0) {
		throw MiscError ("could not initialise xmlsec-crypto");
	}
}

bool libdcp::operator== (libdcp::Size const & a, libdcp::Size const & b)
{
	return (a.width == b.width && a.height == b.height);
}

bool libdcp::operator!= (libdcp::Size const & a, libdcp::Size const & b)
{
	return !(a == b);
}

/** The base64 decode routine in KM_util.cpp gives different values to both
 *  this and the command-line base64 for some inputs.  Not sure why.
 */
int
libdcp::base64_decode (string const & in, unsigned char* out, int out_length)
{
	BIO* b64 = BIO_new (BIO_f_base64 ());

	/* This means the input should have no newlines */
	BIO_set_flags (b64, BIO_FLAGS_BASE64_NO_NL);

	/* Copy our input string, removing newlines */
	char in_buffer[in.size() + 1];
	char* p = in_buffer;
	for (size_t i = 0; i < in.size(); ++i) {
		if (in[i] != '\n' && in[i] != '\r') {
			*p++ = in[i];
		}
	}
		
	BIO* bmem = BIO_new_mem_buf (in_buffer, p - in_buffer);
	bmem = BIO_push (b64, bmem);
	int const N = BIO_read (bmem, out, out_length);
	BIO_free_all (bmem);

	return N;
}

/** @param tm Local time.
 *  @return String of the form 2014-04-02T18:05:23+04:00, where the UTC offset is derived
 *  from the current system time zone.
 */
string
libdcp::tm_to_string (struct tm* tm)
{
	char buffer[64];
	strftime (buffer, 64, "%Y-%m-%dT%H:%M:%S", tm);

	/* Compute current UTC offset */
	boost::posix_time::ptime const utc_now = boost::posix_time::second_clock::universal_time ();
	boost::posix_time::ptime const now = boost::date_time::c_local_adjustor<boost::posix_time::ptime>::utc_to_local (utc_now);

	return string (buffer) + utc_offset_to_string (now - utc_now);
}

/** @param b Offset from UTC to local time.
 *  @return string of the form e.g. -01:00.
 */
string
libdcp::utc_offset_to_string (boost::posix_time::time_duration b)
{
	bool const negative = b.is_negative ();
	if (negative) {
		b = boost::posix_time::time_duration (-b.hours(), b.minutes(), 0, 0);
	}

	stringstream o;
	if (negative) {
		o << "-";
	} else {
		o << "+";
	}

	o << setw(2) << setfill('0') << b.hours() << ":" << setw(2) << setfill('0') << b.minutes();
	return o.str ();
}

string
libdcp::ptime_to_string (boost::posix_time::ptime t)
{
	struct tm t_tm = boost::posix_time::to_tm (t);
	return tm_to_string (&t_tm);
}


/* Apparently there is no way to create an ofstream using a UTF-8
   filename under Windows.  We are hence reduced to using fopen
   with this wrapper.
*/
FILE *
libdcp::fopen_boost (boost::filesystem::path p, string t)
{
#ifdef LIBDCP_WINDOWS
        wstring w (t.begin(), t.end());
	/* c_str() here should give a UTF-16 string */
        return _wfopen (p.c_str(), w.c_str ());
#else
        return fopen (p.c_str(), t.c_str ());
#endif
}
