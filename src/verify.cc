/*
    Copyright (C) 2018 Carl Hetherington <cth@carlh.net>

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

#include "verify.h"
#include "dcp.h"
#include "cpl.h"
#include "reel.h"
#include "reel_picture_asset.h"
#include "reel_sound_asset.h"
#include "exceptions.h"
#include <boost/foreach.hpp>
#include <list>
#include <vector>
#include <iostream>

using std::list;
using std::vector;
using std::string;
using std::cout;
using boost::shared_ptr;
using boost::optional;
using boost::function;

using namespace dcp;

static bool
verify_asset (shared_ptr<ReelAsset> asset, function<void (float)> progress)
{
	string actual_hash = asset->asset_ref()->hash(progress);
	optional<string> cpl_hash = asset->hash();
	DCP_ASSERT (cpl_hash);
	return actual_hash != *cpl_hash;
}

list<VerificationNote>
dcp::verify (vector<boost::filesystem::path> directories, function<void (string, optional<boost::filesystem::path>)> stage, function<void (float)> progress)
{
	list<VerificationNote> notes;

	list<shared_ptr<DCP> > dcps;
	BOOST_FOREACH (boost::filesystem::path i, directories) {
		dcps.push_back (shared_ptr<DCP> (new DCP (i)));
	}

	BOOST_FOREACH (shared_ptr<DCP> dcp, dcps) {
		stage ("Checking DCP", dcp->directory());
		DCP::ReadErrors errors;
		try {
			dcp->read (true, &errors);
		} catch (DCPReadError& e) {
			notes.push_back (VerificationNote (VerificationNote::VERIFY_ERROR, e.what ()));
		} catch (XMLError& e) {
			notes.push_back (VerificationNote (VerificationNote::VERIFY_ERROR, e.what ()));
		}

		BOOST_FOREACH (shared_ptr<CPL> cpl, dcp->cpls()) {
			stage ("Checking CPL", cpl->file());
			BOOST_FOREACH (shared_ptr<Reel> reel, cpl->reels()) {
				stage ("Checking reel", optional<boost::filesystem::path>());
				if (reel->main_picture()) {
					stage ("Checking picture asset hash", reel->main_picture()->asset()->file());
					if (verify_asset (reel->main_picture(), progress)) {
						notes.push_back (VerificationNote (VerificationNote::VERIFY_ERROR, "Picture asset hash is incorrect"));
					} else {
						cout << "pic ok.\n";
					}
				}
				if (reel->main_sound()) {
					stage ("Checking sound asset hash", reel->main_sound()->asset()->file());
					if (verify_asset (reel->main_sound(), progress)) {
						notes.push_back (VerificationNote (VerificationNote::VERIFY_ERROR, "Sound asset hash is incorrect"));
					} else {
						cout << "sounds ok.\n";
					}
				}
			}
		}
	}

	return notes;
}
