/*
    Copyright (C) 2016-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/asset_reader.h
 *  @brief AssetReader class
 */


#ifndef LIBDCP_ASSET_READER_H
#define LIBDCP_ASSET_READER_H


#include "asset.h"
#include "crypto_context.h"
#include "dcp_assert.h"
#include <asdcp/AS_DCP.h>
#include <memory>


namespace dcp {


class AtmosAsset;
class MonoPictureAsset;
class SoundAsset;
class StereoPictureAsset;


template <class R, class F>
class AssetReader
{
public:
	AssetReader (AssetReader const&) = delete;
	AssetReader& operator== (AssetReader const&) = delete;

	~AssetReader ()
	{
		delete _reader;
	}

	std::shared_ptr<const F> get_frame (int n) const
	{
		/* Can't use make_shared here as the constructor is private */
		return std::shared_ptr<const F> (new F(_reader, n, _crypto_context));
	}

	R* reader () const {
		return _reader;
	}

protected:
	R* _reader = nullptr;
	std::shared_ptr<DecryptionContext> _crypto_context;

private:
	friend class AtmosAsset;
	friend class MonoPictureAsset;
	friend class SoundAsset;
	friend class StereoPictureAsset;

	explicit AssetReader (Asset const * asset, boost::optional<Key> key, Standard standard)
		: _crypto_context (new DecryptionContext(key, standard))
	{
		_reader = new R ();
		DCP_ASSERT (asset->file());
		auto const r = _reader->OpenRead (asset->file()->string().c_str());
		if (ASDCP_FAILURE(r)) {
			delete _reader;
			boost::throw_exception (FileError("could not open MXF file for reading", asset->file().get(), r));
		}
	}
};


}


#endif
