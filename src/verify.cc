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
#include "reel_picture_asset.h"
#include "reel_sound_asset.h"
#include "mono_picture_asset.h"
#include "mono_picture_frame.h"
#include "stereo_picture_asset.h"
#include "stereo_picture_frame.h"
#include "exceptions.h"
#include "compose.hpp"
#include "raw_convert.h"
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
using boost::shared_ptr;
using boost::optional;
using boost::function;
using boost::dynamic_pointer_cast;

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

private:
	string _message;
	uint64_t _line;
	uint64_t _column;
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
		add("http://www.w3.org/2001/XMLSchema.dtd", "XMLSchema.dtd");
		add("http://www.w3.org/2001/03/xml.xsd", "xml.xsd");
		add("http://www.w3.org/TR/2002/REC-xmldsig-core-20020212/xmldsig-core-schema.xsd", "xmldsig-core-schema.xsd");
		add("http://www.digicine.com/schemas/437-Y/2007/Main-Stereo-Picture-CPL.xsd", "Main-Stereo-Picture-CPL.xsd");
		add("http://www.digicine.com/PROTO-ASDCP-CPL-20040511.xsd", "PROTO-ASDCP-CPL-20040511.xsd");
		add("http://www.digicine.com/PROTO-ASDCP-PKL-20040311.xsd", "PROTO-ASDCP-PKL-20040311.xsd");
		add("http://www.digicine.com/PROTO-ASDCP-AM-20040311.xsd", "PROTO-ASDCP-AM-20040311.xsd");
	}

	InputSource* resolveEntity(XMLCh const *, XMLCh const * system_id)
	{
		string system_id_str = xml_ch_to_string (system_id);
		if (_files.find(system_id_str) == _files.end()) {
			return 0;
		}

		boost::filesystem::path p = _xsd_dtd_directory / _files[system_id_str];
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

static
void
validate_xml (boost::filesystem::path xml_file, boost::filesystem::path xsd_dtd_directory, list<VerificationNote>& notes)
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

		map<string, string> schema;
		schema["http://www.w3.org/2000/09/xmldsig#"] = "xmldsig-core-schema.xsd";
		schema["http://www.w3.org/TR/2002/REC-xmldsig-core-20020212/xmldsig-core-schema.xsd"] = "xmldsig-core-schema.xsd";
		schema["http://www.smpte-ra.org/schemas/429-7/2006/CPL"] = "SMPTE-429-7-2006-CPL.xsd";
		schema["http://www.smpte-ra.org/schemas/429-8/2006/PKL"] = "SMPTE-429-8-2006-PKL.xsd";
		schema["http://www.smpte-ra.org/schemas/429-9/2007/AM"] = "SMPTE-429-9-2007-AM.xsd";
		schema["http://www.digicine.com/schemas/437-Y/2007/Main-Stereo-Picture-CPL.xsd"] = "Main-Stereo-Picture-CPL.xsd";
		schema["http://www.digicine.com/PROTO-ASDCP-CPL-20040511#"] = "PROTO-ASDCP-CPL-20040511.xsd";
		schema["http://www.digicine.com/PROTO-ASDCP-PKL-20040311#"] = "PROTO-ASDCP-PKL-20040311.xsd";
		schema["http://www.digicine.com/PROTO-ASDCP-AM-20040311#"] = "PROTO-ASDCP-AM-20040311.xsd";

		string locations;
		for (map<string, string>::const_iterator i = schema.begin(); i != schema.end(); ++i) {
			locations += i->first;
			locations += " ";
			boost::filesystem::path p = xsd_dtd_directory / i->second;
			locations += p.string() + " ";
		}

		parser.setExternalSchemaLocation(locations.c_str());
		parser.setValidationSchemaFullChecking(true);
		parser.setErrorHandler(&error_handler);

		LocalFileResolver resolver (xsd_dtd_directory);
		parser.setEntityResolver(&resolver);

		try {
			parser.resetDocumentPool();
			parser.parse(xml_file.string().c_str());
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
				xml_file,
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
		return VERIFY_ASSET_RESULT_CPL_PKL_DIFFER;
	}

	if (actual_hash != *pkl_hash) {
		return VERIFY_ASSET_RESULT_BAD;
	}

	return VERIFY_ASSET_RESULT_GOOD;
}


enum VerifyPictureAssetResult
{
	VERIFY_PICTURE_ASSET_RESULT_GOOD,
	VERIFY_PICTURE_ASSET_RESULT_FRAME_NEARLY_TOO_BIG,
	VERIFY_PICTURE_ASSET_RESULT_BAD,
};


int
biggest_frame_size (shared_ptr<const MonoPictureFrame> frame)
{
	return frame->j2k_size ();
}

int
biggest_frame_size (shared_ptr<const StereoPictureFrame> frame)
{
	return max(frame->left_j2k_size(), frame->right_j2k_size());
}


template <class A, class R, class F>
optional<VerifyPictureAssetResult>
verify_picture_asset_type (shared_ptr<ReelMXF> reel_mxf, function<void (float)> progress)
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
		return VERIFY_PICTURE_ASSET_RESULT_FRAME_NEARLY_TOO_BIG;
	}

	return VERIFY_PICTURE_ASSET_RESULT_GOOD;
}


