/*
    Copyright (C) 2022 Carl Hetherington <cth@carlh.net>

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


#include "dcp.h"
#include "decrypted_kdm.h"
#include "exceptions.h"
#include "filesystem.h"
#include "search.h"


using std::make_shared;
using std::shared_ptr;
using std::vector;
using namespace dcp;


/** Find all the CPLs in some directories and resolve any assets that are found */
vector<shared_ptr<dcp::CPL>>
dcp::find_and_resolve_cpls (vector<boost::filesystem::path> const& directories, bool tolerant)
{
	vector<shared_ptr<dcp::CPL>> cpls;

	/** We accept and ignore some warnings / errors but everything else is bad */
	vector<dcp::VerificationNote::Code> const ignore = {
		dcp::VerificationNote::Code::EMPTY_ASSET_PATH,
		dcp::VerificationNote::Code::EXTERNAL_ASSET,
		dcp::VerificationNote::Code::THREED_ASSET_MARKED_AS_TWOD,
	};

	vector<shared_ptr<dcp::DCP>> dcps;
	for (auto i: directories) {
		if (!filesystem::exists(i)) {
			/* Don't make a DCP object or it will try to create the parent directories
			 * of i if they do not exist (#2344).
			 */
			continue;
		}
		auto dcp = make_shared<dcp::DCP>(i);
		vector<dcp::VerificationNote> notes;
		dcp->read (&notes, true);
		if (!tolerant) {
			for (auto j: notes) {
				if (std::find(ignore.begin(), ignore.end(), j.code()) == ignore.end()) {
					boost::throw_exception(dcp::ReadError(dcp::note_to_string(j)));
				}
			}
		}
		dcps.push_back (dcp);
		for (auto i: dcp->cpls()) {
			cpls.push_back (i);
		}
	}

	for (auto i: dcps) {
		for (auto j: dcps) {
			if (i != j) {
				i->resolve_refs(j->assets(true));
			}
		}
	}

	return cpls;
}

