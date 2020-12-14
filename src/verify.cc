/*
    Copyright (C) 2018-2020 Carl Hetherington <cth@carlh.net>

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
#include "reel_closed_caption_asset.h"
#include "reel_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_subtitle_asset.h"
#include "interop_subtitle_asset.h"
#include "mono_picture_asset.h"
#include "mono_picture_frame.h"
#include "stereo_picture_asset.h"
#include "stereo_picture_frame.h"
#include "exceptions.h"
#include "compose.hpp"
#include "raw_convert.h"
#include "smpte_subtitle_asset.h"
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/parsers/AbstractDOMParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/dom/DOMLSParser.hpp>
#include <xercesc/dom/DOMException.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMError.hpp>
#include <xercesc/dom/DOMLocator.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMAttr.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <boost/noncopyable.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <map>
#include <list>
#include <vector>
#include <iostream>

using std::list;
using std::vector;
using std::string;
using std::cout;
using std::map;
using std::max;
using std::shared_ptr;
using boost::optional;
using boost::function;
using std::dynamic_pointer_cast;

using namespace dcp;
using namespace xercesc;

static
string
xml_ch_to_string (XMLCh const * a)
{
	char* x = XMLString::transcode(a);
	string const o(x);
	XMLString::release(&x);
	return o;
}

class XMLValidationError
{
public:
	XMLValidationError (SAXParseException const & e)
		: _message (xml_ch_to_string(e.getMessage()))
		, _line (e.getLineNumber())
		, _column (e.getColumnNumber())
		, _public_id (e.getPublicId() ? xml_ch_to_string(e.getPublicId()) : "")
		, _system_id (e.getSystemId() ? xml_ch_to_string(e.getSystemId()) : "")
	{

	}

	string message () const {
		return _message;
	}

	uint64_t line () const {
		return _line;
	}

	uint64_t column () const {
		return _column;
	}

	string public_id () const {
		return _public_id;
	}

	string system_id () const {
		return _system_id;
	}

private:
	string _message;
	uint64_t _line;
	uint64_t _column;
	string _public_id;
	string _system_id;
};


class DCPErrorHandler : public ErrorHandler
{
public:
	void warning(const SAXParseException& e)
	{
		maybe_add (XMLValidationError(e));
	}

	void error(const SAXParseException& e)
	{
		maybe_add (XMLValidationError(e));
	}

	void fatalError(const SAXParseException& e)
	{
		maybe_add (XMLValidationError(e));
	}

	void resetErrors() {
		_errors.clear ();
	}

	list<XMLValidationError> errors () const {
		return _errors;
	}

private:
	void maybe_add (XMLValidationError e)
	{
		/* XXX: nasty hack */
		if (
			e.message().find("schema document") != string::npos &&
			e.message().find("has different target namespace from the one specified in instance document") != string::npos
			) {
			return;
		}

		_errors.push_back (e);
	}

	list<XMLValidationError> _errors;
};

class StringToXMLCh : public boost::noncopyable
{
public:
	StringToXMLCh (string a)
	{
		_buffer = XMLString::transcode(a.c_str());
	}

	~StringToXMLCh ()
	{
		XMLString::release (&_buffer);
	}

	XMLCh const * get () const {
		return _buffer;
	}

private:
	XMLCh* _buffer;
};

