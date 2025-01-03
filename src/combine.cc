/*
    Copyright (C) 2020-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/combine.cc
 *  @brief Method to combine DCPs
 */


#include "asset.h"
#include "combine.h"
#include "cpl.h"
#include "dcp.h"
#include "dcp_assert.h"
#include "exceptions.h"
#include "filesystem.h"
#include "font_asset.h"
#include "interop_text_asset.h"
#include "raw_convert.h"
#include <fmt/format.h>
#include <boost/filesystem.hpp>
#include <set>
#include <string>
#include <vector>


using std::dynamic_pointer_cast;
using std::map;
using std::set;
using std::shared_ptr;
using std::string;
using std::vector;
using boost::optional;


boost::filesystem::path
make_unique (boost::filesystem::path path)
{
	if (!dcp::filesystem::exists(path)) {
		return path;
	}

	for (int i = 0; i < 10000; ++i) {
		boost::filesystem::path p = path.parent_path() / (path.stem().string() + fmt::to_string(i) + path.extension().string());
		if (!dcp::filesystem::exists(p)) {
			return p;
		}
	}

	DCP_ASSERT (false);
	return path;
}


static
void
create_hard_link_or_copy (boost::filesystem::path from, boost::filesystem::path to)
{
	try {
		dcp::filesystem::create_hard_link(from, to);
	} catch (boost::filesystem::filesystem_error& e) {
		if (e.code() == boost::system::errc::cross_device_link) {
			dcp::filesystem::copy_file(from, to);
		} else {
			throw;
		}
	}
}


void
dcp::combine (
	vector<boost::filesystem::path> inputs,
	boost::filesystem::path output,
	string issuer,
	string creator,
	string issue_date,
	string annotation_text,
	shared_ptr<const CertificateChain> signer
	)
{
	DCP_ASSERT (!inputs.empty());

	DCP output_dcp (output);
	optional<dcp::Standard> standard;

	for (auto i: inputs) {
		DCP dcp (i);
		dcp.read ();
		if (!standard) {
			standard = *dcp.standard();
		} else if (standard != dcp.standard()) {
			throw CombineError ("Cannot combine Interop and SMPTE DCPs.");
		}
	}

	vector<boost::filesystem::path> paths;
	vector<shared_ptr<dcp::Asset>> assets;

	for (auto i: inputs) {
		DCP dcp (i);
		dcp.read ();

		for (auto j: dcp.cpls()) {
			output_dcp.add (j);
		}

		for (auto j: dcp.assets(true)) {
			if (dynamic_pointer_cast<dcp::CPL>(j)) {
				continue;
			}

			auto sub = dynamic_pointer_cast<dcp::InteropTextAsset>(j);
			if (sub) {
				/* Interop fonts are really fiddly.  The font files are assets (in the ASSETMAP)
				 * and also linked from the font XML by filename.  We have to fix both these things,
				 * and re-write the font XML file since the font URI might have changed if it's a duplicate
				 * with another DCP.
				 */
				auto fonts = sub->font_filenames ();
				for (auto const& k: fonts) {
					sub->set_font_file (k.first, make_unique(output / k.second.filename()));
				}
				auto file = sub->file();
				DCP_ASSERT (file);
				auto new_path = make_unique(output / file->filename());
				sub->write (new_path);
				add_to_container(assets, sub->font_assets());
			}

			assets.push_back (j);
		}
	}

	output_dcp.resolve_refs (assets);

	for (auto i: output_dcp.assets()) {
		if (!dynamic_pointer_cast<dcp::FontAsset>(i) && !dynamic_pointer_cast<dcp::CPL>(i)) {
			auto file = i->file();
			DCP_ASSERT (file);
			auto new_path = make_unique(output / file->filename());
			create_hard_link_or_copy (*file, new_path);
			i->set_file (new_path);
		}
	}

	output_dcp.set_issuer(issuer);
	output_dcp.set_creator(creator);
	output_dcp.set_issue_date(issue_date);
	output_dcp.set_annotation_text(annotation_text);

	output_dcp.write_xml(signer);
}
