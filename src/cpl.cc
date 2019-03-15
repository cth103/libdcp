/*
    Copyright (C) 2012-2016 Carl Hetherington <cth@carlh.net>

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

#include "cpl.h"
#include "util.h"
#include "reel.h"
#include "metadata.h"
#include "certificate_chain.h"
#include "xml.h"
#include "reel_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_subtitle_asset.h"
#include "reel_closed_caption_asset.h"
#include "reel_atmos_asset.h"
#include "local_time.h"
#include "dcp_assert.h"
#include "compose.hpp"
#include <libxml/parser.h>
#include <libxml++/libxml++.h>
#include <boost/foreach.hpp>

using std::string;
using std::list;
using std::pair;
using std::make_pair;
using std::cout;
using boost::shared_ptr;
using boost::optional;
using boost::dynamic_pointer_cast;
using namespace dcp;

static string const cpl_interop_ns = "http://www.digicine.com/PROTO-ASDCP-CPL-20040511#";
static string const cpl_smpte_ns   = "http://www.smpte-ra.org/schemas/429-7/2006/CPL";

CPL::CPL (string annotation_text, ContentKind content_kind)
	/* default _content_title_text to annotation_text */
	: _content_title_text (annotation_text)
	, _content_kind (content_kind)
{
	_metadata.annotation_text = annotation_text;
	/* default _content_version_id to a random ID and _content_version_label to
	   a random ID and the current time.
	*/
	string const uuid = make_uuid();
	_content_version_id = "urn:uuid:" + uuid;
	_content_version_label_text = uuid + LocalTime().as_string ();
}

/** Construct a CPL object from a XML file */
CPL::CPL (boost::filesystem::path file)
	: Asset (file)
	, _content_kind (FEATURE)
{
	cxml::Document f ("CompositionPlaylist");
	f.read_file (file);

	if (f.namespace_uri() == cpl_interop_ns) {
		_standard = INTEROP;
	} else if (f.namespace_uri() == cpl_smpte_ns) {
		_standard = SMPTE;
	} else {
		boost::throw_exception (XMLError ("Unrecognised CPL namespace " + f.namespace_uri()));
	}

	_id = remove_urn_uuid (f.string_child ("Id"));
	_metadata.annotation_text = f.optional_string_child ("AnnotationText").get_value_or ("");
	_metadata.issuer = f.optional_string_child ("Issuer").get_value_or ("");
	_metadata.creator = f.optional_string_child ("Creator").get_value_or ("");
	_metadata.issue_date = f.string_child ("IssueDate");
	_content_title_text = f.string_child ("ContentTitleText");
	_content_kind = content_kind_from_string (f.string_child ("ContentKind"));
	shared_ptr<cxml::Node> content_version = f.optional_node_child ("ContentVersion");
	if (content_version) {
		_content_version_id = content_version->optional_string_child ("Id").get_value_or ("");
		_content_version_label_text = content_version->string_child ("LabelText");
		content_version->done ();
	}
	f.ignore_child ("RatingList");
	_reels = type_grand_children<Reel> (f, "ReelList", "Reel");

	f.ignore_child ("Issuer");
	f.ignore_child ("Signer");
	f.ignore_child ("Signature");

	f.done ();
}

/** Add a reel to this CPL.
 *  @param reel Reel to add.
 */
void
CPL::add (boost::shared_ptr<Reel> reel)
{
	_reels.push_back (reel);
}

/** Write an CompositonPlaylist XML file.
 *  @param file Filename to write.
 *  @param standard INTEROP or SMPTE.
 *  @param signer Signer to sign the CPL, or 0 to add no signature.
 */
void
CPL::write_xml (boost::filesystem::path file, Standard standard, shared_ptr<const CertificateChain> signer) const
{
	xmlpp::Document doc;
	xmlpp::Element* root;
	if (standard == INTEROP) {
		root = doc.create_root_node ("CompositionPlaylist", cpl_interop_ns);
	} else {
		root = doc.create_root_node ("CompositionPlaylist", cpl_smpte_ns);
	}

	if (signer) {
		root->set_namespace_declaration ("http://www.w3.org/2000/09/xmldsig#", "dsig");
	}

	root->add_child("Id")->add_child_text ("urn:uuid:" + _id);
	root->add_child("AnnotationText")->add_child_text (_metadata.annotation_text);
	root->add_child("IssueDate")->add_child_text (_metadata.issue_date);
	root->add_child("Issuer")->add_child_text (_metadata.issuer);
	root->add_child("Creator")->add_child_text (_metadata.creator);
	root->add_child("ContentTitleText")->add_child_text (_content_title_text);
	root->add_child("ContentKind")->add_child_text (content_kind_to_string (_content_kind));
	{
		xmlpp::Node* cv = root->add_child ("ContentVersion");
		cv->add_child ("Id")->add_child_text (_content_version_id);
		cv->add_child ("LabelText")->add_child_text (_content_version_label_text);
	}
	root->add_child("RatingList");

	xmlpp::Element* reel_list = root->add_child ("ReelList");

	BOOST_FOREACH (shared_ptr<Reel> i, _reels) {
		i->write_to_cpl (reel_list, standard);
	}

	indent (root, 0);

	if (signer) {
		signer->sign (root, standard);
	}

	doc.write_to_file_formatted (file.string(), "UTF-8");

	set_file (file);
}