class LocalFileResolver : public EntityResolver
{
public:
	LocalFileResolver (boost::filesystem::path xsd_dtd_directory)
		: _xsd_dtd_directory (xsd_dtd_directory)
	{
		/* XXX: I'm not clear on what things need to be in this list; some XSDs are apparently, magically
		 * found without being here.
		 */
		add("http://www.w3.org/2001/XMLSchema.dtd", "XMLSchema.dtd");
		add("http://www.w3.org/2001/03/xml.xsd", "xml.xsd");
		add("http://www.w3.org/TR/2002/REC-xmldsig-core-20020212/xmldsig-core-schema.xsd", "xmldsig-core-schema.xsd");
		add("http://www.digicine.com/schemas/437-Y/2007/Main-Stereo-Picture-CPL.xsd", "Main-Stereo-Picture-CPL.xsd");
		add("http://www.digicine.com/PROTO-ASDCP-CPL-20040511.xsd", "PROTO-ASDCP-CPL-20040511.xsd");
		add("http://www.digicine.com/PROTO-ASDCP-PKL-20040311.xsd", "PROTO-ASDCP-PKL-20040311.xsd");
		add("http://www.digicine.com/PROTO-ASDCP-AM-20040311.xsd", "PROTO-ASDCP-AM-20040311.xsd");
		add("http://www.digicine.com/PROTO-ASDCP-CC-CPL-20070926#", "PROTO-ASDCP-CC-CPL-20070926.xsd");
		add("interop-subs", "DCSubtitle.v1.mattsson.xsd");
		add("http://www.smpte-ra.org/schemas/428-7/2010/DCST.xsd", "SMPTE-428-7-2010-DCST.xsd");
		add("http://www.smpte-ra.org/schemas/429-16/2014/CPL-Metadata", "SMPTE-429-16.xsd");
		add("http://www.dolby.com/schemas/2012/AD", "Dolby-2012-AD.xsd");
		add("http://www.smpte-ra.org/schemas/429-10/2008/Main-Stereo-Picture-CPL", "SMPTE-429-10-2008.xsd");
	}

	InputSource* resolveEntity(XMLCh const *, XMLCh const * system_id)
	{
		if (!system_id) {
			return 0;
		}
		string system_id_str = xml_ch_to_string (system_id);
		boost::filesystem::path p = _xsd_dtd_directory;
		if (_files.find(system_id_str) == _files.end()) {
			p /= system_id_str;
		} else {
			p /= _files[system_id_str];
		}
		StringToXMLCh ch (p.string());
		return new LocalFileInputSource(ch.get());
	}

private:
	void add (string uri, string file)
	{
		_files[uri] = file;
	}

	std::map<string, string> _files;
	boost::filesystem::path _xsd_dtd_directory;
};


static void
parse (XercesDOMParser& parser, boost::filesystem::path xml)
{
	parser.parse(xml.string().c_str());
}


static void
parse (XercesDOMParser& parser, std::string xml)
{
	xercesc::MemBufInputSource buf(reinterpret_cast<unsigned char const*>(xml.c_str()), xml.size(), "");
	parser.parse(buf);
}


