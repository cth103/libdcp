/*
    Copyright (C) 2014-2021 Carl Hetherington <cth@carlh.net>

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


#include "common.h"
#include "dcp.h"


using std::shared_ptr;
using std::vector;


vector<dcp::VerificationNote>
dcp::filter_notes(vector<dcp::VerificationNote> const& notes, bool ignore_missing_assets, bool ignore_bv21_smpte)
{
	vector<dcp::VerificationNote> filtered;
	std::copy_if(notes.begin(), notes.end(), std::back_inserter(filtered), [ignore_missing_assets, ignore_bv21_smpte](dcp::VerificationNote const& i) {
		bool const missing = (
			i.code() == dcp::VerificationNote::Code::MISSING_ASSET ||
			i.code() == dcp::VerificationNote::Code::EXTERNAL_ASSET ||
			i.code() == dcp::VerificationNote::Code::MISSING_FONT
			);

		if (ignore_missing_assets && missing) {
			return false;
		}

		if (ignore_bv21_smpte && i.code() == dcp::VerificationNote::Code::INVALID_STANDARD) {
			return false;
		}

		return true;
	});

	return filtered;
}

