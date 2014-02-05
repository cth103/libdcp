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

#ifndef LIBDCP_MXF_H
#define LIBDCP_MXF_H

#include "content.h"
#include "key.h"
#include "metadata.h"
#include <boost/signals2.hpp>

namespace ASDCP {
	class AESEncContext;
	class AESDecContext;
}

namespace dcp
{

class MXFMetadata;	

/** @class MXF
 *  @brief Parent class for classes which represent MXF files.
 */
class MXF : public Content
{
public:
	MXF (Fraction edit_rate);
	MXF (boost::filesystem::path file);
	~MXF ();

	virtual std::string key_type () const = 0;
	
	bool equals (
		boost::shared_ptr<const Content> other,
		EqualityOptions opt,
		boost::function<void (NoteType, std::string)> note
		) const;

	/** Fill in a ADSCP::WriteInfo struct.
	 *  @param w struct to fill in.
	 *  @param standard INTEROP or SMPTE.
	 */
	void fill_writer_info (ASDCP::WriterInfo* w, Standard standard);

	void set_progress (boost::signals2::signal<void (float)>* progress) {
		_progress = progress;
	}

	bool encrypted () const {
		return !_key_id.empty ();
	}

	void set_key_id (std::string i) {
		_key_id = i;
	}

	std::string key_id () const {
		return _key_id;
	}
	
	void set_key (Key);

	boost::optional<Key> key () const {
		return _key;
	}

	ASDCP::AESEncContext* encryption_context () const {
		return _encryption_context;
	}

	void set_metadata (MXFMetadata m) {
		_metadata = m;
	}

	MXFMetadata metadata () const {
		return _metadata;
	}

protected:
	std::string pkl_type () const {
		return "application/x-smpte-mxf";
	}
	
	void read_writer_info (ASDCP::WriterInfo const &);
	
	/** Signal to emit to report progress, or 0 */
	boost::signals2::signal<void (float)>* _progress;
	ASDCP::AESEncContext* _encryption_context;
	ASDCP::AESDecContext* _decryption_context;
	std::string _key_id;
	boost::optional<Key> _key;
	MXFMetadata _metadata;
};

}

#endif