template <class T>
void
validate_xml (T xml, boost::filesystem::path xsd_dtd_directory, list<VerificationNote>& notes)
{
	try {
		XMLPlatformUtils::Initialize ();
	} catch (XMLException& e) {
		throw MiscError ("Failed to initialise xerces library");
	}

	DCPErrorHandler error_handler;

	/* All the xerces objects in this scope must be destroyed before XMLPlatformUtils::Terminate() is called */
	{
		XercesDOMParser parser;
		parser.setValidationScheme(XercesDOMParser::Val_Always);
		parser.setDoNamespaces(true);
		parser.setDoSchema(true);

		vector<string> schema;
		schema.push_back("xml.xsd");
		schema.push_back("xmldsig-core-schema.xsd");
		schema.push_back("SMPTE-429-7-2006-CPL.xsd");
		schema.push_back("SMPTE-429-8-2006-PKL.xsd");
		schema.push_back("SMPTE-429-9-2007-AM.xsd");
		schema.push_back("Main-Stereo-Picture-CPL.xsd");
		schema.push_back("PROTO-ASDCP-CPL-20040511.xsd");
		schema.push_back("PROTO-ASDCP-PKL-20040311.xsd");
		schema.push_back("PROTO-ASDCP-AM-20040311.xsd");
		schema.push_back("DCSubtitle.v1.mattsson.xsd");
		schema.push_back("DCDMSubtitle-2010.xsd");
		schema.push_back("PROTO-ASDCP-CC-CPL-20070926.xsd");
		schema.push_back("SMPTE-429-16.xsd");
		schema.push_back("Dolby-2012-AD.xsd");
		schema.push_back("SMPTE-429-10-2008.xsd");
		schema.push_back("xlink.xsd");
		schema.push_back("SMPTE-335-2012.xsd");
		schema.push_back("SMPTE-395-2014-13-1-aaf.xsd");
		schema.push_back("isdcf-mca.xsd");
		schema.push_back("SMPTE-429-12-2008.xsd");

		/* XXX: I'm not especially clear what this is for, but it seems to be necessary.
		 * Schemas that are not mentioned in this list are not read, and the things
		 * they describe are not checked.
		 */
		string locations;
		BOOST_FOREACH (string i, schema) {
			locations += String::compose("%1 %1 ", i, i);
		}

		parser.setExternalSchemaLocation(locations.c_str());
		parser.setValidationSchemaFullChecking(true);
		parser.setErrorHandler(&error_handler);

		LocalFileResolver resolver (xsd_dtd_directory);
		parser.setEntityResolver(&resolver);

		try {
			parser.resetDocumentPool();
			parse(parser, xml);
		} catch (XMLException& e) {
			throw MiscError(xml_ch_to_string(e.getMessage()));
		} catch (DOMException& e) {
			throw MiscError(xml_ch_to_string(e.getMessage()));
		} catch (...) {
			throw MiscError("Unknown exception from xerces");
		}
	}

	XMLPlatformUtils::Terminate ();

	BOOST_FOREACH (XMLValidationError i, error_handler.errors()) {
		notes.push_back (
			VerificationNote(
				VerificationNote::VERIFY_ERROR,
				VerificationNote::XML_VALIDATION_ERROR,
				i.message(),
				boost::trim_copy(i.public_id() + " " + i.system_id()),
				i.line()
				)
			);
	}
}


enum VerifyAssetResult {
	VERIFY_ASSET_RESULT_GOOD,
	VERIFY_ASSET_RESULT_CPL_PKL_DIFFER,
	VERIFY_ASSET_RESULT_BAD
};


static VerifyAssetResult
verify_asset (shared_ptr<const DCP> dcp, shared_ptr<const ReelMXF> reel_mxf, function<void (float)> progress)
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
		return VERIFY_ASSET_RESULT_CPL_PKL_DIFFER;
	}

	if (actual_hash != *pkl_hash) {
		return VERIFY_ASSET_RESULT_BAD;
	}

	return VERIFY_ASSET_RESULT_GOOD;
}


void
verify_language_tag (string tag, list<VerificationNote>& notes)
{
	try {
		dcp::LanguageTag test (tag);
	} catch (dcp::LanguageTagError &) {
		notes.push_back (VerificationNote(VerificationNote::VERIFY_BV21_ERROR, VerificationNote::BAD_LANGUAGE, tag));
	}
}


enum VerifyPictureAssetResult
{
	VERIFY_PICTURE_ASSET_RESULT_GOOD,
	VERIFY_PICTURE_ASSET_RESULT_FRAME_NEARLY_TOO_LARGE,
	VERIFY_PICTURE_ASSET_RESULT_BAD,
};


int
biggest_frame_size (shared_ptr<const MonoPictureFrame> frame)
{
	return frame->size ();
}

int
biggest_frame_size (shared_ptr<const StereoPictureFrame> frame)
{
	return max(frame->left()->size(), frame->right()->size());
}


