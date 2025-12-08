/*
    Copyright (C) 2025 Carl Hetherington <cth@carlh.net>

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


#ifndef LIBDCP_CPL_SUMMARY_H
#define LIBDCP_CPL_SUMMARY_H


#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <string>


namespace dcp {


class CPLSummary
{
public:
	CPLSummary(
		boost::filesystem::path dcp_directory_,
		std::string cpl_id_,
		boost::optional<std::string> annotation_text_,
		boost::filesystem::path cpl_file_,
		bool encrypted_,
		time_t last_write_time_
		)
		: dcp_directory(dcp_directory_)
		, cpl_id(cpl_id_)
		, cpl_annotation_text(annotation_text_)
		, cpl_file(cpl_file_)
		, encrypted(encrypted_)
		, last_write_time(last_write_time_)
	{}

	boost::filesystem::path dcp_directory;
	std::string cpl_id;
	boost::optional<std::string> cpl_annotation_text;
	boost::filesystem::path cpl_file;
	/** true if this CPL has any encrypted assets */
	bool encrypted;
	time_t last_write_time;
};


}


#endif

