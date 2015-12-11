/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

#include "j2k.h"
#include "exceptions.h"
#include "openjpeg_image.h"
#include "data.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include <openjpeg.h>
#include <cmath>
#include <iostream>

using std::min;
using std::pow;
using boost::shared_ptr;
using boost::shared_array;
using namespace dcp;

shared_ptr<dcp::OpenJPEGImage>
dcp::decompress_j2k (Data data, int reduce)
{
	return dcp::decompress_j2k (data.data().get(), data.size(), reduce);
}

class ReadBuffer
{
public:
	ReadBuffer (uint8_t* data, int64_t size)
		: _data (data)
		, _size (size)
		, _offset (0)
	{}

	OPJ_SIZE_T read (void* buffer, OPJ_SIZE_T nb_bytes)
	{
		int64_t N = min (nb_bytes, _size - _offset);
		memcpy (buffer, _data + _offset, N);
		_offset += N;
		return N;
	}

private:
	uint8_t* _data;
	OPJ_SIZE_T _size;
	OPJ_SIZE_T _offset;
};

static OPJ_SIZE_T
read_function (void* buffer, OPJ_SIZE_T nb_bytes, void* data)
{
	return reinterpret_cast<ReadBuffer*>(data)->read (buffer, nb_bytes);
}

static void
read_free_function (void* data)
{
	delete reinterpret_cast<ReadBuffer*>(data);
}

/** Decompress a JPEG2000 image to a bitmap.
 *  @param data JPEG2000 data.
 *  @param size Size of data in bytes.
 *  @param reduce A power of 2 by which to reduce the size of the decoded image;
 *  e.g. 0 reduces by (2^0 == 1), ie keeping the same size.
 *       1 reduces by (2^1 == 2), ie halving the size of the image.
 *  This is useful for scaling 4K DCP images down to 2K.
 *  @return OpenJPEGImage.
 */
shared_ptr<dcp::OpenJPEGImage>
dcp::decompress_j2k (uint8_t* data, int64_t size, int reduce)
{
	uint8_t const jp2_magic[] = {
		0x00,
		0x00,
		0x00,
		0x0c,
		'j',
		'P',
		0x20,
		0x20
	};

	OPJ_CODEC_FORMAT format = OPJ_CODEC_J2K;
	if (size >= int (sizeof (jp2_magic)) && memcmp (data, jp2_magic, sizeof (jp2_magic)) == 0) {
		format = OPJ_CODEC_JP2;
	}

	opj_codec_t* decoder = opj_create_decompress (format);
	if (!decoder) {
		boost::throw_exception (DCPReadError ("could not create JPEG2000 decompresser"));
	}
	opj_dparameters_t parameters;
	opj_set_default_decoder_parameters (&parameters);
	parameters.cp_reduce = reduce;
	opj_setup_decoder (decoder, &parameters);

	opj_stream_t* stream = opj_stream_default_create (OPJ_TRUE);
	if (!stream) {
		throw MiscError ("could not create JPEG2000 stream");
	}

	opj_stream_set_read_function (stream, read_function);
	ReadBuffer* buffer = new ReadBuffer (data, size);
	opj_stream_set_user_data (stream, buffer, read_free_function);
	opj_stream_set_user_data_length (stream, size);

	opj_image_t* image = 0;
	opj_read_header (stream, decoder, &image);
	if (opj_decode (decoder, stream, image) == OPJ_FALSE) {
		opj_destroy_codec (decoder);
		opj_stream_destroy (stream);
		if (format == OPJ_CODEC_J2K) {
			boost::throw_exception (DCPReadError (String::compose ("could not decode JPEG2000 codestream of %1 bytes.", size)));
		} else {
			boost::throw_exception (DCPReadError (String::compose ("could not decode JP2 file of %1 bytes.", size)));
		}
	}

	opj_destroy_codec (decoder);
	opj_stream_destroy (stream);

	image->x1 = rint (float(image->x1) / pow (2.0f, reduce));
	image->y1 = rint (float(image->y1) / pow (2.0f, reduce));
	return shared_ptr<OpenJPEGImage> (new OpenJPEGImage (image));
}