template <class A, class R, class F>
optional<VerifyPictureAssetResult>
verify_picture_asset_type (shared_ptr<const ReelMXF> reel_mxf, function<void (float)> progress)
{
	shared_ptr<A> asset = dynamic_pointer_cast<A>(reel_mxf->asset_ref().asset());
	if (!asset) {
		return optional<VerifyPictureAssetResult>();
	}

	int biggest_frame = 0;
	shared_ptr<R> reader = asset->start_read ();
	int64_t const duration = asset->intrinsic_duration ();
	for (int64_t i = 0; i < duration; ++i) {
		shared_ptr<const F> frame = reader->get_frame (i);
		biggest_frame = max(biggest_frame, biggest_frame_size(frame));
		progress (float(i) / duration);
	}

	static const int max_frame =   rint(250 * 1000000 / (8 * asset->edit_rate().as_float()));
	static const int risky_frame = rint(230 * 1000000 / (8 * asset->edit_rate().as_float()));
	if (biggest_frame > max_frame) {
		return VERIFY_PICTURE_ASSET_RESULT_BAD;
	} else if (biggest_frame > risky_frame) {
		return VERIFY_PICTURE_ASSET_RESULT_FRAME_NEARLY_TOO_LARGE;
	}

	return VERIFY_PICTURE_ASSET_RESULT_GOOD;
}


static VerifyPictureAssetResult
verify_picture_asset (shared_ptr<const ReelMXF> reel_mxf, function<void (float)> progress)
{
	optional<VerifyPictureAssetResult> r = verify_picture_asset_type<MonoPictureAsset, MonoPictureAssetReader, MonoPictureFrame>(reel_mxf, progress);
	if (!r) {
		r = verify_picture_asset_type<StereoPictureAsset, StereoPictureAssetReader, StereoPictureFrame>(reel_mxf, progress);
	}

	DCP_ASSERT (r);
	return *r;
}


static void
verify_main_picture_asset (
	shared_ptr<const DCP> dcp,
	shared_ptr<const ReelPictureAsset> reel_asset,
	function<void (string, optional<boost::filesystem::path>)> stage,
	function<void (float)> progress,
	list<VerificationNote>& notes
	)
{
	shared_ptr<const PictureAsset> asset = reel_asset->asset();
	boost::filesystem::path const file = *asset->file();
	stage ("Checking picture asset hash", file);
	VerifyAssetResult const r = verify_asset (dcp, reel_asset, progress);
	switch (r) {
		case VERIFY_ASSET_RESULT_BAD:
			notes.push_back (
				VerificationNote(
					VerificationNote::VERIFY_ERROR, VerificationNote::PICTURE_HASH_INCORRECT, file
					)
				);
			break;
		case VERIFY_ASSET_RESULT_CPL_PKL_DIFFER:
			notes.push_back (
				VerificationNote(
					VerificationNote::VERIFY_ERROR, VerificationNote::PKL_CPL_PICTURE_HASHES_DIFFER, file
					)
				);
			break;
		default:
			break;
	}
	stage ("Checking picture frame sizes", asset->file());
	VerifyPictureAssetResult const pr = verify_picture_asset (reel_asset, progress);
	switch (pr) {
		case VERIFY_PICTURE_ASSET_RESULT_BAD:
			notes.push_back (
				VerificationNote(
					VerificationNote::VERIFY_ERROR, VerificationNote::PICTURE_FRAME_TOO_LARGE_IN_BYTES, file
					)
				);
			break;
		case VERIFY_PICTURE_ASSET_RESULT_FRAME_NEARLY_TOO_LARGE:
			notes.push_back (
				VerificationNote(
					VerificationNote::VERIFY_WARNING, VerificationNote::PICTURE_FRAME_NEARLY_TOO_LARGE_IN_BYTES, file
					)
				);
			break;
		default:
			break;
	}

	/* Only flat/scope allowed by Bv2.1 */
	if (
		asset->size() != dcp::Size(2048, 858) &&
		asset->size() != dcp::Size(1998, 1080) &&
		asset->size() != dcp::Size(4096, 1716) &&
		asset->size() != dcp::Size(3996, 2160)) {
		notes.push_back(
			VerificationNote(
				VerificationNote::VERIFY_BV21_ERROR,
				VerificationNote::PICTURE_ASSET_INVALID_SIZE_IN_PIXELS,
				String::compose("%1x%2", asset->size().width, asset->size().height),
				file
				)
			);
	}

	/* Only 24, 25, 48fps allowed for 2K */
	if (
		(asset->size() == dcp::Size(2048, 858) || asset->size() == dcp::Size(1998, 1080)) &&
		(asset->edit_rate() != dcp::Fraction(24, 1) && asset->edit_rate() != dcp::Fraction(25, 1) && asset->edit_rate() != dcp::Fraction(48, 1))
	   ) {
		notes.push_back(
			VerificationNote(
				VerificationNote::VERIFY_BV21_ERROR,
				VerificationNote::PICTURE_ASSET_INVALID_FRAME_RATE_FOR_2K,
				String::compose("%1/%2", asset->edit_rate().numerator, asset->edit_rate().denominator),
				file
				)
			);
	}

	if (asset->size() == dcp::Size(4096, 1716) || asset->size() == dcp::Size(3996, 2160)) {
		/* Only 24fps allowed for 4K */
		if (asset->edit_rate() != dcp::Fraction(24, 1)) {
			notes.push_back(
				VerificationNote(
					VerificationNote::VERIFY_BV21_ERROR,
					VerificationNote::PICTURE_ASSET_INVALID_FRAME_RATE_FOR_4K,
					String::compose("%1/%2", asset->edit_rate().numerator, asset->edit_rate().denominator),
					file
					)
				);
		}

		/* Only 2D allowed for 4K */
		if (dynamic_pointer_cast<const StereoPictureAsset>(asset)) {
			notes.push_back(
				VerificationNote(
					VerificationNote::VERIFY_BV21_ERROR,
					VerificationNote::PICTURE_ASSET_4K_3D,
					file
					)
				);

		}
	}

}


