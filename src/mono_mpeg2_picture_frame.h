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


#ifndef LIBDCP_MONO_MPEG2_PICTURE_FRAME_H
#define LIBDCP_MONO_MPEG2_PICTURE_FRAME_H


#include "asset_reader.h"
#include "data.h"


namespace dcp {


class MonoMPEG2PictureFrame : public Data
{
public:
	MonoMPEG2PictureFrame(uint8_t const * data, int size);

	MonoMPEG2PictureFrame(MonoMPEG2PictureFrame const&) = delete;
	MonoMPEG2PictureFrame& operator=(MonoMPEG2PictureFrame const&) = delete;

	/* XXX: couldn't we just return the frame buffer */

	/** @return Pointer to MPEG2 data */
	uint8_t const * data() const override;

	/** @return Pointer to MPEG2 data */
	uint8_t* data () override;

	/** @return Size of MPEG2 data in bytes */
	int size() const override;

private:
	/* XXX: this is a bit of a shame, but I tried friend MonoMPEG2PictureAssetReader and it's
	   rejected by some (seemingly older) GCCs.
	*/
	friend class AssetReader<ASDCP::MPEG2::MXFReader, MonoMPEG2PictureFrame>;

	MonoMPEG2PictureFrame(ASDCP::MPEG2::MXFReader* reader, int n, std::shared_ptr<DecryptionContext>, bool check_hmac);

	/* XXX why is this a shared_ptr? */
	std::shared_ptr<ASDCP::MPEG2::FrameBuffer> _buffer;
};


}


#endif
