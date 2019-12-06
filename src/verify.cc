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
#include "raw_convert.h"
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
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

static
bool
good_urn_uuid (string id)
{
	boost::regex ex("urn:uuid:[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}");
	return boost::regex_match (id, ex);
}

static
bool
good_date (string date)
{
	boost::regex ex("\\d{4}-(\\d{2})-(\\d{2})T(\\d{2}):(\\d{2}):(\\d{2})[+-](\\d{2}):(\\d{2})");
	boost::match_results<string::const_iterator> res;
	if (!regex_match (date, res, ex, boost::match_default)) {
		return false;
	}
	int const month = dcp::raw_convert<int>(res[1].str());
	if (month < 1 || month > 12) {
		return false;
	}
	int const day = dcp::raw_convert<int>(res[2].str());
	if (day < 1 || day > 31) {
		return false;
	}
	if (dcp::raw_convert<int>(res[3].str()) > 23) {
		return false;
	}
	if (dcp::raw_convert<int>(res[4].str()) > 59) {
		return false;
	}
	if (dcp::raw_convert<int>(res[5].str()) > 59) {
		return false;
	}
	if (dcp::raw_convert<int>(res[6].str()) > 23) {
		return false;
	}
	if (dcp::raw_convert<int>(res[7].str()) > 59) {
		return false;
	}
	return true;
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
		try {
			dcp->read (&notes);
		} catch (DCPReadError& e) {
			notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::Code::GENERAL_READ, string(e.what())));
		} catch (XMLError& e) {
			notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::Code::GENERAL_READ, string(e.what())));
		}

		BOOST_FOREACH (shared_ptr<CPL> cpl, dcp->cpls()) {
			stage ("Checking CPL", cpl->file());

			cxml::Document cpl_doc ("CompositionPlaylist");
			cpl_doc.read_file (cpl->file().get());
			if (!good_urn_uuid(cpl_doc.string_child("Id"))) {
				notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::Code::BAD_URN_UUID, string("CPL <Id> is malformed")));
			}
			if (!good_date(cpl_doc.string_child("IssueDate"))) {
				notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::Code::BAD_DATE, string("CPL <IssueDate> is malformed")));
			}
			if (cpl->standard() && cpl->standard().get() == SMPTE && !good_urn_uuid(cpl_doc.node_child("ContentVersion")->string_child("Id"))) {
				notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::Code::BAD_URN_UUID, string("<ContentVersion> <Id> is malformed.")));
			}

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
					if (reel->main_picture()->asset_ref().resolved()) {
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
				}
				if (reel->main_sound() && reel->main_sound()->asset_ref().resolved()) {
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

string
dcp::note_to_string (dcp::VerificationNote note)
{
	switch (note.code()) {
	case dcp::VerificationNote::GENERAL_READ:
		return *note.note();
	case dcp::VerificationNote::CPL_HASH_INCORRECT:
		return "The hash of the CPL in the PKL does not agree with the CPL file";
	case dcp::VerificationNote::INVALID_PICTURE_FRAME_RATE:
		return "The picture in a reel has an invalid frame rate";
	case dcp::VerificationNote::PICTURE_HASH_INCORRECT:
		return dcp::String::compose("The hash of the picture asset %1 does not agree with the PKL file", note.file()->filename());
	case dcp::VerificationNote::PKL_CPL_PICTURE_HASHES_DISAGREE:
		return "The PKL and CPL hashes disagree for a picture asset.";
	case dcp::VerificationNote::SOUND_HASH_INCORRECT:
		return dcp::String::compose("The hash of the sound asset %1 does not agree with the PKL file", note.file()->filename());
	case dcp::VerificationNote::PKL_CPL_SOUND_HASHES_DISAGREE:
		return "The PKL and CPL hashes disagree for a sound asset.";
	case dcp::VerificationNote::EMPTY_ASSET_PATH:
		return "The asset map contains an empty asset path.";
	case dcp::VerificationNote::MISSING_ASSET:
		return "The file for an asset in the asset map cannot be found.";
	case dcp::VerificationNote::MISMATCHED_STANDARD:
		return "The DCP contains both SMPTE and Interop parts.";
	case dcp::VerificationNote::BAD_URN_UUID:
		return "There is a badly-formed urn:uuid.";
	case dcp::VerificationNote::BAD_DATE:
		return "There is a badly-formed date.";
	}

	return "";
}