static void
verify_main_sound_asset (
	shared_ptr<const DCP> dcp,
	shared_ptr<const ReelSoundAsset> reel_asset,
	function<void (string, optional<boost::filesystem::path>)> stage,
	function<void (float)> progress,
	list<VerificationNote>& notes
	)
{
	shared_ptr<const dcp::SoundAsset> asset = reel_asset->asset();
	stage ("Checking sound asset hash", asset->file());
	VerifyAssetResult const r = verify_asset (dcp, reel_asset, progress);
	switch (r) {
		case VERIFY_ASSET_RESULT_BAD:
			notes.push_back (
				VerificationNote(
					VerificationNote::VERIFY_ERROR, VerificationNote::SOUND_HASH_INCORRECT, *asset->file()
					)
				);
			break;
		case VERIFY_ASSET_RESULT_CPL_PKL_DIFFER:
			notes.push_back (
				VerificationNote(
					VerificationNote::VERIFY_ERROR, VerificationNote::PKL_CPL_SOUND_HASHES_DIFFER, *asset->file()
					)
				);
			break;
		default:
			break;
	}

	stage ("Checking sound asset metadata", asset->file());

	verify_language_tag (asset->language(), notes);
}


static void
verify_main_subtitle_reel (shared_ptr<const ReelSubtitleAsset> reel_asset, list<VerificationNote>& notes)
{
	/* XXX: is Language compulsory? */
	if (reel_asset->language()) {
		verify_language_tag (*reel_asset->language(), notes);
	}
}


static void
verify_closed_caption_reel (shared_ptr<const ReelClosedCaptionAsset> reel_asset, list<VerificationNote>& notes)
{
	/* XXX: is Language compulsory? */
	if (reel_asset->language()) {
		verify_language_tag (*reel_asset->language(), notes);
	}
}


