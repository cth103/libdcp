/*
    Copyright (C) 2018-2021 Carl Hetherington <cth@carlh.net>

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


#include "compose.hpp"
#include "cpl.h"
#include "dcp.h"
#include "file.h"
#include "reel.h"
#include "reel_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_text_asset.h"
#include "verify.h"
#include "verify_report.h"


using std::shared_ptr;
using std::string;
using std::vector;
using boost::optional;
using namespace dcp;


void write_line(File& file, string format)
{
	file.puts(string(format + "\n").c_str());
}


template <typename... Args>
void write_line(File& file, string format, Args... args)
{
	file.puts(String::compose(format + "\n", std::forward<Args>(args)...).c_str());
}


static void
verify_report(dcp::VerificationResult const& result, Formatter& formatter)
{
	if (result.dcps.size() > 1) {
		formatter.subheading("DCPs");
	} else {
		formatter.subheading("DCP");
	}

	auto reel_asset_details = [&formatter](shared_ptr<dcp::ReelAsset> asset) {
		formatter.list_item(String::compose("UUID: %1", asset->id()));
		formatter.list_item(String::compose("Intrinsic duration: %1", asset->intrinsic_duration()));
		formatter.list_item(String::compose("Entry point: %1", asset->entry_point().get_value_or(0)));
		formatter.list_item(String::compose("Duration: %1", asset->duration().get_value_or(0)));
		if (asset->annotation_text()) {
			formatter.list_item(String::compose("Annotation text: %1", *asset->annotation_text()));
		}
	};

	auto write_notes = [&formatter](dcp::VerificationResult const& result, optional<string> cpl_id) {
		for (auto note: result.notes) {
			if (note.cpl_id() == cpl_id) {
				auto const note_as_string = dcp::note_to_string(note, formatter.process_string(), formatter.process_filename());
				switch (note.type()) {
				case dcp::VerificationNote::Type::OK:
					formatter.list_item(note_as_string, string("ok"));
					break;
				case dcp::VerificationNote::Type::WARNING:
					formatter.list_item(note_as_string, string("warning"));
					break;
				case dcp::VerificationNote::Type::ERROR:
					formatter.list_item(note_as_string, string("error"));
					break;
				case dcp::VerificationNote::Type::BV21_ERROR:
					formatter.list_item(note_as_string, string("bv21-error"));
					break;
				}
			}
		}
	};

	for (auto dcp: result.dcps) {
		auto ul = formatter.unordered_list();
		for (auto cpl: dcp->cpls()) {
			formatter.list_item(String::compose("CPL ID: %1", cpl->id()));
			int reel_index = 1;
			for (auto reel: cpl->reels()) {
				formatter.list_item(String::compose("Reel: %1", reel_index++));
				auto ul2 = formatter.unordered_list();
				if (auto pic = reel->main_picture()) {
					formatter.list_item("Main picture");
					auto ul3 = formatter.unordered_list();
					reel_asset_details(pic);
					formatter.list_item(String::compose("Frame rate: %1", pic->frame_rate().numerator));
					formatter.list_item(String::compose("Screen aspect ratio: %1x%2", pic->screen_aspect_ratio().numerator, pic->screen_aspect_ratio().denominator));
				}
				if (auto sound = reel->main_sound()) {
					formatter.list_item("Main sound");
					auto ul3 = formatter.unordered_list();
					reel_asset_details(sound);
				}
				if (auto sub = reel->main_subtitle()) {
					formatter.list_item("Main subtitle");
					auto ul3 = formatter.unordered_list();
					reel_asset_details(sub);
					if (sub->language()) {
						formatter.list_item(String::compose("Language: %1", *sub->language()));
					}
				}
			}
			write_notes(result, cpl->id());
		}
	}

	if (std::count_if(result.notes.begin(), result.notes.end(), [](VerificationNote const& note) { return !note.cpl_id(); }) > 0) {
		formatter.subheading("Report");
		write_notes(result, {});
	}
}


void
dcp::verify_report(vector<dcp::VerificationResult> const& results, Formatter& formatter)
{
	auto document = formatter.document();
	auto body = formatter.body();

	formatter.heading("DCP verification report");

	for (auto result: results) {
		::verify_report(result, formatter);
	}
}

