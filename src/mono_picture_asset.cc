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


/** @file  src/mono_picture_asset.cc
 *  @brief MonoPictureAsset class
 */


#include "mono_picture_asset.h"
#include "mono_picture_asset_writer.h"
#include "mono_picture_asset_reader.h"
#include "exceptions.h"
#include "dcp_assert.h"
#include "mono_picture_frame.h"
#include "compose.hpp"
#include <asdcp/AS_DCP.h>
#include <asdcp/KM_fileio.h>


using std::string;
using std::vector;
using std::list;
using std::pair;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using std::make_shared;
#if BOOST_VERSION >= 106100
using namespace boost::placeholders;
#endif
using namespace dcp;


MonoPictureAsset::MonoPictureAsset (boost::filesystem::path file)
	: PictureAsset (file)
{
	ASDCP::JP2K::MXFReader reader;
	auto r = reader.OpenRead (file.string().c_str());
	if (ASDCP_FAILURE(r)) {
		boost::throw_exception (MXFFileError("could not open MXF file for reading", file.string(), r));
	}

	ASDCP::JP2K::PictureDescriptor desc;
	if (ASDCP_FAILURE (reader.FillPictureDescriptor(desc))) {
		boost::throw_exception (ReadError("could not read video MXF information"));
	}

	read_picture_descriptor (desc);

	ASDCP::WriterInfo info;
	if (ASDCP_FAILURE (reader.FillWriterInfo (info))) {
		boost::throw_exception (ReadError("could not read video MXF information"));
	}

	_id = read_writer_info (info);
}


MonoPictureAsset::MonoPictureAsset (Fraction edit_rate, Standard standard)
	: PictureAsset (edit_rate, standard)
{

}


static void
storing_note_handler (list<pair<NoteType, string> >& notes, NoteType t, string s)
{
	notes.push_back (make_pair (t, s));
}


bool
MonoPictureAsset::equals (shared_ptr<const Asset> other, EqualityOptions opt, NoteHandler note) const
{
	if (!dynamic_pointer_cast<const MonoPictureAsset>(other)) {
		return false;
	}

	ASDCP::JP2K::MXFReader reader_A;
	DCP_ASSERT (_file);
	auto r = reader_A.OpenRead (_file->string().c_str());
	if (ASDCP_FAILURE(r)) {
		boost::throw_exception (MXFFileError("could not open MXF file for reading", _file->string(), r));
	}

	ASDCP::JP2K::MXFReader reader_B;
	DCP_ASSERT (other->file ());
	r = reader_B.OpenRead (other->file()->string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", other->file()->string(), r));
	}

	ASDCP::JP2K::PictureDescriptor desc_A;
	if (ASDCP_FAILURE (reader_A.FillPictureDescriptor (desc_A))) {
		boost::throw_exception (ReadError ("could not read video MXF information"));
	}
	ASDCP::JP2K::PictureDescriptor desc_B;
	if (ASDCP_FAILURE (reader_B.FillPictureDescriptor (desc_B))) {
		boost::throw_exception (ReadError ("could not read video MXF information"));
	}

	if (!descriptor_equals (desc_A, desc_B, note)) {
		return false;
	}

	auto other_picture = dynamic_pointer_cast<const MonoPictureAsset> (other);
	DCP_ASSERT (other_picture);

	bool result = true;

	auto reader = start_read ();
	auto other_reader = other_picture->start_read ();

#ifdef LIBDCP_OPENMP
#pragma omp parallel for
#endif

	for (int i = 0; i < _intrinsic_duration; ++i) {
		if (i >= other_picture->intrinsic_duration()) {
			result = false;
		}

		if (result || opt.keep_going) {

			auto frame_A = reader->get_frame (i);
			auto frame_B = other_reader->get_frame (i);

			list<pair<NoteType, string> > notes;

			if (!frame_buffer_equals (
				    i, opt, bind (&storing_note_handler, boost::ref(notes), _1, _2),
				    frame_A->data(), frame_A->size(),
				    frame_B->data(), frame_B->size()
				    )) {
				result = false;
			}

#ifdef LIBDCP_OPENMP
#pragma omp critical
#endif
			{
				note (NoteType::PROGRESS, String::compose("Compared video frame %1 of %2", i, _intrinsic_duration));
				for (auto const& i: notes) {
					note (i.first, i.second);
				}
			}
		}
	}

	return result;
}


shared_ptr<PictureAssetWriter>
MonoPictureAsset::start_write (boost::filesystem::path file, bool overwrite)
{
	/* Can't use make_shared here as the MonoPictureAssetWriter constructor is private */
	return shared_ptr<MonoPictureAssetWriter>(new MonoPictureAssetWriter(this, file, overwrite));
}

shared_ptr<MonoPictureAssetReader>
MonoPictureAsset::start_read () const
{
	/* Can't use make_shared here as the MonoPictureAssetReader constructor is private */
	return shared_ptr<MonoPictureAssetReader>(new MonoPictureAssetReader(this, key(), standard()));

}

string
MonoPictureAsset::cpl_node_name () const
{
	return "MainPicture";
}