static void
verify_subtitle_asset (
	shared_ptr<const SubtitleAsset> asset,
	function<void (string, optional<boost::filesystem::path>)> stage,
	boost::filesystem::path xsd_dtd_directory,
	list<VerificationNote>& notes
	)
{
	stage ("Checking subtitle XML", asset->file());
	/* Note: we must not use SubtitleAsset::xml_as_string() here as that will mean the data on disk
	 * gets passed through libdcp which may clean up and therefore hide errors.
	 */
	validate_xml (asset->raw_xml(), xsd_dtd_directory, notes);

	shared_ptr<const SMPTESubtitleAsset> smpte = dynamic_pointer_cast<const SMPTESubtitleAsset>(asset);
	if (smpte) {
		if (smpte->language()) {
			verify_language_tag (*smpte->language(), notes);
		} else {
			notes.push_back (
				VerificationNote(
					VerificationNote::VERIFY_BV21_ERROR, VerificationNote::MISSING_SUBTITLE_LANGUAGE, *asset->file()
					)
				);
		}
		if (boost::filesystem::file_size(*asset->file()) > 115 * 1024 * 1024) {
			notes.push_back (
				VerificationNote(
					VerificationNote::VERIFY_BV21_ERROR, VerificationNote::TIMED_TEXT_ASSET_TOO_LARGE_IN_BYTES, *asset->file()
					)
				);
		}
		/* XXX: I'm not sure what Bv2.1_7.2.1 means when it says "the font resource shall not be larger than 10MB"
		 * but I'm hoping that checking for the total size of all fonts being <= 10MB will do.
		 */
		map<string, ArrayData> fonts = asset->font_data ();
		int total_size = 0;
		for (map<string, ArrayData>::const_iterator i = fonts.begin(); i != fonts.end(); ++i) {
			total_size += i->second.size();
		}
		if (total_size > 10 * 1024 * 1024) {
			notes.push_back (
				VerificationNote(
					VerificationNote::VERIFY_BV21_ERROR, VerificationNote::TIMED_TEXT_FONTS_TOO_LARGE_IN_BYTES, *asset->file()
					)
				);
		}
	}
}


static void
verify_closed_caption_asset (
	shared_ptr<const SubtitleAsset> asset,
	function<void (string, optional<boost::filesystem::path>)> stage,
	boost::filesystem::path xsd_dtd_directory,
	list<VerificationNote>& notes
	)
{
	verify_subtitle_asset (asset, stage, xsd_dtd_directory, notes);

	if (asset->raw_xml().size() > 256 * 1024) {
		notes.push_back (
			VerificationNote(
				VerificationNote::VERIFY_BV21_ERROR, VerificationNote::CLOSED_CAPTION_XML_TOO_LARGE_IN_BYTES, *asset->file()
				)
			);
	}
}