class WriteBuffer
{
public:
/* XXX: is there a better strategy for this? */
#define MAX_J2K_SIZE (1024 * 1024 * 2)
	WriteBuffer ()
		: _data (shared_array<uint8_t> (new uint8_t[MAX_J2K_SIZE]), MAX_J2K_SIZE)
		, _offset (0)
	{
		_data.set_size (0);
	}

	OPJ_SIZE_T write (void* buffer, OPJ_SIZE_T nb_bytes)
	{
		DCP_ASSERT ((_offset + nb_bytes) < MAX_J2K_SIZE);
		memcpy (_data.data().get() + _offset, buffer, nb_bytes);
		_offset += nb_bytes;
		if (_offset > OPJ_SIZE_T (_data.size())) {
			_data.set_size (_offset);
		}
		return nb_bytes;
	}

	OPJ_BOOL seek (OPJ_SIZE_T nb_bytes)
	{
		_offset = nb_bytes;
		return OPJ_TRUE;
	}

	Data data () const
	{
		return _data;
	}

private:
	Data _data;
	OPJ_SIZE_T _offset;
};

static OPJ_SIZE_T
write_function (void* buffer, OPJ_SIZE_T nb_bytes, void* data)
{
	return reinterpret_cast<WriteBuffer*>(data)->write (buffer, nb_bytes);
}

static void
write_free_function (void* data)
{
	delete reinterpret_cast<WriteBuffer*>(data);
}

static OPJ_BOOL
seek_function (OPJ_OFF_T nb_bytes, void* data)
{
	return reinterpret_cast<WriteBuffer*>(data)->seek (nb_bytes);
}

static void
error_callback (char const * msg, void *)
{
	throw MiscError (msg);
}

Data
dcp::compress_j2k (shared_ptr<const OpenJPEGImage> xyz, int bandwidth, int frames_per_second, bool threed, bool fourk)
{
	/* get a J2K compressor handle */
	opj_codec_t* encoder = opj_create_compress (OPJ_CODEC_J2K);
	if (encoder == 0) {
		throw MiscError ("could not create JPEG2000 encoder");
	}

	opj_set_error_handler (encoder, error_callback, 0);

	/* Set encoding parameters to default values */
	opj_cparameters_t parameters;
	opj_set_default_encoder_parameters (&parameters);
	parameters.rsiz = fourk ? OPJ_PROFILE_CINEMA_4K : OPJ_PROFILE_CINEMA_2K;
	parameters.cp_comment = strdup ("libdcp");

	/* set max image */
	parameters.max_cs_size = (bandwidth / 8) / frames_per_second;
	if (threed) {
		/* In 3D we have only half the normal bandwidth per eye */
		parameters.max_cs_size /= 2;
	}
	parameters.max_comp_size = parameters.max_cs_size / 1.25;
	parameters.tcp_numlayers = 1;
	parameters.tcp_mct = 1;

	/* Setup the encoder parameters using the current image and user parameters */
	opj_setup_encoder (encoder, &parameters, xyz->opj_image());

	opj_stream_t* stream = opj_stream_default_create (OPJ_FALSE);
	if (!stream) {
		throw MiscError ("could not create JPEG2000 stream");
	}

	opj_stream_set_write_function (stream, write_function);
	opj_stream_set_seek_function (stream, seek_function);
	WriteBuffer* buffer = new WriteBuffer ();
	opj_stream_set_user_data (stream, buffer, write_free_function);

	if (!opj_start_compress (encoder, xyz->opj_image(), stream)) {
		throw MiscError ("could not start JPEG2000 encoding");
	}

	if (!opj_encode (encoder, stream)) {
		opj_destroy_codec (encoder);
		opj_stream_destroy (stream);
		throw MiscError ("JPEG2000 encoding failed");
	}

	if (!opj_end_compress (encoder, stream)) {
		opj_destroy_codec (encoder);
		opj_stream_destroy (stream);
		throw MiscError ("could not end JPEG2000 encoding");
	}

	Data enc (buffer->data ());

	free (parameters.cp_comment);
	opj_destroy_codec (encoder);
	opj_stream_destroy (stream);

	return enc;
}
