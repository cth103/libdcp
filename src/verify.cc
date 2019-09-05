/*
    Copyright (C) 2018-2019 Carl Hetherington <cth@carlh.net>

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
#include "compose.hpp"
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

enum Result {
	RESULT_GOOD,
	RESULT_CPL_PKL_DIFFER,
	RESULT_BAD
};

static Result
verify_asset (shared_ptr<DCP> dcp, shared_ptr<ReelMXF> reel_mxf, function<void (float)> progress)
{
	string const actual_hash = reel_mxf->asset_ref()->hash(progress);

	list<shared_ptr<PKL> > pkls = dcp->pkls();
	/* We've read this DCP in so it must have at least one PKL */
	DCP_ASSERT (!pkls.empty());

	shared_ptr<Asset> asset = reel_mxf->asset_ref().asset();

	optional<string> pkl_hash;
	BOOST_FOREACH (shared_ptr<PKL> i, pkls) {
		pkl_hash = i->hash (reel_mxf->asset_ref()->id());
		if (pkl_hash) {
			break;
		}
	}

	DCP_ASSERT (pkl_hash);

	optional<string> cpl_hash = reel_mxf->hash();
	if (cpl_hash && *cpl_hash != *pkl_hash) {
		return RESULT_CPL_PKL_DIFFER;
	}

	if (actual_hash != *pkl_hash) {
		return RESULT_BAD;
	}

	return RESULT_GOOD;
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
			notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::GENERAL_READ, string(e.what())));
		} catch (XMLError& e) {
			notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::GENERAL_READ, string(e.what())));
		}

		BOOST_FOREACH (shared_ptr<CPL> cpl, dcp->cpls()) {
			stage ("Checking CPL", cpl->file());

			/* Check that the CPL's hash corresponds to the PKL */
			BOOST_FOREACH (shared_ptr<PKL> i, dcp->pkls()) {
				optional<string> h = i->hash(cpl->id());
				if (h && make_digest(Data(*cpl->file())) != *h) {
					notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::CPL_HASH_INCORRECT));
				}
			}

			BOOST_FOREACH (shared_ptr<Reel> reel, cpl->reels()) {
				stage ("Checking reel", optional<boost::filesystem::path>());
				if (reel->main_picture()) {
					/* Check reel stuff */
					Fraction const frame_rate = reel->main_picture()->frame_rate();
					if (frame_rate.denominator != 1 ||
					    (frame_rate.numerator != 24 &&
					     frame_rate.numerator != 25 &&
					     frame_rate.numerator != 30 &&
					     frame_rate.numerator != 48 &&
					     frame_rate.numerator != 50 &&
					     frame_rate.numerator != 60 &&
					     frame_rate.numerator != 96)) {
						notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::INVALID_PICTURE_FRAME_RATE));
					}
					/* Check asset */
					stage ("Checking picture asset hash", reel->main_picture()->asset()->file());
					Result const r = verify_asset (dcp, reel->main_picture(), progress);
					switch (r) {
					case RESULT_BAD:
						notes.push_back (
							VerificationNote(
								VerificationNote::VERIFY_ERROR, VerificationNote::PICTURE_HASH_INCORRECT, *reel->main_picture()->asset()->file()
								)
							);
						break;
					case RESULT_CPL_PKL_DIFFER:
						notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::PKL_CPL_PICTURE_HASHES_DISAGREE));
						break;
					default:
						break;
					}
				}
				if (reel->main_sound()) {
					stage ("Checking sound asset hash", reel->main_sound()->asset()->file());
					Result const r = verify_asset (dcp, reel->main_sound(), progress);
					switch (r) {
					case RESULT_BAD:
						notes.push_back (
							VerificationNote(
								VerificationNote::VERIFY_ERROR, VerificationNote::SOUND_HASH_INCORRECT, *reel->main_sound()->asset()->file()
								)
							);
						break;
					case RESULT_CPL_PKL_DIFFER:
						notes.push_back (VerificationNote (VerificationNote::VERIFY_ERROR, VerificationNote::PKL_CPL_SOUND_HASHES_DISAGREE));
						break;
					default:
						break;
					}
				}
			}
		}
	}

	return notes;
}
