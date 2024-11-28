/*
    Copyright (C) 2024 Carl Hetherington <cth@carlh.net>

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


/** @file  src/verify_internal.h
 *  @brief Verification-related things exposed for testing.
 *
 *  These things are not intended for use by library users, just
 *  by the libdcp tests.
 */


#ifndef LIBDCP_VERIFY_INTERNAL_H
#define LIBDCP_VERIFY_INTERNAL_H


#include "cpl.h"
#include "verify.h"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <memory>
#include <vector>


namespace dcp {

class TextAsset;

struct LinesCharactersResult
{
	bool warning_length_exceeded = false;
	bool error_length_exceeded = false;
	bool line_count_exceeded = false;
};


extern void verify_text_lines_and_characters(
	std::shared_ptr<const dcp::TextAsset> asset,
	int warning_length,
	int error_length,
	dcp::LinesCharactersResult* result
	);


class Context
{
public:
	Context(
		std::vector<VerificationNote>& notes_,
		boost::filesystem::path xsd_dtd_directory_,
		std::function<void (std::string, boost::optional<boost::filesystem::path>)> stage_,
		std::function<void (float)> progress_,
		VerificationOptions options_
	       )
		: notes(notes_)
		, xsd_dtd_directory(xsd_dtd_directory_)
		, stage(stage_)
		, progress(progress_)
		, options(options_)
	{}

	Context(Context const&) = delete;
	Context& operator=(Context const&) = delete;

	template<typename... Args>
	void ok(dcp::VerificationNote::Code code, Args... args)
	{
		add_note({dcp::VerificationNote::Type::OK, code, std::forward<Args>(args)...});
	}

	template<typename... Args>
	void warning(dcp::VerificationNote::Code code, Args... args)
	{
		add_note({dcp::VerificationNote::Type::WARNING, code, std::forward<Args>(args)...});
	}

	template<typename... Args>
	void bv21_error(dcp::VerificationNote::Code code, Args... args)
	{
		add_note({dcp::VerificationNote::Type::BV21_ERROR, code, std::forward<Args>(args)...});
	}

	template<typename... Args>
	void error(dcp::VerificationNote::Code code, Args... args)
	{
		add_note({dcp::VerificationNote::Type::ERROR, code, std::forward<Args>(args)...});
	}

	void add_note(dcp::VerificationNote note)
	{
		if (cpl) {
			note.set_cpl_id(cpl->id());
		}
		notes.push_back(std::move(note));
	}

	void add_note_if_not_existing(dcp::VerificationNote note)
	{
		if (find(notes.begin(), notes.end(), note) == notes.end()) {
			add_note(note);
		}
	}

	std::vector<VerificationNote>& notes;
	std::shared_ptr<const DCP> dcp;
	std::shared_ptr<const CPL> cpl;
	boost::filesystem::path xsd_dtd_directory;
	std::function<void (std::string, boost::optional<boost::filesystem::path>)> stage;
	std::function<void (float)> progress;
	VerificationOptions options;

	boost::optional<std::string> subtitle_language;
	boost::optional<int> audio_channels;
};


extern void verify_extension_metadata(dcp::Context& context);

}


#endif