list<VerificationNote>
dcp::verify (
	vector<boost::filesystem::path> directories,
	function<void (string, optional<boost::filesystem::path>)> stage,
	function<void (float)> progress,
	boost::filesystem::path xsd_dtd_directory
	)
{
	xsd_dtd_directory = boost::filesystem::canonical (xsd_dtd_directory);

	list<VerificationNote> notes;

	list<shared_ptr<DCP> > dcps;
	BOOST_FOREACH (boost::filesystem::path i, directories) {
		dcps.push_back (shared_ptr<DCP> (new DCP (i)));
	}

	BOOST_FOREACH (shared_ptr<DCP> dcp, dcps) {
		stage ("Checking DCP", dcp->directory());
		try {
			dcp->read (&notes);
		} catch (ReadError& e) {
			notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::GENERAL_READ, string(e.what())));
		} catch (XMLError& e) {
			notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::GENERAL_READ, string(e.what())));
		} catch (MXFFileError& e) {
			notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::GENERAL_READ, string(e.what())));
		} catch (cxml::Error& e) {
			notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::GENERAL_READ, string(e.what())));
		}

		if (dcp->standard() != dcp::SMPTE) {
			notes.push_back (VerificationNote(VerificationNote::VERIFY_BV21_ERROR, VerificationNote::NOT_SMPTE));
		}

		BOOST_FOREACH (shared_ptr<CPL> cpl, dcp->cpls()) {
			stage ("Checking CPL", cpl->file());
			validate_xml (cpl->file().get(), xsd_dtd_directory, notes);

			BOOST_FOREACH (string const& i, cpl->additional_subtitle_languages()) {
				verify_language_tag (i, notes);
			}

			if (cpl->release_territory()) {
				verify_language_tag (cpl->release_territory().get(), notes);
			}

			/* Check that the CPL's hash corresponds to the PKL */
			BOOST_FOREACH (shared_ptr<PKL> i, dcp->pkls()) {
				optional<string> h = i->hash(cpl->id());
				if (h && make_digest(ArrayData(*cpl->file())) != *h) {
					notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::CPL_HASH_INCORRECT));
				}
			}

			BOOST_FOREACH (shared_ptr<Reel> reel, cpl->reels()) {
				stage ("Checking reel", optional<boost::filesystem::path>());

				BOOST_FOREACH (shared_ptr<ReelAsset> i, reel->assets()) {
					if (i->duration() && (i->duration().get() * i->edit_rate().denominator / i->edit_rate().numerator) < 1) {
						notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::DURATION_TOO_SMALL, i->id()));
					}
					if ((i->intrinsic_duration() * i->edit_rate().denominator / i->edit_rate().numerator) < 1) {
						notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::INTRINSIC_DURATION_TOO_SMALL, i->id()));
					}
				}

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
						verify_main_picture_asset (dcp, reel->main_picture(), stage, progress, notes);
					}
				}

				if (reel->main_sound() && reel->main_sound()->asset_ref().resolved()) {
					verify_main_sound_asset (dcp, reel->main_sound(), stage, progress, notes);
				}

				if (reel->main_subtitle()) {
					verify_main_subtitle_reel (reel->main_subtitle(), notes);
					if (reel->main_subtitle()->asset_ref().resolved()) {
						verify_subtitle_asset (reel->main_subtitle()->asset(), stage, xsd_dtd_directory, notes);
					}
				}

				BOOST_FOREACH (shared_ptr<dcp::ReelClosedCaptionAsset> i, reel->closed_captions()) {
					verify_closed_caption_reel (i, notes);
					if (i->asset_ref().resolved()) {
						verify_closed_caption_asset (i->asset(), stage, xsd_dtd_directory, notes);
					}
				}
			}
		}

		BOOST_FOREACH (shared_ptr<PKL> pkl, dcp->pkls()) {
			stage ("Checking PKL", pkl->file());
			validate_xml (pkl->file().get(), xsd_dtd_directory, notes);
		}

		if (dcp->asset_map_path()) {
			stage ("Checking ASSETMAP", dcp->asset_map_path().get());
			validate_xml (dcp->asset_map_path().get(), xsd_dtd_directory, notes);
		} else {
			notes.push_back (VerificationNote(VerificationNote::VERIFY_ERROR, VerificationNote::MISSING_ASSETMAP));
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
		return "The hash of the CPL in the PKL does not agree with the CPL file.";
	case dcp::VerificationNote::INVALID_PICTURE_FRAME_RATE:
		return "The picture in a reel has an invalid frame rate.";
	case dcp::VerificationNote::PICTURE_HASH_INCORRECT:
		return dcp::String::compose("The hash of the picture asset %1 does not agree with the PKL file.", note.file()->filename());
	case dcp::VerificationNote::PKL_CPL_PICTURE_HASHES_DIFFER:
		return dcp::String::compose("The PKL and CPL hashes differ for the picture asset %1.", note.file()->filename());
	case dcp::VerificationNote::SOUND_HASH_INCORRECT:
		return dcp::String::compose("The hash of the sound asset %1 does not agree with the PKL file.", note.file()->filename());
	case dcp::VerificationNote::PKL_CPL_SOUND_HASHES_DIFFER:
		return dcp::String::compose("The PKL and CPL hashes differ for the sound asset %1.", note.file()->filename());
	case dcp::VerificationNote::EMPTY_ASSET_PATH:
		return "The asset map contains an empty asset path.";
	case dcp::VerificationNote::MISSING_ASSET:
		return String::compose("The file for an asset in the asset map cannot be found; missing file is %1.", note.file()->filename());
	case dcp::VerificationNote::MISMATCHED_STANDARD:
		return "The DCP contains both SMPTE and Interop parts.";
	case dcp::VerificationNote::XML_VALIDATION_ERROR:
		return String::compose("An XML file is badly formed: %1 (%2:%3)", note.note().get(), note.file()->filename(), note.line().get());
	case dcp::VerificationNote::MISSING_ASSETMAP:
		return "No ASSETMAP or ASSETMAP.xml was found.";
	case dcp::VerificationNote::INTRINSIC_DURATION_TOO_SMALL:
		return String::compose("The intrinsic duration of an asset is less than 1 second long: %1", note.note().get());
	case dcp::VerificationNote::DURATION_TOO_SMALL:
		return String::compose("The duration of an asset is less than 1 second long: %1", note.note().get());
	case dcp::VerificationNote::PICTURE_FRAME_TOO_LARGE_IN_BYTES:
		return String::compose("The instantaneous bit rate of the picture asset %1 is larger than the limit of 250Mbit/s in at least one place.", note.file()->filename());
	case dcp::VerificationNote::PICTURE_FRAME_NEARLY_TOO_LARGE_IN_BYTES:
		return String::compose("The instantaneous bit rate of the picture asset %1 is close to the limit of 250Mbit/s in at least one place.", note.file()->filename());
	case dcp::VerificationNote::EXTERNAL_ASSET:
		return String::compose("An asset that this DCP refers to is not included in the DCP.  It may be a VF.  Missing asset is %1.", note.note().get());
	case dcp::VerificationNote::NOT_SMPTE:
		return "This DCP does not use the SMPTE standard, which is required for Bv2.1 compliance.";
	case dcp::VerificationNote::BAD_LANGUAGE:
		return String::compose("The DCP specifies a language '%1' which does not conform to the RFC 5646 standard.", note.note().get());
	case dcp::VerificationNote::PICTURE_ASSET_INVALID_SIZE_IN_PIXELS:
		return String::compose("A picture asset's size (%1) is not one of those allowed by Bv2.1 (2048x858, 1998x1080, 4096x1716 or 3996x2160)", note.note().get());
	case dcp::VerificationNote::PICTURE_ASSET_INVALID_FRAME_RATE_FOR_2K:
		return String::compose("A picture asset's frame rate (%1) is not one of those allowed for 2K DCPs by Bv2.1 (24, 25 or 48fps)", note.note().get());
	case dcp::VerificationNote::PICTURE_ASSET_INVALID_FRAME_RATE_FOR_4K:
		return String::compose("A picture asset's frame rate (%1) is not 24fps as required for 4K DCPs by Bv2.1", note.note().get());
	case dcp::VerificationNote::PICTURE_ASSET_4K_3D:
		return "3D 4K DCPs are not allowed by Bv2.1";
	case dcp::VerificationNote::CLOSED_CAPTION_XML_TOO_LARGE_IN_BYTES:
		return String::compose("The XML for the closed caption asset %1 is longer than the 256KB maximum required by Bv2.1", note.file()->filename());
	case dcp::VerificationNote::TIMED_TEXT_ASSET_TOO_LARGE_IN_BYTES:
		return String::compose("The total size of the timed text asset %1 is larger than the 115MB maximum required by Bv2.1", note.file()->filename());
	case dcp::VerificationNote::TIMED_TEXT_FONTS_TOO_LARGE_IN_BYTES:
		return String::compose("The total size of the fonts in timed text asset %1 is larger than the 10MB maximum required by Bv2.1", note.file()->filename());
	case dcp::VerificationNote::MISSING_SUBTITLE_LANGUAGE:
		return String::compose("The XML for a SMPTE subtitle asset has no <Language> tag, which is required by Bv2.1", note.file()->filename());
	}

	return "";
}
