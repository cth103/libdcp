/*
    Copyright (C) 2012-2016 Carl Hetherington <cth@carlh.net>

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

#include "mono_picture_asset.h"
#include "mono_picture_asset_writer.h"
#include "mono_picture_asset_reader.h"
#include "AS_DCP.h"
#include "KM_fileio.h"
#include "exceptions.h"
#include "dcp_assert.h"
#include "mono_picture_frame.h"
#include "compose.hpp"

using std::string;
using std::vector;
using std::list;
using std::pair;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using namespace dcp;

MonoPictureAsset::MonoPictureAsset (boost::filesystem::path file)
	: PictureAsset (file)
{
	ASDCP::JP2K::MXFReader reader;
	Kumu::Result_t r = reader.OpenRead (file.string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", file.string(), r));
	}

	ASDCP::JP2K::PictureDescriptor desc;
	if (ASDCP_FAILURE (reader.FillPictureDescriptor (desc))) {
		boost::throw_exception (DCPReadError ("could not read video MXF information"));
	}

	read_picture_descriptor (desc);

	ASDCP::WriterInfo info;
	if (ASDCP_FAILURE (reader.FillWriterInfo (info))) {
		boost::throw_exception (DCPReadError ("could not read video MXF information"));
	}

	_id = read_writer_info (info);
}

MonoPictureAsset::MonoPictureAsset (Fraction edit_rate)
	: PictureAsset (edit_rate)
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
	if (!dynamic_pointer_cast<const MonoPictureAsset> (other)) {
		return false;
	}

	ASDCP::JP2K::MXFReader reader_A;
	Kumu::Result_t r = reader_A.OpenRead (_file.string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", _file.string(), r));
	}

	ASDCP::JP2K::MXFReader reader_B;
	r = reader_B.OpenRead (other->file().string().c_str());
	if (ASDCP_FAILURE (r)) {
		boost::throw_exception (MXFFileError ("could not open MXF file for reading", other->file().string(), r));
	}

	ASDCP::JP2K::PictureDescriptor desc_A;
	if (ASDCP_FAILURE (reader_A.FillPictureDescriptor (desc_A))) {
		boost::throw_exception (DCPReadError ("could not read video MXF information"));
	}
	ASDCP::JP2K::PictureDescriptor desc_B;
	if (ASDCP_FAILURE (reader_B.FillPictureDescriptor (desc_B))) {
		boost::throw_exception (DCPReadError ("could not read video MXF information"));
	}

	if (!descriptor_equals (desc_A, desc_B, note)) {
		return false;
	}

	shared_ptr<const MonoPictureAsset> other_picture = dynamic_pointer_cast<const MonoPictureAsset> (other);
	DCP_ASSERT (other_picture);

	bool result = true;

#ifdef LIBDCP_OPENMP
#pragma omp parallel for
#endif

	shared_ptr<MonoPictureAssetReader> reader = start_read ();
	shared_ptr<MonoPictureAssetReader> other_reader = other_picture->start_read ();

	for (int i = 0; i < _intrinsic_duration; ++i) {
		if (i >= other_picture->intrinsic_duration()) {
			result = false;
		}

		if (result || opt.keep_going) {

			shared_ptr<const MonoPictureFrame> frame_A = reader->get_frame (i);
			shared_ptr<const MonoPictureFrame> frame_B = other_reader->get_frame (i);

			list<pair<NoteType, string> > notes;

			if (!frame_buffer_equals (
				    i, opt, bind (&storing_note_handler, boost::ref(notes), _1, _2),
				    frame_A->j2k_data(), frame_A->j2k_size(),
				    frame_B->j2k_data(), frame_B->j2k_size()
				    )) {
				result = false;
			}

#ifdef LIBDCP_OPENMP
#pragma omp critical
#endif
			{
				note (DCP_PROGRESS, String::compose ("Compared video frame %1 of %2", i, _intrinsic_duration));
				for (list<pair<NoteType, string> >::const_iterator i = notes.begin(); i != notes.end(); ++i) {
					note (i->first, i->second);
				}
			}
		}
	}

	return result;
}

shared_ptr<PictureAssetWriter>
MonoPictureAsset::start_write (boost::filesystem::path file, Standard standard, bool overwrite)
{
	/* XXX: can't we use shared_ptr here? */
	return shared_ptr<MonoPictureAssetWriter> (new MonoPictureAssetWriter (this, file, standard, overwrite));
}

shared_ptr<MonoPictureAssetReader>
MonoPictureAsset::start_read () const
{
	return shared_ptr<MonoPictureAssetReader> (new MonoPictureAssetReader (this));
}

string
MonoPictureAsset::cpl_node_name () const
{
	return "MainPicture";
}