list<shared_ptr<ReelMXF> >
CPL::reel_mxfs ()
{
	list<shared_ptr<ReelMXF> > c;

	BOOST_FOREACH (shared_ptr<Reel> i, _reels) {
		if (i->main_picture ()) {
			c.push_back (i->main_picture());
		}
		if (i->main_sound ()) {
			c.push_back (i->main_sound());
		}
		if (i->main_subtitle ()) {
			c.push_back (i->main_subtitle());
		}
		BOOST_FOREACH (shared_ptr<ReelClosedCaptionAsset> j, i->closed_captions()) {
			c.push_back (j);
		}
		if (i->atmos ()) {
			c.push_back (i->atmos());
		}
	}

	return c;
}

list<shared_ptr<const ReelMXF> >
CPL::reel_mxfs () const
{
	list<shared_ptr<const ReelMXF> > c;

	BOOST_FOREACH (shared_ptr<Reel> i, _reels) {
		if (i->main_picture ()) {
			c.push_back (i->main_picture());
		}
		if (i->main_sound ()) {
			c.push_back (i->main_sound());
		}
		if (i->main_subtitle ()) {
			c.push_back (i->main_subtitle());
		}
		BOOST_FOREACH (shared_ptr<ReelClosedCaptionAsset> j, i->closed_captions()) {
			c.push_back (j);
		}
		if (i->atmos ()) {
			c.push_back (i->atmos());
		}
	}

	return c;
}

bool
CPL::equals (shared_ptr<const Asset> other, EqualityOptions opt, NoteHandler note) const
{
	shared_ptr<const CPL> other_cpl = dynamic_pointer_cast<const CPL> (other);
	if (!other_cpl) {
		return false;
	}

	if (_metadata.annotation_text != other_cpl->_metadata.annotation_text && !opt.cpl_annotation_texts_can_differ) {
		string const s = "CPL: annotation texts differ: " + _metadata.annotation_text + " vs " + other_cpl->_metadata.annotation_text + "\n";
		note (DCP_ERROR, s);
		return false;
	}

	if (_content_kind != other_cpl->_content_kind) {
		note (DCP_ERROR, "CPL: content kinds differ");
		return false;
	}

	if (_reels.size() != other_cpl->_reels.size()) {
		note (DCP_ERROR, String::compose ("CPL: reel counts differ (%1 vs %2)", _reels.size(), other_cpl->_reels.size()));
		return false;
	}

	list<shared_ptr<Reel> >::const_iterator a = _reels.begin ();
	list<shared_ptr<Reel> >::const_iterator b = other_cpl->_reels.begin ();

	while (a != _reels.end ()) {
		if (!(*a)->equals (*b, opt, note)) {
			return false;
		}
		++a;
		++b;
	}

	return true;
}

/** @return true if we have any encrypted content */
bool
CPL::encrypted () const
{
	BOOST_FOREACH (shared_ptr<Reel> i, _reels) {
		if (i->encrypted ()) {
			return true;
		}
	}

	return false;
}

/** Add a KDM to this CPL.  If the KDM is for any of this CPLs assets it will be used
 *  to decrypt those assets.
 *  @param kdm KDM.
 */
void
CPL::add (DecryptedKDM const & kdm)
{
	BOOST_FOREACH (shared_ptr<Reel> i, _reels) {
		i->add (kdm);
	}
}

void
CPL::resolve_refs (list<shared_ptr<Asset> > assets)
{
	BOOST_FOREACH (shared_ptr<Reel> i, _reels) {
		i->resolve_refs (assets);
	}
}

string
CPL::pkl_type (Standard standard) const
{
	return static_pkl_type (standard);
}

string
CPL::static_pkl_type (Standard standard)
{
	switch (standard) {
	case INTEROP:
		return "text/xml;asdcpKind=CPL";
	case SMPTE:
		return "text/xml";
	default:
		DCP_ASSERT (false);
	}
}

int64_t
CPL::duration () const
{
	int64_t d = 0;
	BOOST_FOREACH (shared_ptr<Reel> i, _reels) {
		d += i->duration ();
	}
	return d;
}