static VerifyPictureAssetResult
verify_picture_asset (shared_ptr<ReelMXF> reel_mxf, function<void (float)> progress)
{
	optional<VerifyPictureAssetResult> r = verify_picture_asset_type<MonoPictureAsset, MonoPictureAssetReader, MonoPictureFrame>(reel_mxf, progress);
	if (!r) {
		r = verify_picture_asset_type<StereoPictureAsset, StereoPictureAssetReader, StereoPictureFrame>(reel_mxf, progress);
	}

	DCP_ASSERT (r);
	return *r;
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
		}

		BOOST_FOREACH (shared_ptr<CPL> cpl, dcp->cpls()) {
			stage ("Checking CPL", cpl->file());
			validate_xml (cpl->file().get(), xsd_dtd_directory, notes);

			/* Check that the CPL's hash corresponds to the PKL */
			BOOST_FOREACH (shared_ptr<PKL> i, dcp->pkls()) {
				optional<string> h = i->hash(cpl->id());
				if (h && make_digest(Data(*cpl->file())) != *h) {
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
						boost::filesystem::path const file = *reel->main_picture()->asset()->file();
						stage ("Checking picture asset hash", file);
						VerifyAssetResult const r = verify_asset (dcp, reel->main_picture(), progress);
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
									VerificationNote::VERIFY_ERROR, VerificationNote::PKL_CPL_PICTURE_HASHES_DISAGREE, file
									)
								);
							break;
						default:
							break;
						}
						stage ("Checking picture frame sizes", reel->main_picture()->asset()->file());
						VerifyPictureAssetResult const pr = verify_picture_asset (reel->main_picture(), progress);
						switch (pr) {
						case VERIFY_PICTURE_ASSET_RESULT_BAD:
							notes.push_back (
								VerificationNote(
									VerificationNote::VERIFY_ERROR, VerificationNote::PICTURE_FRAME_TOO_LARGE, file
									)
								);
							break;
						case VERIFY_PICTURE_ASSET_RESULT_FRAME_NEARLY_TOO_BIG:
							notes.push_back (
								VerificationNote(
									VerificationNote::VERIFY_WARNING, VerificationNote::PICTURE_FRAME_NEARLY_TOO_LARGE, file
									)
								);
							break;
						default:
							break;
						}
					}
				}
				if (reel->main_sound() && reel->main_sound()->asset_ref().resolved()) {
					stage ("Checking sound asset hash", reel->main_sound()->asset()->file());
					VerifyAssetResult const r = verify_asset (dcp, reel->main_sound(), progress);
					switch (r) {
					case VERIFY_ASSET_RESULT_BAD:
						notes.push_back (
							VerificationNote(
								VerificationNote::VERIFY_ERROR, VerificationNote::SOUND_HASH_INCORRECT, *reel->main_sound()->asset()->file()
								)
							);
						break;
					case VERIFY_ASSET_RESULT_CPL_PKL_DIFFER:
						notes.push_back (
							VerificationNote(
								VerificationNote::VERIFY_ERROR, VerificationNote::PKL_CPL_SOUND_HASHES_DISAGREE, *reel->main_sound()->asset()->file()
								)
							);
						break;
					default:
						break;
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
		return "The hash of the CPL in the PKL does not agree with the CPL file";
	case dcp::VerificationNote::INVALID_PICTURE_FRAME_RATE:
		return "The picture in a reel has an invalid frame rate";
	case dcp::VerificationNote::PICTURE_HASH_INCORRECT:
		return dcp::String::compose("The hash of the picture asset %1 does not agree with the PKL file", note.file()->filename());
	case dcp::VerificationNote::PKL_CPL_PICTURE_HASHES_DISAGREE:
		return dcp::String::compose("The PKL and CPL hashes disagree for the picture asset %1", note.file()->filename());
	case dcp::VerificationNote::SOUND_HASH_INCORRECT:
		return dcp::String::compose("The hash of the sound asset %1 does not agree with the PKL file", note.file()->filename());
	case dcp::VerificationNote::PKL_CPL_SOUND_HASHES_DISAGREE:
		return dcp::String::compose("The PKL and CPL hashes disagree for the sound asset %1", note.file()->filename());
	case dcp::VerificationNote::EMPTY_ASSET_PATH:
		return "The asset map contains an empty asset path.";
	case dcp::VerificationNote::MISSING_ASSET:
		return String::compose("The file for an asset in the asset map cannot be found; missing file is %1.", note.file()->filename());
	case dcp::VerificationNote::MISMATCHED_STANDARD:
		return "The DCP contains both SMPTE and Interop parts.";
	case dcp::VerificationNote::XML_VALIDATION_ERROR:
		return String::compose("An XML file is badly formed: %1 (%2:%3)", note.note().get(), note.file()->filename(), note.line().get());
	case dcp::VerificationNote::MISSING_ASSETMAP:
		return "No ASSETMAP or ASSETMAP.xml was found";
	case dcp::VerificationNote::INTRINSIC_DURATION_TOO_SMALL:
		return String::compose("The intrinsic duration of an asset is less than 1 second long: %1", note.note().get());
	case dcp::VerificationNote::DURATION_TOO_SMALL:
		return String::compose("The duration of an asset is less than 1 second long: %1", note.note().get());
	case dcp::VerificationNote::PICTURE_FRAME_TOO_LARGE:
		return String::compose("The instantaneous bit rate of the picture asset %1 is larger than the limit of 250Mbit/s in at least one place", note.file()->filename());
	case dcp::VerificationNote::PICTURE_FRAME_NEARLY_TOO_LARGE:
		return String::compose("The instantaneous bit rate of the picture asset %1 is close to the limit of 250Mbit/s in at least one place", note.file()->filename());
	}

	return "";
}
