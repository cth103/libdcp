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


/** @file  src/openjpeg_image.cc
 *  @brief OpenJPEGImage class
 */


#include "openjpeg_image.h"
#include "dcp_assert.h"
#include <openjpeg.h>
#include <stdexcept>


using namespace dcp;


#ifdef LIBDCP_OPENJPEG1
#define OPJ_CLRSPC_SRGB CLRSPC_SRGB
#endif


OpenJPEGImage::OpenJPEGImage (opj_image_t* image)
	: _opj_image (image)
{
	DCP_ASSERT (_opj_image->numcomps == 3);
}


#ifdef LIBDCP_OPENJPEG1
typedef int32_t OPJ_INT32;
typedef uint8_t OPJ_BYTE;
#endif


OpenJPEGImage::OpenJPEGImage (OpenJPEGImage const & other)
{
	_opj_image = reinterpret_cast<opj_image_t*>(malloc(sizeof(opj_image_t)));
	DCP_ASSERT (_opj_image);
	memcpy (_opj_image, other._opj_image, sizeof (opj_image_t));

	int const data_size = _opj_image->x1 * _opj_image->y1 * 4;

	_opj_image->comps = reinterpret_cast<opj_image_comp_t*> (malloc (_opj_image->numcomps * sizeof (opj_image_comp_t)));
	DCP_ASSERT (_opj_image->comps);
	memcpy (_opj_image->comps, other._opj_image->comps, _opj_image->numcomps * sizeof (opj_image_comp_t));
	for (unsigned int i = 0; i < _opj_image->numcomps; ++i) {
		_opj_image->comps[i].data = reinterpret_cast<OPJ_INT32*> (malloc (data_size));
		DCP_ASSERT (_opj_image->comps[i].data);
		memcpy (_opj_image->comps[i].data, other._opj_image->comps[i].data, data_size);
	}

	_opj_image->icc_profile_buf = reinterpret_cast<OPJ_BYTE*> (malloc (_opj_image->icc_profile_len));
	DCP_ASSERT (_opj_image->icc_profile_buf);
	memcpy (_opj_image->icc_profile_buf, other._opj_image->icc_profile_buf, _opj_image->icc_profile_len);
}


OpenJPEGImage::OpenJPEGImage (Size size)
{
	create (size);
}


OpenJPEGImage::OpenJPEGImage (uint8_t const * data_16, dcp::Size size, int stride)
{
	create (size);

	int jn = 0;
	for (int y = 0; y < size.height; ++y) {
		uint16_t const * p = reinterpret_cast<uint16_t const *> (data_16 + y * stride);
		for (int x = 0; x < size.width; ++x) {
			/* Truncate 16-bit to 12-bit */
			_opj_image->comps[0].data[jn] = *p++ >> 4;
			_opj_image->comps[1].data[jn] = *p++ >> 4;
			_opj_image->comps[2].data[jn] = *p++ >> 4;
			++jn;
		}
	}
}


void
OpenJPEGImage::create (Size size)
{
	opj_image_cmptparm_t cmptparm[3];

	for (int i = 0; i < 3; ++i) {
		cmptparm[i].dx = 1;
		cmptparm[i].dy = 1;
		cmptparm[i].w = size.width;
		cmptparm[i].h = size.height;
		cmptparm[i].x0 = 0;
		cmptparm[i].y0 = 0;
		cmptparm[i].prec = 12;
		cmptparm[i].bpp = 12;
		cmptparm[i].sgnd = 0;
	}

	/* XXX: is this _SRGB right? */
	_opj_image = opj_image_create (3, &cmptparm[0], OPJ_CLRSPC_SRGB);
	if (_opj_image == nullptr) {
		throw std::runtime_error ("could not create libopenjpeg image");
	}

	_opj_image->x0 = 0;
	_opj_image->y0 = 0;
	_opj_image->x1 = size.width;
	_opj_image->y1 = size.height;
}


OpenJPEGImage::~OpenJPEGImage ()
{
	opj_image_destroy (_opj_image);
}


int *
OpenJPEGImage::data (int c) const
{
	DCP_ASSERT (c >= 0 && c < 3);
	return _opj_image->comps[c].data;
}


dcp::Size
OpenJPEGImage::size () const
{
	/* XXX: this may not be right; x0 and y0 can presumably be non-zero */
	return dcp::Size (_opj_image->x1, _opj_image->y1);
}


int
OpenJPEGImage::precision (int component) const
{
	return _opj_image->comps[component].prec;
}


int
OpenJPEGImage::factor (int component) const
{
	return _opj_image->comps[component].factor;
}


bool
OpenJPEGImage::srgb () const
{
	return _opj_image->color_space == OPJ_CLRSPC_SRGB;
}
