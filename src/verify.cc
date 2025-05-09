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


/** @file  src/verify.cc
 *  @brief dcp::verify() method and associated code
 */


#include "compose.hpp"
#include "cpl.h"
#include "dcp.h"
#include "exceptions.h"
#include "filesystem.h"
#include "interop_text_asset.h"
#include "mono_j2k_picture_asset.h"
#include "mono_j2k_picture_frame.h"
#include "raw_convert.h"
#include "reel.h"
#include "reel_interop_text_asset.h"
#include "reel_markers_asset.h"
#include "reel_picture_asset.h"
#include "reel_sound_asset.h"
#include "reel_smpte_text_asset.h"
#include "reel_text_asset.h"
#include "smpte_text_asset.h"
#include "stereo_j2k_picture_asset.h"
#include "stereo_j2k_picture_frame.h"
#include "verify.h"
#include "verify_internal.h"
#include "verify_j2k.h"
#include <libxml/parserInternals.h>
#include <xercesc/dom/DOMAttr.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMError.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/dom/DOMException.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/dom/DOMLSParser.hpp>
#include <xercesc/dom/DOMLocator.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/parsers/AbstractDOMParser.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <fmt/format.h>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <map>
#include <numeric>
#include <regex>
#include <set>
#include <vector>


using std::cout;
using std::dynamic_pointer_cast;
using std::function;
using std::list;
using std::make_shared;
using std::map;
using std::max;
using std::set;
using std::shared_ptr;
using std::string;
using std::vector;
using boost::optional;


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
	void warning(const SAXParseException& e) override
	{
		maybe_add (XMLValidationError(e));
	}

	void error(const SAXParseException& e) override
	{
		maybe_add (XMLValidationError(e));
	}

	void fatalError(const SAXParseException& e) override
	{
		maybe_add (XMLValidationError(e));
	}

	void resetErrors() override {
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


class StringToXMLCh
{
public:
	StringToXMLCh (string a)
	{
		_buffer = XMLString::transcode(a.c_str());
	}

	StringToXMLCh (StringToXMLCh const&) = delete;
	StringToXMLCh& operator= (StringToXMLCh const&) = delete;

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
		add("http://www.smpte-ra.org/schemas/428-7/2010/DCST.xsd", "DCDMSubtitle-2010.xsd");
		add("http://www.smpte-ra.org/schemas/428-7/2014/DCST.xsd", "DCDMSubtitle-2014.xsd");
		add("http://www.smpte-ra.org/schemas/429-16/2014/CPL-Metadata", "SMPTE-429-16.xsd");
		add("http://www.dolby.com/schemas/2012/AD", "Dolby-2012-AD.xsd");
		add("http://www.smpte-ra.org/schemas/429-10/2008/Main-Stereo-Picture-CPL", "SMPTE-429-10-2008.xsd");
	}

	InputSource* resolveEntity(XMLCh const *, XMLCh const * system_id) override
	{
		if (!system_id) {
			return 0;
		}
		auto system_id_str = xml_ch_to_string (system_id);
		auto p = _xsd_dtd_directory;
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
	parser.parse(xml.c_str());
}


static void
parse (XercesDOMParser& parser, string xml)
{
	xercesc::MemBufInputSource buf(reinterpret_cast<unsigned char const*>(xml.c_str()), xml.size(), "");
	parser.parse(buf);
}


template <class T>
void
validate_xml(Context& context, T xml)
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
		schema.push_back("DCDMSubtitle-2014.xsd");
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
		for (auto i: schema) {
			locations += String::compose("%1 %1 ", i, i);
		}

		parser.setExternalSchemaLocation(locations.c_str());
		parser.setValidationSchemaFullChecking(true);
		parser.setErrorHandler(&error_handler);

		LocalFileResolver resolver(context.xsd_dtd_directory);
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

	for (auto i: error_handler.errors()) {
		context.error(
			VerificationNote::Code::INVALID_XML,
			i.message(),
			boost::trim_copy(i.public_id() + " " + i.system_id()),
			i.line()
		);
	}
}


enum class VerifyAssetResult {
	GOOD,
	CPL_PKL_DIFFER,
	BAD
};


static VerifyAssetResult
verify_asset(
	Context& context,
	shared_ptr<const ReelFileAsset> reel_file_asset,
	string* reference_hash,
	string* calculated_hash
	)
{
	DCP_ASSERT(reference_hash);
	DCP_ASSERT(calculated_hash);

	/* When reading the DCP the hash will have been set to the one from the PKL/CPL.
	 * We want to calculate the hash of the actual file contents here, so that we
	 * can check it.  unset_hash() means that this calculation will happen on the
	 * call to hash().
	 */
	reel_file_asset->asset_ref()->unset_hash();
	*calculated_hash = reel_file_asset->asset_ref()->hash([&context](int64_t done, int64_t total) {
		context.progress(float(done) / total);
	});

	auto pkls = context.dcp->pkls();
	/* We've read this DCP in so it must have at least one PKL */
	DCP_ASSERT (!pkls.empty());

	auto asset = reel_file_asset->asset_ref().asset();

	optional<string> maybe_pkl_hash;
	for (auto i: pkls) {
		maybe_pkl_hash = i->hash (reel_file_asset->asset_ref()->id());
		if (maybe_pkl_hash) {
			break;
		}
	}

	DCP_ASSERT(maybe_pkl_hash);
	*reference_hash = *maybe_pkl_hash;

	auto cpl_hash = reel_file_asset->hash();
	if (cpl_hash && *cpl_hash != *reference_hash) {
		return VerifyAssetResult::CPL_PKL_DIFFER;
	}

	if (*calculated_hash != *reference_hash) {
		return VerifyAssetResult::BAD;
	}

	return VerifyAssetResult::GOOD;
}


static void
verify_language_tag(Context& context, string tag)
{
	try {
		LanguageTag test (tag);
	} catch (LanguageTagError &) {
		context.bv21_error(VerificationNote::Code::INVALID_LANGUAGE, tag);
	}
}


static void
verify_picture_details(
	Context& context,
	shared_ptr<const ReelFileAsset> reel_file_asset,
	boost::filesystem::path file,
	int64_t start_frame
	)
{
	auto asset = dynamic_pointer_cast<J2KPictureAsset>(reel_file_asset->asset_ref().asset());
	if (!asset) {
		/* No verification of MPEG2 picture assets at the moment */
		return;
	}

	auto const duration = asset->intrinsic_duration ();

	auto check_and_add = [&context](vector<VerificationNote> const& j2k_notes) {
		for (auto i: j2k_notes) {
			context.add_note_if_not_existing(i);
		}
	};

	int const max_frame =   rint(250 * 1000000 / (8 * asset->edit_rate().as_float()));
	int const risky_frame = rint(230 * 1000000 / (8 * asset->edit_rate().as_float()));

	bool any_bad_frames_seen = false;

	auto check_frame_size = [max_frame, risky_frame, file, start_frame, &any_bad_frames_seen](Context& context, int index, int size, int frame_rate) {
		if (size > max_frame) {
			context.add_note(
				VerificationNote(
					VerificationNote::Type::ERROR, VerificationNote::Code::INVALID_PICTURE_FRAME_SIZE_IN_BYTES, file
					).set_frame(start_frame + index).set_frame_rate(frame_rate)
			);
			any_bad_frames_seen = true;
		} else if (size > risky_frame) {
			context.add_note(
				VerificationNote(
					VerificationNote::Type::WARNING, VerificationNote::Code::NEARLY_INVALID_PICTURE_FRAME_SIZE_IN_BYTES, file
					).set_frame(start_frame + index).set_frame_rate(frame_rate)
			);
			any_bad_frames_seen = true;
		}
	};

	if (auto mono_asset = dynamic_pointer_cast<MonoJ2KPictureAsset>(reel_file_asset->asset_ref().asset())) {
		auto reader = mono_asset->start_read ();
		for (int64_t i = 0; i < duration; ++i) {
			auto frame = reader->get_frame (i);
			check_frame_size(context, i, frame->size(), mono_asset->frame_rate().numerator);
			if (!mono_asset->encrypted() || mono_asset->key()) {
				vector<VerificationNote> j2k_notes;
				verify_j2k(frame, start_frame, i, mono_asset->frame_rate().numerator, j2k_notes);
				check_and_add (j2k_notes);
			}
			context.progress(float(i) / duration);
		}
	} else if (auto stereo_asset = dynamic_pointer_cast<StereoJ2KPictureAsset>(asset)) {
		auto reader = stereo_asset->start_read ();
		for (int64_t i = 0; i < duration; ++i) {
			auto frame = reader->get_frame (i);
			check_frame_size(context, i, frame->left()->size(), stereo_asset->frame_rate().numerator);
			check_frame_size(context, i, frame->right()->size(), stereo_asset->frame_rate().numerator);
			if (!stereo_asset->encrypted() || stereo_asset->key()) {
				vector<VerificationNote> j2k_notes;
				verify_j2k(frame->left(), start_frame, i, stereo_asset->frame_rate().numerator, j2k_notes);
				verify_j2k(frame->right(), start_frame, i, stereo_asset->frame_rate().numerator, j2k_notes);
				check_and_add (j2k_notes);
			}
			context.progress(float(i) / duration);
		}

	}

	if (!any_bad_frames_seen) {
		context.ok(VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES, file);
	}
}


static void
verify_main_picture_asset(Context& context, shared_ptr<const ReelPictureAsset> reel_asset, int64_t start_frame)
{
	auto asset = reel_asset->asset();
	auto const file = *asset->file();

	if (
		context.options.check_asset_hashes &&
		(!context.options.maximum_asset_size_for_hash_check || filesystem::file_size(file) < *context.options.maximum_asset_size_for_hash_check) &&
		context.should_verify_asset(reel_asset->id())
	   ) {
		context.stage("Checking picture asset hash", file);
		string reference_hash;
		string calculated_hash;
		auto const r = verify_asset(context, reel_asset, &reference_hash, &calculated_hash);
		switch (r) {
			case VerifyAssetResult::BAD:
				context.add_note(
					dcp::VerificationNote(
						VerificationNote::Type::ERROR,
						VerificationNote::Code::INCORRECT_PICTURE_HASH,
						file
						).set_reference_hash(reference_hash).set_calculated_hash(calculated_hash)
					);
				break;
			case VerifyAssetResult::CPL_PKL_DIFFER:
				context.error(VerificationNote::Code::MISMATCHED_PICTURE_HASHES, file);
				break;
			default:
				context.ok(VerificationNote::Code::CORRECT_PICTURE_HASH, file);
				break;
		}
	}

	if (context.options.check_picture_details) {
		context.stage("Checking picture asset details", asset->file());
		verify_picture_details(context, reel_asset, file, start_frame);
	}

	if (dynamic_pointer_cast<const J2KPictureAsset>(asset)) {
		/* Only flat/scope allowed by Bv2.1 */
		if (
			asset->size() != Size(2048, 858) &&
			asset->size() != Size(1998, 1080) &&
			asset->size() != Size(4096, 1716) &&
			asset->size() != Size(3996, 2160)) {
			context.bv21_error(VerificationNote::Code::INVALID_PICTURE_SIZE_IN_PIXELS, String::compose("%1x%2", asset->size().width, asset->size().height), file);
		}
	} else if (dynamic_pointer_cast<const MPEG2PictureAsset>(asset)) {
		if (asset->size() != Size(1920, 1080)) {
			context.error(VerificationNote::Code::INVALID_PICTURE_SIZE_IN_PIXELS, fmt::format("{}x{}", asset->size().width, asset->size().height), file);
		}
	}

	/* Only 24, 25, 48fps allowed for 2K */
	if (
		(asset->size() == Size(2048, 858) || asset->size() == Size(1998, 1080)) &&
		(asset->edit_rate() != Fraction(24, 1) && asset->edit_rate() != Fraction(25, 1) && asset->edit_rate() != Fraction(48, 1))
	   ) {
		context.bv21_error(
			VerificationNote::Code::INVALID_PICTURE_FRAME_RATE_FOR_2K,
			String::compose("%1/%2", asset->edit_rate().numerator, asset->edit_rate().denominator),
			file
		);
	}

	if (asset->size() == Size(4096, 1716) || asset->size() == Size(3996, 2160)) {
		/* Only 24fps allowed for 4K */
		if (asset->edit_rate() != Fraction(24, 1)) {
			context.bv21_error(
				VerificationNote::Code::INVALID_PICTURE_FRAME_RATE_FOR_4K,
				String::compose("%1/%2", asset->edit_rate().numerator, asset->edit_rate().denominator),
				file
			);
		}

		/* Only 2D allowed for 4K */
		if (dynamic_pointer_cast<const StereoJ2KPictureAsset>(asset)) {
			context.bv21_error(
				VerificationNote::Code::INVALID_PICTURE_ASSET_RESOLUTION_FOR_3D,
				String::compose("%1/%2", asset->edit_rate().numerator, asset->edit_rate().denominator),
				file
			);

		}
	}
}


static void
verify_main_sound_asset(Context& context, shared_ptr<const ReelSoundAsset> reel_asset)
{
	auto asset = reel_asset->asset();
	auto const file = *asset->file();

	if (
		context.options.check_asset_hashes &&
		(!context.options.maximum_asset_size_for_hash_check || filesystem::file_size(file) < *context.options.maximum_asset_size_for_hash_check) &&
		context.should_verify_asset(reel_asset->id())
	   ) {
		context.stage("Checking sound asset hash", file);
		string reference_hash;
		string calculated_hash;
		auto const r = verify_asset(context, reel_asset, &reference_hash, &calculated_hash);
		switch (r) {
			case VerifyAssetResult::BAD:
				context.add_note(
					dcp::VerificationNote(
						VerificationNote::Type::ERROR,
						VerificationNote::Code::INCORRECT_SOUND_HASH,
						file
						).set_reference_hash(reference_hash).set_calculated_hash(calculated_hash)
					);
				break;
			case VerifyAssetResult::CPL_PKL_DIFFER:
				context.error(VerificationNote::Code::MISMATCHED_SOUND_HASHES, file);
				break;
			default:
				break;
		}
	}

	if (!context.audio_channels) {
		context.audio_channels = asset->channels();
	} else if (*context.audio_channels != asset->channels()) {
		context.error(VerificationNote::Code::MISMATCHED_SOUND_CHANNEL_COUNTS, file);
	}

	context.stage("Checking sound asset metadata", file);

	if (auto lang = asset->language()) {
		verify_language_tag(context, *lang);
	}
	if (asset->sampling_rate() != 48000) {
		context.bv21_error(VerificationNote::Code::INVALID_SOUND_FRAME_RATE, fmt::to_string(asset->sampling_rate()), file);
	}
	if (asset->bit_depth() != 24) {
		context.error(VerificationNote::Code::INVALID_SOUND_BIT_DEPTH, fmt::to_string(asset->bit_depth()), file);
	}
}


static void
verify_main_subtitle_reel(Context& context, shared_ptr<const ReelTextAsset> reel_asset)
{
	/* XXX: is Language compulsory? */
	if (reel_asset->language()) {
		verify_language_tag(context, *reel_asset->language());
	}

	if (!reel_asset->entry_point()) {
		context.bv21_error(VerificationNote::Code::MISSING_SUBTITLE_ENTRY_POINT, reel_asset->id());
	} else if (reel_asset->entry_point().get()) {
		context.bv21_error(VerificationNote::Code::INCORRECT_SUBTITLE_ENTRY_POINT, reel_asset->id());
	}
}


static void
verify_closed_caption_reel(Context& context, shared_ptr<const ReelTextAsset> reel_asset)
{
	/* XXX: is Language compulsory? */
	if (reel_asset->language()) {
		verify_language_tag(context, *reel_asset->language());
	}

	if (!reel_asset->entry_point()) {
		context.bv21_error(VerificationNote::Code::MISSING_CLOSED_CAPTION_ENTRY_POINT, reel_asset->id());
	} else if (reel_asset->entry_point().get()) {
		context.bv21_error(VerificationNote::Code::INCORRECT_CLOSED_CAPTION_ENTRY_POINT, reel_asset->id());
	}
}


/** Verify stuff that is common to both subtitles and closed captions */
void
verify_smpte_timed_text_asset (
	Context& context,
	shared_ptr<const SMPTETextAsset> asset,
	optional<int64_t> reel_asset_duration
	)
{
	if (asset->language()) {
		verify_language_tag(context, *asset->language());
	} else if (asset->raw_xml()) {
		/* Raise this error only if we can access the raw XML (i.e. the asset was unencrypted or has been decrypted) */
		context.bv21_error(VerificationNote::Code::MISSING_SUBTITLE_LANGUAGE, *asset->file());
	}

	auto const size = filesystem::file_size(asset->file().get());
	if (size > 115 * 1024 * 1024) {
		context.bv21_error(VerificationNote::Code::INVALID_TIMED_TEXT_SIZE_IN_BYTES, fmt::to_string(size), *asset->file());
	}

	/* XXX: I'm not sure what Bv2.1_7.2.1 means when it says "the font resource shall not be larger than 10MB"
	 * but I'm hoping that checking for the total size of all fonts being <= 10MB will do.
	 */
	auto fonts = asset->font_data ();
	int total_size = 0;
	for (auto i: fonts) {
		total_size += i.second.size();
	}
	if (total_size > 10 * 1024 * 1024) {
		context.bv21_error(VerificationNote::Code::INVALID_TIMED_TEXT_FONT_SIZE_IN_BYTES, fmt::to_string(total_size), asset->file().get());
	}

	if (asset->raw_xml()) {
		/* Raise these errors only if we can access the raw XML (i.e. the asset was unencrypted or has been decrypted) */
		if (!asset->start_time()) {
			context.bv21_error(VerificationNote::Code::MISSING_SUBTITLE_START_TIME, asset->file().get());
		} else if (asset->start_time() != Time()) {
			context.bv21_error(VerificationNote::Code::INVALID_SUBTITLE_START_TIME, asset->file().get());
		}
	}

	if (reel_asset_duration && *reel_asset_duration != asset->intrinsic_duration()) {
		context.bv21_error(
			VerificationNote::Code::MISMATCHED_TIMED_TEXT_DURATION,
			String::compose("%1 %2", *reel_asset_duration, asset->intrinsic_duration()),
			asset->file().get()
			);
	}
}


/** Verify Interop subtitle / CCAP stuff */
void
verify_interop_text_asset(Context& context, shared_ptr<const InteropTextAsset> asset)
{
	if (asset->texts().empty()) {
		context.error(VerificationNote::Code::MISSING_SUBTITLE, asset->id(), asset->file().get());
	}
	auto const unresolved = asset->unresolved_fonts();
	if (!unresolved.empty()) {
		context.error(VerificationNote::Code::MISSING_FONT, unresolved.front());
	}
}


/** Verify SMPTE subtitle-only stuff */
void
verify_smpte_subtitle_asset(Context& context, shared_ptr<const SMPTETextAsset> asset)
{
	if (asset->language()) {
		if (!context.subtitle_language) {
			context.subtitle_language = *asset->language();
		} else if (context.subtitle_language != *asset->language()) {
			context.bv21_error(VerificationNote::Code::MISMATCHED_SUBTITLE_LANGUAGES);
		}
	}

	DCP_ASSERT (asset->resource_id());
	auto xml_id = asset->xml_id();
	if (xml_id) {
		if (asset->resource_id().get() != xml_id) {
			context.bv21_error(VerificationNote::Code::MISMATCHED_TIMED_TEXT_RESOURCE_ID);
		}

		if (asset->id() == asset->resource_id().get() || asset->id() == xml_id) {
			context.bv21_error(VerificationNote::Code::INCORRECT_TIMED_TEXT_ASSET_ID);
		}
	} else {
		context.warning(VerificationNote::Code::MISSED_CHECK_OF_ENCRYPTED);
	}

	if (asset->raw_xml()) {
		/* Deluxe require this in their QC even if it seems never to be mentioned in any standard */
		cxml::Document doc("SubtitleReel");
		doc.read_string(*asset->raw_xml());
		auto issue_date = doc.string_child("IssueDate");
		std::regex reg("^\\d\\d\\d\\d-\\d\\d-\\d\\dT\\d\\d:\\d\\d:\\d\\d$");
		if (!std::regex_match(issue_date, reg)) {
			context.warning(VerificationNote::Code::INVALID_SUBTITLE_ISSUE_DATE, issue_date);
		}
	}
}


/** Verify all subtitle stuff */
static void
verify_subtitle_asset(Context& context, shared_ptr<const TextAsset> asset, optional<int64_t> reel_asset_duration)
{
	context.stage("Checking subtitle XML", asset->file());
	/* Note: we must not use TextAsset::xml_as_string() here as that will mean the data on disk
	 * gets passed through libdcp which may clean up and therefore hide errors.
	 */
	if (asset->raw_xml()) {
		validate_xml(context, asset->raw_xml().get());
	} else {
		context.warning(VerificationNote::Code::MISSED_CHECK_OF_ENCRYPTED);
	}

	auto namespace_count = [](shared_ptr<const TextAsset> asset, string root_node) {
		cxml::Document doc(root_node);
		doc.read_string(asset->raw_xml().get());
		auto root = dynamic_cast<xmlpp::Element const*>(doc.node())->cobj();
		int count = 0;
		for (auto ns = root->nsDef; ns != nullptr; ns = ns->next) {
			++count;
		}
		return count;
	};

	auto interop = dynamic_pointer_cast<const InteropTextAsset>(asset);
	if (interop) {
		verify_interop_text_asset(context, interop);
		if (namespace_count(asset, "DCSubtitle") > 1) {
			context.warning(VerificationNote::Code::INCORRECT_SUBTITLE_NAMESPACE_COUNT, asset->id());
		}
	}

	auto smpte = dynamic_pointer_cast<const SMPTETextAsset>(asset);
	if (smpte) {
		verify_smpte_timed_text_asset(context, smpte, reel_asset_duration);
		verify_smpte_subtitle_asset(context, smpte);
		/* This asset may be encrypted and in that case we'll have no raw_xml() */
		if (asset->raw_xml() && namespace_count(asset, "SubtitleReel") > 1) {
			context.warning(VerificationNote::Code::INCORRECT_SUBTITLE_NAMESPACE_COUNT, asset->id());
		}
	}
}


/** Verify all closed caption stuff */
static void
verify_closed_caption_asset (
	Context& context,
	shared_ptr<const TextAsset> asset,
	optional<int64_t> reel_asset_duration
	)
{
	context.stage("Checking closed caption XML", asset->file());
	/* Note: we must not use TextAsset::xml_as_string() here as that will mean the data on disk
	 * gets passed through libdcp which may clean up and therefore hide errors.
	 */
	auto raw_xml = asset->raw_xml();
	if (raw_xml) {
		validate_xml(context, *raw_xml);
		if (raw_xml->size() > 256 * 1024) {
			context.bv21_error(VerificationNote::Code::INVALID_CLOSED_CAPTION_XML_SIZE_IN_BYTES, fmt::to_string(raw_xml->size()), *asset->file());
		}
	} else {
		context.warning(VerificationNote::Code::MISSED_CHECK_OF_ENCRYPTED);
	}

	auto interop = dynamic_pointer_cast<const InteropTextAsset>(asset);
	if (interop) {
		verify_interop_text_asset(context, interop);
	}

	auto smpte = dynamic_pointer_cast<const SMPTETextAsset>(asset);
	if (smpte) {
		verify_smpte_timed_text_asset(context, smpte, reel_asset_duration);
	}
}


/** Check the timing of the individual subtitles and make sure there are no empty <Text> nodes etc. */
static
void
verify_text_details (
	Context& context,
	vector<shared_ptr<Reel>> reels,
	int edit_rate,
	std::function<bool (shared_ptr<Reel>)> check,
	std::function<optional<string> (shared_ptr<Reel>)> xml,
	std::function<int64_t (shared_ptr<Reel>)> duration,
	std::function<std::string (shared_ptr<Reel>)> id
	)
{
	/* end of last subtitle (in editable units) */
	optional<int64_t> last_out;
	auto too_short = false;
	auto too_short_bv21 = false;
	auto too_close = false;
	auto too_early = false;
	auto reel_overlap = false;
	auto empty_text = false;
	/* current reel start time (in editable units) */
	int64_t reel_offset = 0;
	optional<string> missing_load_font_id;

	std::function<void (cxml::ConstNodePtr, optional<int>, optional<Time>, int, bool, bool&, vector<string>&)> parse;

	parse = [&parse, &last_out, &too_short, &too_short_bv21, &too_close, &too_early, &empty_text, &reel_offset, &missing_load_font_id](
		cxml::ConstNodePtr node,
		optional<int> tcr,
		optional<Time> start_time,
		int er,
		bool first_reel,
		bool& has_text,
		vector<string>& font_ids
		) {
		if (node->name() == "Subtitle") {
			Time in (node->string_attribute("TimeIn"), tcr);
			if (start_time) {
				in -= *start_time;
			}
			Time out (node->string_attribute("TimeOut"), tcr);
			if (start_time) {
				out -= *start_time;
			}
			if (first_reel && tcr && in < Time(0, 0, 4, 0, *tcr)) {
				too_early = true;
			}
			auto length = out - in;
			if (length.as_editable_units_ceil(er) <= 0) {
				too_short = true;
			} else if (length.as_editable_units_ceil(er) < 15) {
				too_short_bv21 = true;
			}
			if (last_out) {
				/* XXX: this feels dubious - is it really what Bv2.1 means? */
				auto distance = reel_offset + in.as_editable_units_ceil(er) - *last_out;
				if (distance >= 0 && distance < 2) {
					too_close = true;
				}
			}
			last_out = reel_offset + out.as_editable_units_floor(er);
		} else if (node->name() == "Text") {
			std::function<bool (cxml::ConstNodePtr)> node_has_content = [&](cxml::ConstNodePtr node) {
				if (!node->content().empty()) {
					return true;
				}
				for (auto i: node->node_children()) {
					if (node_has_content(i)) {
						return true;
					}
				}
				return false;
			};
			if (!node_has_content(node)) {
				empty_text = true;
			}
			has_text = true;
		} else if (node->name() == "LoadFont") {
			if (auto const id = node->optional_string_attribute("Id")) {
				font_ids.push_back(*id);
			} else if (auto const id = node->optional_string_attribute("ID")) {
				font_ids.push_back(*id);
			}
		} else if (node->name() == "Font") {
			if (auto const font_id = node->optional_string_attribute("Id")) {
				if (std::find_if(font_ids.begin(), font_ids.end(), [font_id](string const& id) { return id == font_id; }) == font_ids.end()) {
					missing_load_font_id = font_id;
				}
			}
		}
		for (auto i: node->node_children()) {
			parse(i, tcr, start_time, er, first_reel, has_text, font_ids);
		}
	};

	for (auto i = 0U; i < reels.size(); ++i) {
		if (!check(reels[i])) {
			continue;
		}

		auto reel_xml = xml(reels[i]);
		if (!reel_xml) {
			context.warning(VerificationNote::Code::MISSED_CHECK_OF_ENCRYPTED);
			continue;
		}

		/* We need to look at <Subtitle> instances in the XML being checked, so we can't use the subtitles
		 * read in by libdcp's parser.
		 */

		shared_ptr<cxml::Document> doc;
		optional<int> tcr;
		optional<Time> start_time;
		switch (context.dcp->standard().get_value_or(dcp::Standard::SMPTE)) {
		case dcp::Standard::INTEROP:
			doc = make_shared<cxml::Document>("DCSubtitle");
			doc->read_string (*reel_xml);
			break;
		case dcp::Standard::SMPTE:
			doc = make_shared<cxml::Document>("SubtitleReel");
			doc->read_string (*reel_xml);
			tcr = doc->number_child<int>("TimeCodeRate");
			if (auto start_time_string = doc->optional_string_child("StartTime")) {
				start_time = Time(*start_time_string, tcr);
			}
			break;
		}
		bool has_text = false;
		vector<string> font_ids;
		parse(doc, tcr, start_time, edit_rate, i == 0, has_text, font_ids);
		auto end = reel_offset + duration(reels[i]);
		if (last_out && *last_out > end) {
			reel_overlap = true;
		}
		reel_offset = end;

		if (context.dcp->standard() && *context.dcp->standard() == dcp::Standard::SMPTE && has_text && font_ids.empty()) {
			context.add_note(dcp::VerificationNote(dcp::VerificationNote::Type::ERROR, VerificationNote::Code::MISSING_LOAD_FONT).set_id(id(reels[i])));
		}
	}

	if (last_out && *last_out > reel_offset) {
		reel_overlap = true;
	}

	if (too_early) {
		context.warning(VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME);
	}

	if (too_short) {
		context.error(VerificationNote::Code::INVALID_SUBTITLE_DURATION);
	}

	if (too_short_bv21) {
		context.warning(VerificationNote::Code::INVALID_SUBTITLE_DURATION_BV21);
	}

	if (too_close) {
		context.warning(VerificationNote::Code::INVALID_SUBTITLE_SPACING);
	}

	if (reel_overlap) {
		context.error(VerificationNote::Code::SUBTITLE_OVERLAPS_REEL_BOUNDARY);
	}

	if (empty_text) {
		context.warning(VerificationNote::Code::EMPTY_TEXT);
	}

	if (missing_load_font_id) {
		context.add_note(dcp::VerificationNote(VerificationNote::Type::ERROR, VerificationNote::Code::MISSING_LOAD_FONT_FOR_FONT).set_id(*missing_load_font_id));
	}
}


static
void
verify_closed_caption_details(Context& context, vector<shared_ptr<Reel>> reels)
{
	std::function<void (cxml::ConstNodePtr node, std::vector<cxml::ConstNodePtr>& text_or_image)> find_text_or_image;
	find_text_or_image = [&find_text_or_image](cxml::ConstNodePtr node, std::vector<cxml::ConstNodePtr>& text_or_image) {
		for (auto i: node->node_children()) {
			if (i->name() == "Text") {
				text_or_image.push_back (i);
			} else {
				find_text_or_image (i, text_or_image);
			}
		}
	};

	auto mismatched_valign = false;
	auto incorrect_order = false;

	std::function<void (cxml::ConstNodePtr)> parse;
	parse = [&parse, &find_text_or_image, &mismatched_valign, &incorrect_order](cxml::ConstNodePtr node) {
		if (node->name() == "Subtitle") {
			vector<cxml::ConstNodePtr> text_or_image;
			find_text_or_image (node, text_or_image);
			optional<string> last_valign;
			optional<float> last_vpos;
			for (auto i: text_or_image) {
				auto valign = i->optional_string_attribute("VAlign");
				if (!valign) {
					valign = i->optional_string_attribute("Valign").get_value_or("center");
				}
				auto vpos = i->optional_number_attribute<float>("VPosition");
				if (!vpos) {
					vpos = i->optional_number_attribute<float>("Vposition").get_value_or(50);
				}

				if (last_valign) {
					if (*last_valign != valign) {
						mismatched_valign = true;
					}
				}
				last_valign = valign;

				if (!mismatched_valign) {
					if (last_vpos) {
						if (*last_valign == "top" || *last_valign == "center") {
							if (*vpos < *last_vpos) {
								incorrect_order = true;
							}
						} else {
							if (*vpos > *last_vpos) {
								incorrect_order = true;
							}
						}
					}
					last_vpos = vpos;
				}
			}
		}

		for (auto i: node->node_children()) {
			parse(i);
		}
	};

	for (auto reel: reels) {
		for (auto ccap: reel->closed_captions()) {
			auto reel_xml = ccap->asset()->raw_xml();
			if (!reel_xml) {
				context.warning(VerificationNote::Code::MISSED_CHECK_OF_ENCRYPTED);
				continue;
			}

			/* We need to look at <Subtitle> instances in the XML being checked, so we can't use the subtitles
			 * read in by libdcp's parser.
			 */

			shared_ptr<cxml::Document> doc;
			optional<int> tcr;
			optional<Time> start_time;
			try {
				doc = make_shared<cxml::Document>("SubtitleReel");
				doc->read_string (*reel_xml);
			} catch (...) {
				doc = make_shared<cxml::Document>("DCSubtitle");
				doc->read_string (*reel_xml);
			}
			parse (doc);
		}
	}

	if (mismatched_valign) {
		context.error(VerificationNote::Code::MISMATCHED_CLOSED_CAPTION_VALIGN);
	}

	if (incorrect_order) {
		context.error(VerificationNote::Code::INCORRECT_CLOSED_CAPTION_ORDERING);
	}
}


void
dcp::verify_text_lines_and_characters(
	shared_ptr<const TextAsset> asset,
	int warning_length,
	int error_length,
	LinesCharactersResult* result
	)
{
	class Event
	{
	public:
		Event (Time time_, float position_, int characters_)
			: time (time_)
			, position (position_)
			, characters (characters_)
		{}

		Event (Time time_, shared_ptr<Event> start_)
			: time (time_)
			, start (start_)
		{}

		Time time;
		int position = 0;   ///< vertical position from 0 at top of screen to 100 at bottom
		int characters = 0; ///< number of characters in the text of this event
		shared_ptr<Event> start;
	};

	vector<shared_ptr<Event>> events;

	auto position = [](shared_ptr<const TextString> sub) {
		switch (sub->v_align()) {
		case VAlign::TOP:
			return lrintf(sub->v_position() * 100);
		case VAlign::CENTER:
			return lrintf((0.5f + sub->v_position()) * 100);
		case VAlign::BOTTOM:
			return lrintf((1.0f - sub->v_position()) * 100);
		}

		return 0L;
	};

	/* Make a list of "subtitle starts" and "subtitle ends" events */
	for (auto j: asset->texts()) {
		if (auto text = dynamic_pointer_cast<const TextString>(j)) {
			auto in = make_shared<Event>(text->in(), position(text), text->text().length());
			events.push_back(in);
			events.push_back(make_shared<Event>(text->out(), in));
		}
	}

	std::sort(events.begin(), events.end(), [](shared_ptr<Event> const& a, shared_ptr<Event> const& b) {
		return a->time < b->time;
	});

	/* The number of characters currently displayed at different vertical positions, i.e. on
	 * what we consider different lines.  Key is the vertical position (0 to 100) and the value
	 * is a list of the active subtitles in that position.
	 */
	map<int, vector<shared_ptr<Event>>> current;
	for (auto i: events) {
		if (current.size() > 3) {
			result->line_count_exceeded = true;
		}
		for (auto j: current) {
			int length = std::accumulate(j.second.begin(), j.second.end(), 0, [](int total, shared_ptr<const Event> event) { return total + event->characters; });
			if (length > warning_length) {
				result->warning_length_exceeded = true;
			}
			if (length > error_length) {
				result->error_length_exceeded = true;
			}
		}

		if (i->start) {
			/* end of a subtitle */
			auto iter = current.find(i->start->position);
			/* It could be that there's no entry in current for this start position:
			 * perhaps the end is before the start, or something else bad.
			 */
			if (iter != current.end()) {
				DCP_ASSERT (current.find(i->start->position) != current.end());
				auto current_position = current[i->start->position];
				auto iter = std::find(current_position.begin(), current_position.end(), i->start);
				if (iter != current_position.end()) {
					current_position.erase(iter);
				}
				if (current_position.empty()) {
					current.erase(i->start->position);
				}
			}
		} else {
			/* start of a subtitle */
			if (current.find(i->position) == current.end()) {
				current[i->position] = vector<shared_ptr<Event>>{i};
			} else {
				current[i->position].push_back(i);
			}
		}
	}
}


static
void
verify_text_details(Context& context, vector<shared_ptr<Reel>> reels)
{
	if (reels.empty()) {
		return;
	}

	if (reels[0]->main_subtitle() && reels[0]->main_subtitle()->asset_ref().resolved()) {
		verify_text_details(context, reels, reels[0]->main_subtitle()->edit_rate().numerator,
			[](shared_ptr<Reel> reel) {
				return static_cast<bool>(reel->main_subtitle());
			},
			[](shared_ptr<Reel> reel) {
				return reel->main_subtitle()->asset()->raw_xml();
			},
			[](shared_ptr<Reel> reel) {
				return reel->main_subtitle()->actual_duration();
			},
			[](shared_ptr<Reel> reel) {
				return reel->main_subtitle()->id();
			}
		);
	}

	for (auto i = 0U; i < reels[0]->closed_captions().size(); ++i) {
		verify_text_details(context, reels, reels[0]->closed_captions()[i]->edit_rate().numerator,
			[i](shared_ptr<Reel> reel) {
				return i < reel->closed_captions().size();
			},
			[i](shared_ptr<Reel> reel) {
				return reel->closed_captions()[i]->asset()->raw_xml();
			},
			[i](shared_ptr<Reel> reel) {
				return reel->closed_captions()[i]->actual_duration();
			},
			[i](shared_ptr<Reel> reel) {
				return reel->closed_captions()[i]->id();
			}
		);
	}

	verify_closed_caption_details(context, reels);
}


void
dcp::verify_extension_metadata(Context& context)
{
	DCP_ASSERT(context.cpl->file());
	cxml::Document doc ("CompositionPlaylist");
	doc.read_file(dcp::filesystem::fix_long_path(context.cpl->file().get()));

	auto missing = false;
	string malformed;

	if (auto reel_list = doc.node_child("ReelList")) {
		auto reels = reel_list->node_children("Reel");
		if (!reels.empty()) {
			if (auto asset_list = reels[0]->optional_node_child("AssetList")) {
				if (auto metadata = asset_list->optional_node_child("CompositionMetadataAsset")) {
					if (auto extension_list = metadata->optional_node_child("ExtensionMetadataList")) {
						missing = true;
						for (auto extension: extension_list->node_children("ExtensionMetadata")) {
							if (extension->optional_string_attribute("scope").get_value_or("") != "http://isdcf.com/ns/cplmd/app") {
								continue;
							}
							missing = false;
							if (auto name = extension->optional_node_child("Name")) {
								if (name->content() != "Application") {
									malformed = "<Name> should be 'Application'";
								}
							}
							if (auto property_list = extension->optional_node_child("PropertyList")) {
								auto properties = property_list->node_children("Property");
								auto is_bv_20_or_21 = [](shared_ptr<const cxml::Node> property) {
									auto name = property->optional_node_child("Name");
									auto value = property->optional_node_child("Value");
									string content = value ? value->content() : "";
									return name && value && name->content() == "DCP Constraints Profile" &&
										(content == "SMPTE-RDD-52:2020-Bv2.0" || content == "SMPTE-RDD-52:2020-Bv2.1");
								};
								if (!std::any_of(properties.begin(), properties.end(), is_bv_20_or_21)) {
									malformed = "No correctly-formed DCP Constraints Profile found";
								}
							}
						}
					} else {
						missing = true;
					}
				}
			}
		}
	}

	if (missing) {
		context.bv21_error(VerificationNote::Code::MISSING_EXTENSION_METADATA, context.cpl->file().get());
	} else if (!malformed.empty()) {
		context.bv21_error(VerificationNote::Code::INVALID_EXTENSION_METADATA, malformed, context.cpl->file().get());
	}
}


bool
pkl_has_encrypted_assets(shared_ptr<const DCP> dcp, shared_ptr<const PKL> pkl)
{
	vector<string> encrypted;
	for (auto i: dcp->cpls()) {
		for (auto j: i->reel_file_assets()) {
			if (j->asset_ref().resolved()) {
				auto mxf = dynamic_pointer_cast<MXF>(j->asset_ref().asset());
				if (mxf && mxf->encrypted()) {
					encrypted.push_back(j->asset_ref().id());
				}
			}
		}
	}

	for (auto i: pkl->assets()) {
		if (find(encrypted.begin(), encrypted.end(), i->id()) != encrypted.end()) {
			return true;
		}
	}

	return false;
}


static
void
verify_reel(
	Context& context,
	shared_ptr<const Reel> reel,
	int64_t start_frame,
	optional<dcp::Size> main_picture_active_area,
	bool* have_main_subtitle,
	bool* have_no_main_subtitle,
	size_t* most_closed_captions,
	size_t* fewest_closed_captions,
	map<Marker, Time>* markers_seen
	)
{
	for (auto i: reel->assets()) {
		if (i->duration() && (i->duration().get() * i->edit_rate().denominator / i->edit_rate().numerator) < 1) {
			context.error(VerificationNote::Code::INVALID_DURATION, i->id());
		}
		if ((i->intrinsic_duration() * i->edit_rate().denominator / i->edit_rate().numerator) < 1) {
			context.error(VerificationNote::Code::INVALID_INTRINSIC_DURATION, i->id());
		}
		auto file_asset = dynamic_pointer_cast<ReelFileAsset>(i);
		if (i->encryptable() && !file_asset->hash()) {
			context.bv21_error(VerificationNote::Code::MISSING_HASH, i->id());
		}
	}

	if (context.dcp->standard() == Standard::SMPTE) {
		boost::optional<int64_t> duration;
		for (auto i: reel->assets()) {
			if (!duration) {
				duration = i->actual_duration();
			} else if (*duration != i->actual_duration()) {
				context.bv21_error(VerificationNote::Code::MISMATCHED_ASSET_DURATION);
				break;
			}
		}
	}

	if (reel->main_picture()) {
		/* Check reel stuff */
		auto const frame_rate = reel->main_picture()->frame_rate();
		if (frame_rate.denominator != 1 ||
		    (frame_rate.numerator != 24 &&
		     frame_rate.numerator != 25 &&
		     frame_rate.numerator != 30 &&
		     frame_rate.numerator != 48 &&
		     frame_rate.numerator != 50 &&
		     frame_rate.numerator != 60 &&
		     frame_rate.numerator != 96)) {
			context.error(VerificationNote::Code::INVALID_PICTURE_FRAME_RATE, String::compose("%1/%2", frame_rate.numerator, frame_rate.denominator));
		}
		/* Check asset */
		if (reel->main_picture()->asset_ref().resolved()) {
			verify_main_picture_asset(context, reel->main_picture(), start_frame);
			auto const asset_size = reel->main_picture()->asset()->size();
			if (main_picture_active_area) {
				if (main_picture_active_area->width > asset_size.width) {
					context.error(
						VerificationNote::Code::INVALID_MAIN_PICTURE_ACTIVE_AREA,
						String::compose("width %1 is bigger than the asset width %2", main_picture_active_area->width, asset_size.width),
						context.cpl->file().get()
					);
				}
				if (main_picture_active_area->height > asset_size.height) {
					context.error(
						VerificationNote::Code::INVALID_MAIN_PICTURE_ACTIVE_AREA,
						String::compose("height %1 is bigger than the asset height %2", main_picture_active_area->height, asset_size.height),
						context.cpl->file().get()
					);
				}
			}
		}

	}

	if (reel->main_sound() && reel->main_sound()->asset_ref().resolved()) {
		verify_main_sound_asset(context, reel->main_sound());
	}

	if (reel->main_subtitle()) {
		verify_main_subtitle_reel(context, reel->main_subtitle());
		if (reel->main_subtitle()->asset_ref().resolved()) {
			verify_subtitle_asset(context, reel->main_subtitle()->asset(), reel->main_subtitle()->duration());
		}
		*have_main_subtitle = true;
	} else {
		*have_no_main_subtitle = true;
	}

	for (auto i: reel->closed_captions()) {
		verify_closed_caption_reel(context, i);
		if (i->asset_ref().resolved()) {
			verify_closed_caption_asset(context, i->asset(), i->duration());
		}
	}

	if (reel->main_markers()) {
		for (auto const& i: reel->main_markers()->get()) {
			markers_seen->insert(i);
		}
		if (reel->main_markers()->entry_point()) {
			context.error(VerificationNote::Code::UNEXPECTED_ENTRY_POINT);
		}
		if (reel->main_markers()->duration()) {
			context.error(VerificationNote::Code::UNEXPECTED_DURATION);
		}
	}

	*fewest_closed_captions = std::min(*fewest_closed_captions, reel->closed_captions().size());
	*most_closed_captions = std::max(*most_closed_captions, reel->closed_captions().size());

}


static
void
verify_cpl(Context& context, shared_ptr<const CPL> cpl)
{
	context.stage("Checking CPL", cpl->file());
	validate_xml(context, cpl->file().get());

	if (cpl->any_encrypted() && !cpl->all_encrypted()) {
		context.bv21_error(VerificationNote::Code::PARTIALLY_ENCRYPTED);
	} else if (cpl->all_encrypted()) {
		context.ok(VerificationNote::Code::ALL_ENCRYPTED);
	} else if (!cpl->all_encrypted()) {
		context.ok(VerificationNote::Code::NONE_ENCRYPTED);
	}

	for (auto const& i: cpl->additional_subtitle_languages()) {
		verify_language_tag(context, i);
	}

	if (!cpl->content_kind().scope() || *cpl->content_kind().scope() == "http://www.smpte-ra.org/schemas/429-7/2006/CPL#standard-content") {
		/* This is a content kind from http://www.smpte-ra.org/schemas/429-7/2006/CPL#standard-content; make sure it's one
		 * of the approved ones.
		 */
		auto all = ContentKind::all();
		auto name = cpl->content_kind().name();
		transform(name.begin(), name.end(), name.begin(), ::tolower);
		auto iter = std::find_if(all.begin(), all.end(), [name](ContentKind const& k) { return !k.scope() && k.name() == name; });
		if (iter == all.end()) {
			context.error(VerificationNote::Code::INVALID_CONTENT_KIND, cpl->content_kind().name());
		} else {
			context.ok(VerificationNote::Code::VALID_CONTENT_KIND, cpl->content_kind().name());
		}
	}

	if (cpl->release_territory()) {
		if (!cpl->release_territory_scope() || cpl->release_territory_scope().get() != "http://www.smpte-ra.org/schemas/429-16/2014/CPL-Metadata#scope/release-territory/UNM49") {
			auto terr = cpl->release_territory().get();
			bool valid = true;
			/* Must be a valid region tag, or "001" */
			try {
				LanguageTag::RegionSubtag test(terr);
			} catch (...) {
				if (terr != "001") {
					context.bv21_error(VerificationNote::Code::INVALID_LANGUAGE, terr);
					valid = false;
				}
			}
			if (valid) {
				context.ok(VerificationNote::Code::VALID_RELEASE_TERRITORY, terr);
			}
		}
	}

	for (auto version: cpl->content_versions()) {
		if (version.label_text.empty()) {
			context.warning(VerificationNote::Code::EMPTY_CONTENT_VERSION_LABEL_TEXT, cpl->file().get());
			break;
		} else {
			context.ok(VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT, version.label_text);
		}
	}

	if (context.dcp->standard() == Standard::SMPTE) {
		if (!cpl->annotation_text()) {
			context.bv21_error(VerificationNote::Code::MISSING_CPL_ANNOTATION_TEXT, cpl->file().get());
		} else if (cpl->annotation_text().get() != cpl->content_title_text()) {
			context.warning(VerificationNote::Code::MISMATCHED_CPL_ANNOTATION_TEXT, cpl->file().get());
		} else {
			context.ok(VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT, cpl->annotation_text().get());
		}
	}

	for (auto i: context.dcp->pkls()) {
		/* Check that the CPL's hash corresponds to the PKL */
		optional<string> h = i->hash(cpl->id());
		auto calculated_cpl_hash = make_digest(ArrayData(*cpl->file()));
		if (h && calculated_cpl_hash != *h) {
			context.add_note(
				dcp::VerificationNote(
					VerificationNote::Type::ERROR,
					VerificationNote::Code::MISMATCHED_CPL_HASHES,
					cpl->file().get()
					).set_calculated_hash(calculated_cpl_hash).set_reference_hash(*h)
				);
		} else {
			context.ok(VerificationNote::Code::MATCHING_CPL_HASHES);
		}

		/* Check that any PKL with a single CPL has its AnnotationText the same as the CPL's ContentTitleText */
		optional<string> required_annotation_text;
		for (auto j: i->assets()) {
			/* See if this is a CPL */
			for (auto k: context.dcp->cpls()) {
				if (j->id() == k->id()) {
					if (!required_annotation_text) {
						/* First CPL we have found; this is the required AnnotationText unless we find another */
						required_annotation_text = cpl->content_title_text();
					} else {
						/* There's more than one CPL so we don't care what the PKL's AnnotationText is */
						required_annotation_text = boost::none;
					}
				}
			}
		}

		if (required_annotation_text && i->annotation_text() != required_annotation_text) {
			context.bv21_error(VerificationNote::Code::MISMATCHED_PKL_ANNOTATION_TEXT_WITH_CPL, i->id(), i->file().get());
		} else {
			context.ok(VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL);
		}
	}

	/* set to true if any reel has a MainSubtitle */
	auto have_main_subtitle = false;
	/* set to true if any reel has no MainSubtitle */
	auto have_no_main_subtitle = false;
	/* fewest number of closed caption assets seen in a reel */
	size_t fewest_closed_captions = SIZE_MAX;
	/* most number of closed caption assets seen in a reel */
	size_t most_closed_captions = 0;
	map<Marker, Time> markers_seen;

	auto const main_picture_active_area = cpl->main_picture_active_area();
	bool active_area_ok = true;
	if (main_picture_active_area && (main_picture_active_area->width % 2)) {
		context.error(
			VerificationNote::Code::INVALID_MAIN_PICTURE_ACTIVE_AREA,
			String::compose("width %1 is not a multiple of 2", main_picture_active_area->width),
			cpl->file().get()
		     );
		active_area_ok = false;
	}
	if (main_picture_active_area && (main_picture_active_area->height % 2)) {
		context.error(
			VerificationNote::Code::INVALID_MAIN_PICTURE_ACTIVE_AREA,
			String::compose("height %1 is not a multiple of 2", main_picture_active_area->height),
			cpl->file().get()
		     );
		active_area_ok = false;
	}

	if (main_picture_active_area && active_area_ok) {
		context.ok(
			VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA, String::compose("%1x%2", main_picture_active_area->width, main_picture_active_area->height),
			cpl->file().get()
			);
	}

	int64_t frame = 0;
	for (auto reel: cpl->reels()) {
		context.stage("Checking reel", optional<boost::filesystem::path>());
		verify_reel(
			context,
			reel,
			frame,
			main_picture_active_area,
			&have_main_subtitle,
			&have_no_main_subtitle,
			&most_closed_captions,
			&fewest_closed_captions,
			&markers_seen
			);
		frame += reel->duration();
	}

	verify_text_details(context, cpl->reels());

	if (context.dcp->standard() == Standard::SMPTE) {
		if (auto msc = cpl->main_sound_configuration()) {
			if (msc->valid() && context.audio_channels && msc->channels() != *context.audio_channels) {
				context.error(
					VerificationNote::Code::INVALID_MAIN_SOUND_CONFIGURATION,
					String::compose("MainSoundConfiguration has %1 channels but sound assets have %2", msc->channels(), *context.audio_channels),
					cpl->file().get()
				);
			}
		}

		if (have_main_subtitle && have_no_main_subtitle) {
			context.bv21_error(VerificationNote::Code::MISSING_MAIN_SUBTITLE_FROM_SOME_REELS);
		}

		if (fewest_closed_captions != most_closed_captions) {
			context.bv21_error(VerificationNote::Code::MISMATCHED_CLOSED_CAPTION_ASSET_COUNTS);
		}

		if (cpl->content_kind() == ContentKind::FEATURE) {
			if (markers_seen.find(Marker::FFEC) == markers_seen.end()) {
				context.bv21_error(VerificationNote::Code::MISSING_FFEC_IN_FEATURE);
			}
			if (markers_seen.find(Marker::FFMC) == markers_seen.end()) {
				context.bv21_error(VerificationNote::Code::MISSING_FFMC_IN_FEATURE);
			}
		}

		auto ffoc = markers_seen.find(Marker::FFOC);
		if (ffoc == markers_seen.end()) {
			context.warning(VerificationNote::Code::MISSING_FFOC);
		} else if (ffoc->second.e != 1) {
			context.warning(VerificationNote::Code::INCORRECT_FFOC, fmt::to_string(ffoc->second.e));
		}

		auto lfoc = markers_seen.find(Marker::LFOC);
		if (lfoc == markers_seen.end()) {
			context.warning(VerificationNote::Code::MISSING_LFOC);
		} else {
			auto lfoc_time = lfoc->second.as_editable_units_ceil(lfoc->second.tcr);
			if (lfoc_time != (cpl->reels().back()->duration() - 1)) {
				context.warning(VerificationNote::Code::INCORRECT_LFOC, fmt::to_string(lfoc_time));
			}
		}

		LinesCharactersResult result;
		for (auto reel: cpl->reels()) {
			if (reel->main_subtitle() && reel->main_subtitle()->asset_ref().resolved()) {
				verify_text_lines_and_characters(reel->main_subtitle()->asset(), 52, 79, &result);
			}
		}

		if (result.line_count_exceeded) {
			context.warning(VerificationNote::Code::INVALID_SUBTITLE_LINE_COUNT);
		}
		if (result.error_length_exceeded) {
			context.warning(VerificationNote::Code::INVALID_SUBTITLE_LINE_LENGTH);
		} else if (result.warning_length_exceeded) {
			context.warning(VerificationNote::Code::NEARLY_INVALID_SUBTITLE_LINE_LENGTH);
		}

		result = LinesCharactersResult();
		for (auto reel: cpl->reels()) {
			for (auto i: reel->closed_captions()) {
				if (i->asset()) {
					verify_text_lines_and_characters(i->asset(), 32, 32, &result);
				}
			}
		}

		if (result.line_count_exceeded) {
			context.bv21_error(VerificationNote::Code::INVALID_CLOSED_CAPTION_LINE_COUNT);
		}
		if (result.error_length_exceeded) {
			context.bv21_error(VerificationNote::Code::INVALID_CLOSED_CAPTION_LINE_LENGTH);
		}

		if (!cpl->read_composition_metadata()) {
			context.bv21_error(VerificationNote::Code::MISSING_CPL_METADATA, cpl->file().get());
		} else if (!cpl->version_number()) {
			context.bv21_error(VerificationNote::Code::MISSING_CPL_METADATA_VERSION_NUMBER, cpl->file().get());
		}

		verify_extension_metadata(context);

		if (cpl->any_encrypted()) {
			cxml::Document doc("CompositionPlaylist");
			DCP_ASSERT(cpl->file());
			doc.read_file(dcp::filesystem::fix_long_path(cpl->file().get()));
			if (!doc.optional_node_child("Signature")) {
				context.bv21_error(VerificationNote::Code::UNSIGNED_CPL_WITH_ENCRYPTED_CONTENT, cpl->file().get());
			}
		}
	}
}


static
void
verify_pkl(Context& context, shared_ptr<const PKL> pkl)
{
	validate_xml(context, pkl->file().get());

	if (pkl_has_encrypted_assets(context.dcp, pkl)) {
		cxml::Document doc("PackingList");
		doc.read_file(dcp::filesystem::fix_long_path(pkl->file().get()));
		if (!doc.optional_node_child("Signature")) {
			context.bv21_error(VerificationNote::Code::UNSIGNED_PKL_WITH_ENCRYPTED_CONTENT, pkl->id(), pkl->file().get());
		}
	}

	set<string> uuid_set;
	for (auto asset: pkl->assets()) {
		if (!uuid_set.insert(asset->id()).second) {
			context.error(VerificationNote::Code::DUPLICATE_ASSET_ID_IN_PKL, pkl->id(), pkl->file().get());
			break;
		}
	}
}



static
void
verify_assetmap(Context& context, shared_ptr<const DCP> dcp)
{
	auto asset_map = dcp->asset_map();
	DCP_ASSERT(asset_map);

	validate_xml(context, asset_map->file().get());

	set<string> uuid_set;
	for (auto const& asset: asset_map->assets()) {
		if (!uuid_set.insert(asset.id()).second) {
			context.error(VerificationNote::Code::DUPLICATE_ASSET_ID_IN_ASSETMAP, asset_map->id(), asset_map->file().get());
			break;
		}
	}
}


dcp::VerificationResult
dcp::verify (
	vector<boost::filesystem::path> directories,
	vector<dcp::DecryptedKDM> kdms,
	function<void (string, optional<boost::filesystem::path>)> stage,
	function<void (float)> progress,
	VerificationOptions options,
	optional<boost::filesystem::path> xsd_dtd_directory
	)
{
	if (!xsd_dtd_directory) {
		xsd_dtd_directory = resources_directory() / "xsd";
	}
	*xsd_dtd_directory = filesystem::canonical(*xsd_dtd_directory);

	vector<VerificationNote> notes;
	Context context(notes, *xsd_dtd_directory, stage, progress, options);

	vector<shared_ptr<DCP>> dcps;
	for (auto i: directories) {
		dcps.push_back (make_shared<DCP>(i));
	}

	for (auto dcp: dcps) {
		stage ("Checking DCP", dcp->directory());

		context.dcp = dcp;

		bool carry_on = true;
		try {
			dcp->read (&notes, true);
		} catch (MissingAssetmapError& e) {
			context.error(VerificationNote::Code::FAILED_READ, string(e.what()));
			carry_on = false;
		} catch (ReadError& e) {
			context.error(VerificationNote::Code::FAILED_READ, string(e.what()));
		} catch (XMLError& e) {
			context.error(VerificationNote::Code::FAILED_READ, string(e.what()));
		} catch (MXFFileError& e) {
			context.error(VerificationNote::Code::FAILED_READ, string(e.what()));
		} catch (BadURNUUIDError& e) {
			context.error(VerificationNote::Code::FAILED_READ, string(e.what()));
		} catch (cxml::Error& e) {
			context.error(VerificationNote::Code::FAILED_READ, string(e.what()));
		} catch (xmlpp::parse_error& e) {
			carry_on = false;
			context.error(VerificationNote::Code::FAILED_READ, string(e.what()));
		}

		if (!carry_on) {
			continue;
		}

		if (dcp->standard() != Standard::SMPTE) {
			notes.push_back ({VerificationNote::Type::BV21_ERROR, VerificationNote::Code::INVALID_STANDARD});
		}

		for (auto kdm: kdms) {
			dcp->add(kdm);
		}

		for (auto cpl: dcp->cpls()) {
			try {
				context.cpl = cpl;
				verify_cpl(context, cpl);
				context.cpl.reset();
			} catch (ReadError& e) {
				notes.push_back({VerificationNote::Type::ERROR, VerificationNote::Code::FAILED_READ, string(e.what())});
			}
		}

		for (auto pkl: dcp->pkls()) {
			stage("Checking PKL", pkl->file());
			verify_pkl(context, pkl);
		}

		if (dcp->asset_map_file()) {
			stage("Checking ASSETMAP", dcp->asset_map_file().get());
			verify_assetmap(context, dcp);
		} else {
			context.error(VerificationNote::Code::MISSING_ASSETMAP);
		}
	}

	return { notes, dcps };
}


string
dcp::note_to_string(VerificationNote note, function<string (string)> process_string, function<string (string)> process_filename)
{
	/** These strings should say what is wrong, incorporating any extra details (ID, filenames etc.).
   	 *
	 *  e.g. "ClosedCaption asset has no <EntryPoint> tag.",
	 *  not "ClosedCaption assets must have an <EntryPoint> tag."
	 *
	 *  It's OK to use XML tag names where they are clear.
	 *  If both ID and filename are available, use only the ID.
	 *  End messages with a full stop.
	 *  Messages should not mention whether or not their errors are a part of Bv2.1.
	 */

	auto filename = [note, process_filename]() {
		return process_filename(note.file()->filename().string());
	};

#define compose(format, ...) String::compose(process_string(format), __VA_ARGS__)

	switch (note.code()) {
	case VerificationNote::Code::FAILED_READ:
		return process_string(*note.note());
	case VerificationNote::Code::MATCHING_CPL_HASHES:
		return process_string("The hash of the CPL in the PKL matches the CPL file.");
	case VerificationNote::Code::MISMATCHED_CPL_HASHES:
		return compose("The hash (%1) of the CPL (%2) in the PKL does not agree with the CPL file (%3).", note.reference_hash().get(), note.cpl_id().get(), note.calculated_hash().get());
	case VerificationNote::Code::INVALID_PICTURE_FRAME_RATE:
		return compose("The picture in a reel has an invalid frame rate %1.", note.note().get());
	case VerificationNote::Code::INCORRECT_PICTURE_HASH:
		return compose("The hash (%1) of the picture asset %2 does not agree with the PKL file (%3).", note.calculated_hash().get(), filename(), note.reference_hash().get());
	case VerificationNote::Code::CORRECT_PICTURE_HASH:
		return compose("The picture asset %1 has the expected hashes in the CPL and PKL.", filename());
	case VerificationNote::Code::MISMATCHED_PICTURE_HASHES:
		return compose("The PKL and CPL hashes differ for the picture asset %1.", filename());
	case VerificationNote::Code::INCORRECT_SOUND_HASH:
		return compose("The hash (%1) of the sound asset %2 does not agree with the PKL file (%3).", note.calculated_hash().get(), filename(), note.reference_hash().get());
	case VerificationNote::Code::MISMATCHED_SOUND_HASHES:
		return compose("The PKL and CPL hashes differ for the sound asset %1.", filename());
	case VerificationNote::Code::EMPTY_ASSET_PATH:
		return process_string("The asset map contains an empty asset path.");
	case VerificationNote::Code::MISSING_ASSET:
		return compose("The file %1 for an asset in the asset map cannot be found.", filename());
	case VerificationNote::Code::MISMATCHED_STANDARD:
		return process_string("The DCP contains both SMPTE and Interop parts.");
	case VerificationNote::Code::INVALID_XML:
		return compose("An XML file is badly formed: %1 (%2:%3)", note.note().get(), filename(), note.line().get());
	case VerificationNote::Code::MISSING_ASSETMAP:
		return process_string("No valid ASSETMAP or ASSETMAP.xml was found.");
	case VerificationNote::Code::INVALID_INTRINSIC_DURATION:
		return compose("The intrinsic duration of the asset %1 is less than 1 second.", note.note().get());
	case VerificationNote::Code::INVALID_DURATION:
		return compose("The duration of the asset %1 is less than 1 second.", note.note().get());
	case VerificationNote::Code::VALID_PICTURE_FRAME_SIZES_IN_BYTES:
		return compose("Each frame of the picture asset %1 has a bit rate safely under the limit of 250Mbit/s.", filename());
	case VerificationNote::Code::INVALID_PICTURE_FRAME_SIZE_IN_BYTES:
		return compose(
			"Frame %1 (timecode %2) in asset %3 has an instantaneous bit rate that is larger than the limit of 250Mbit/s.",
			note.frame().get(),
			dcp::Time(note.frame().get(), note.frame_rate().get(), note.frame_rate().get()).as_string(dcp::Standard::SMPTE),
			filename()
			);
	case VerificationNote::Code::NEARLY_INVALID_PICTURE_FRAME_SIZE_IN_BYTES:
		return compose(
			"Frame %1 (timecode %2) in asset %3 has an instantaneous bit rate that is close to the limit of 250Mbit/s.",
			note.frame().get(),
			dcp::Time(note.frame().get(), note.frame_rate().get(), note.frame_rate().get()).as_string(dcp::Standard::SMPTE),
			filename()
			);
	case VerificationNote::Code::EXTERNAL_ASSET:
		return compose("The asset %1 that this DCP refers to is not included in the DCP.  It may be a VF.", note.note().get());
	case VerificationNote::Code::THREED_ASSET_MARKED_AS_TWOD:
		return compose("The asset %1 is 3D but its MXF is marked as 2D.", filename());
	case VerificationNote::Code::INVALID_STANDARD:
		return "This DCP does not use the SMPTE standard.";
	case VerificationNote::Code::INVALID_LANGUAGE:
		return compose("The DCP specifies a language '%1' which does not conform to the RFC 5646 standard.", note.note().get());
	case VerificationNote::Code::VALID_RELEASE_TERRITORY:
		return compose("Valid release territory %1.", note.note().get());
	case VerificationNote::Code::INVALID_PICTURE_SIZE_IN_PIXELS:
		return compose("The size %1 of picture asset %2 is not allowed.", note.note().get(), filename());
	case VerificationNote::Code::INVALID_PICTURE_FRAME_RATE_FOR_2K:
		return compose("The frame rate %1 of picture asset %2 is not allowed for 2K DCPs.", note.note().get(), filename());
	case VerificationNote::Code::INVALID_PICTURE_FRAME_RATE_FOR_4K:
		return compose("The frame rate %1 of picture asset %2 is not allowed for 4K DCPs.", note.note().get(), filename());
	case VerificationNote::Code::INVALID_PICTURE_ASSET_RESOLUTION_FOR_3D:
		return process_string("3D 4K DCPs are not allowed.");
	case VerificationNote::Code::INVALID_CLOSED_CAPTION_XML_SIZE_IN_BYTES:
		return compose("The size %1 of the closed caption asset %2 is larger than the 256KB maximum.", note.note().get(), filename());
	case VerificationNote::Code::INVALID_TIMED_TEXT_SIZE_IN_BYTES:
		return compose("The size %1 of the timed text asset %2 is larger than the 115MB maximum.", note.note().get(), filename());
	case VerificationNote::Code::INVALID_TIMED_TEXT_FONT_SIZE_IN_BYTES:
		return compose("The size %1 of the fonts in timed text asset %2 is larger than the 10MB maximum.", note.note().get(), filename());
	case VerificationNote::Code::MISSING_SUBTITLE_LANGUAGE:
		return compose("The XML for the SMPTE subtitle asset %1 has no <Language> tag.", filename());
	case VerificationNote::Code::MISMATCHED_SUBTITLE_LANGUAGES:
		return process_string("Some subtitle assets have different <Language> tags than others");
	case VerificationNote::Code::MISSING_SUBTITLE_START_TIME:
		return compose("The XML for the SMPTE subtitle asset %1 has no <StartTime> tag.", filename());
	case VerificationNote::Code::INVALID_SUBTITLE_START_TIME:
		return compose("The XML for a SMPTE subtitle asset %1 has a non-zero <StartTime> tag.", filename());
	case VerificationNote::Code::INVALID_SUBTITLE_FIRST_TEXT_TIME:
		return process_string("The first subtitle or closed caption is less than 4 seconds from the start of the DCP.");
	case VerificationNote::Code::INVALID_SUBTITLE_DURATION:
		return process_string("At least one subtitle has a zero or negative duration.");
	case VerificationNote::Code::INVALID_SUBTITLE_DURATION_BV21:
		return process_string("At least one subtitle lasts less than 15 frames.");
	case VerificationNote::Code::INVALID_SUBTITLE_SPACING:
		return process_string("At least one pair of subtitles is separated by less than 2 frames.");
	case VerificationNote::Code::SUBTITLE_OVERLAPS_REEL_BOUNDARY:
		return process_string("At least one subtitle extends outside of its reel.");
	case VerificationNote::Code::INVALID_SUBTITLE_LINE_COUNT:
		return process_string("There are more than 3 subtitle lines in at least one place in the DCP.");
	case VerificationNote::Code::NEARLY_INVALID_SUBTITLE_LINE_LENGTH:
		return process_string("There are more than 52 characters in at least one subtitle line.");
	case VerificationNote::Code::INVALID_SUBTITLE_LINE_LENGTH:
		return process_string("There are more than 79 characters in at least one subtitle line.");
	case VerificationNote::Code::INVALID_CLOSED_CAPTION_LINE_COUNT:
		return process_string("There are more than 3 closed caption lines in at least one place.");
	case VerificationNote::Code::INVALID_CLOSED_CAPTION_LINE_LENGTH:
		return process_string("There are more than 32 characters in at least one closed caption line.");
	case VerificationNote::Code::INVALID_SOUND_FRAME_RATE:
		return compose("The sound asset %1 has a sampling rate of %2", filename(), note.note().get());
	case VerificationNote::Code::INVALID_SOUND_BIT_DEPTH:
		return compose("The sound asset %1 has a bit depth of %2", filename(), note.note().get());
	case VerificationNote::Code::MISSING_CPL_ANNOTATION_TEXT:
		return compose("The CPL %1 has no <AnnotationText> tag.", note.cpl_id().get());
	case VerificationNote::Code::MISMATCHED_CPL_ANNOTATION_TEXT:
		return compose("The CPL %1 has an <AnnotationText> which differs from its <ContentTitleText>.", note.cpl_id().get());
	case VerificationNote::Code::VALID_CPL_ANNOTATION_TEXT:
		return compose("Valid CPL annotation text %1", note.note().get());
	case VerificationNote::Code::MISMATCHED_ASSET_DURATION:
		return process_string("All assets in a reel do not have the same duration.");
	case VerificationNote::Code::MISSING_MAIN_SUBTITLE_FROM_SOME_REELS:
		return process_string("At least one reel contains a subtitle asset, but some reel(s) do not.");
	case VerificationNote::Code::MISMATCHED_CLOSED_CAPTION_ASSET_COUNTS:
		return process_string("At least one reel has closed captions, but reels have different numbers of closed caption assets.");
	case VerificationNote::Code::MISSING_SUBTITLE_ENTRY_POINT:
		return compose("The subtitle asset %1 has no <EntryPoint> tag.", note.note().get());
	case VerificationNote::Code::INCORRECT_SUBTITLE_ENTRY_POINT:
		return compose("The subtitle asset %1 has an <EntryPoint> other than 0.", note.note().get());
	case VerificationNote::Code::MISSING_CLOSED_CAPTION_ENTRY_POINT:
		return compose("The closed caption asset %1 has no <EntryPoint> tag.", note.note().get());
	case VerificationNote::Code::INCORRECT_CLOSED_CAPTION_ENTRY_POINT:
		return compose("The closed caption asset %1 has an <EntryPoint> other than 0.", note.note().get());
	case VerificationNote::Code::MISSING_HASH:
		return compose("The asset %1 has no <Hash> tag in the CPL.", note.note().get());
	case VerificationNote::Code::MISSING_FFEC_IN_FEATURE:
		return process_string("The DCP is marked as a Feature but there is no FFEC (first frame of end credits) marker.");
	case VerificationNote::Code::MISSING_FFMC_IN_FEATURE:
		return process_string("The DCP is marked as a Feature but there is no FFMC (first frame of moving credits) marker.");
	case VerificationNote::Code::MISSING_FFOC:
		return process_string("There should be a FFOC (first frame of content) marker.");
	case VerificationNote::Code::MISSING_LFOC:
		return process_string("There should be a LFOC (last frame of content) marker.");
	case VerificationNote::Code::INCORRECT_FFOC:
		return compose("The FFOC marker is %1 instead of 1", note.note().get());
	case VerificationNote::Code::INCORRECT_LFOC:
		return compose("The LFOC marker is %1 instead of 1 less than the duration of the last reel.", note.note().get());
	case VerificationNote::Code::MISSING_CPL_METADATA:
		return compose("The CPL %1 has no <CompositionMetadataAsset> tag.", note.cpl_id().get());
	case VerificationNote::Code::MISSING_CPL_METADATA_VERSION_NUMBER:
		return compose("The CPL %1 has no <VersionNumber> in its <CompositionMetadataAsset>.", note.cpl_id().get());
	case VerificationNote::Code::MISSING_EXTENSION_METADATA:
		return compose("The CPL %1 has no <ExtensionMetadata> in its <CompositionMetadataAsset>.", note.cpl_id().get());
	case VerificationNote::Code::INVALID_EXTENSION_METADATA:
		return compose("The CPL %1 has a malformed <ExtensionMetadata> (%2).", filename(), note.note().get());
	case VerificationNote::Code::UNSIGNED_CPL_WITH_ENCRYPTED_CONTENT:
		return compose("The CPL %1, which has encrypted content, is not signed.", note.cpl_id().get());
	case VerificationNote::Code::UNSIGNED_PKL_WITH_ENCRYPTED_CONTENT:
		return compose("The PKL %1, which has encrypted content, is not signed.", note.note().get());
	case VerificationNote::Code::MISMATCHED_PKL_ANNOTATION_TEXT_WITH_CPL:
		return compose("The PKL %1 has only one CPL but its <AnnotationText> does not match the CPL's <ContentTitleText>.", note.note().get());
	case VerificationNote::Code::MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL:
		return process_string("The PKL and CPL annotation texts match.");
	case VerificationNote::Code::ALL_ENCRYPTED:
		return process_string("All the assets are encrypted.");
	case VerificationNote::Code::NONE_ENCRYPTED:
		return process_string("All the assets are unencrypted.");
	case VerificationNote::Code::PARTIALLY_ENCRYPTED:
		return process_string("Some assets are encrypted but some are not.");
	case VerificationNote::Code::INVALID_JPEG2000_CODESTREAM:
		return compose(
			"Frame %1 (timecode %2) has an invalid JPEG2000 codestream (%3).",
			note.frame().get(),
			dcp::Time(note.frame().get(), note.frame_rate().get(), note.frame_rate().get()).as_string(dcp::Standard::SMPTE),
			note.note().get()
			);
	case VerificationNote::Code::INVALID_JPEG2000_GUARD_BITS_FOR_2K:
		return compose("The JPEG2000 codestream uses %1 guard bits in a 2K image instead of 1.", note.note().get());
	case VerificationNote::Code::INVALID_JPEG2000_GUARD_BITS_FOR_4K:
		return compose("The JPEG2000 codestream uses %1 guard bits in a 4K image instead of 2.", note.note().get());
	case VerificationNote::Code::INVALID_JPEG2000_TILE_SIZE:
		return process_string("The JPEG2000 tile size is not the same as the image size.");
	case VerificationNote::Code::INVALID_JPEG2000_CODE_BLOCK_WIDTH:
		return compose("The JPEG2000 codestream uses a code block width of %1 instead of 32.", note.note().get());
	case VerificationNote::Code::INVALID_JPEG2000_CODE_BLOCK_HEIGHT:
		return compose("The JPEG2000 codestream uses a code block height of %1 instead of 32.", note.note().get());
	case VerificationNote::Code::INCORRECT_JPEG2000_POC_MARKER_COUNT_FOR_2K:
		return compose("%1 POC markers found in 2K JPEG2000 codestream instead of 0.", note.note().get());
	case VerificationNote::Code::INCORRECT_JPEG2000_POC_MARKER_COUNT_FOR_4K:
		return compose("%1 POC markers found in 4K JPEG2000 codestream instead of 1.", note.note().get());
	case VerificationNote::Code::INCORRECT_JPEG2000_POC_MARKER:
		return compose("Incorrect POC marker content found (%1).", note.note().get());
	case VerificationNote::Code::INVALID_JPEG2000_POC_MARKER_LOCATION:
		return process_string("POC marker found outside main header.");
	case VerificationNote::Code::INVALID_JPEG2000_TILE_PARTS_FOR_2K:
		return compose("The JPEG2000 codestream has %1 tile parts in a 2K image instead of 3.", note.note().get());
	case VerificationNote::Code::INVALID_JPEG2000_TILE_PARTS_FOR_4K:
		return compose("The JPEG2000 codestream has %1 tile parts in a 4K image instead of 6.", note.note().get());
	case VerificationNote::Code::INVALID_JPEG2000_RSIZ_FOR_2K:
	case VerificationNote::Code::INVALID_JPEG2000_RSIZ_FOR_4K:
		return compose("The JPEG2000 codestream has an invalid Rsiz (capabilities) value of %1.", note.note().get());
	case VerificationNote::Code::MISSING_JPEG200_TLM_MARKER:
		return process_string("No TLM marker was found in a JPEG2000 codestream.");
	case VerificationNote::Code::MISMATCHED_TIMED_TEXT_RESOURCE_ID:
		return process_string("The Resource ID in a timed text MXF did not match the ID of the contained XML.");
	case VerificationNote::Code::INCORRECT_TIMED_TEXT_ASSET_ID:
		return process_string("The Asset ID in a timed text MXF is the same as the Resource ID or that of the contained XML.");
	case VerificationNote::Code::MISMATCHED_TIMED_TEXT_DURATION:
	{
		vector<string> parts;
		boost::split (parts, note.note().get(), boost::is_any_of(" "));
		DCP_ASSERT (parts.size() == 2);
		return compose("The reel duration of some timed text (%1) is not the same as the ContainerDuration of its MXF (%2).", parts[0], parts[1]);
	}
	case VerificationNote::Code::MISSED_CHECK_OF_ENCRYPTED:
		return process_string("Some aspect of this DCP could not be checked because it is encrypted.");
	case VerificationNote::Code::EMPTY_TEXT:
		return process_string("There is an empty <Text> node in a subtitle or closed caption.");
	case VerificationNote::Code::MISMATCHED_CLOSED_CAPTION_VALIGN:
		return process_string("Some closed <Text> or <Image> nodes have different vertical alignments within a <Subtitle>.");
	case VerificationNote::Code::INCORRECT_CLOSED_CAPTION_ORDERING:
		return process_string("Some closed captions are not listed in the order of their vertical position.");
	case VerificationNote::Code::UNEXPECTED_ENTRY_POINT:
		return process_string("There is an <EntryPoint> node inside a <MainMarkers>.");
	case VerificationNote::Code::UNEXPECTED_DURATION:
		return process_string("There is an <Duration> node inside a <MainMarkers>.");
	case VerificationNote::Code::INVALID_CONTENT_KIND:
		return compose("<ContentKind> has an invalid value %1.", note.note().get());
	case VerificationNote::Code::VALID_CONTENT_KIND:
		return compose("Valid <ContentKind> %1.", note.note().get());
	case VerificationNote::Code::INVALID_MAIN_PICTURE_ACTIVE_AREA:
		return compose("<MainPictureActiveaArea> has an invalid value: %1", note.note().get());
	case VerificationNote::Code::VALID_MAIN_PICTURE_ACTIVE_AREA:
		return compose("<MainPictureActiveaArea> %1 is valid", note.note().get());
	case VerificationNote::Code::DUPLICATE_ASSET_ID_IN_PKL:
		return compose("The PKL %1 has more than one asset with the same ID.", note.note().get());
	case VerificationNote::Code::DUPLICATE_ASSET_ID_IN_ASSETMAP:
		return compose("The ASSETMAP %1 has more than one asset with the same ID.", note.note().get());
	case VerificationNote::Code::MISSING_SUBTITLE:
		return compose("The subtitle asset %1 has no subtitles.", note.note().get());
	case VerificationNote::Code::INVALID_SUBTITLE_ISSUE_DATE:
		return compose("<IssueDate> has an invalid value: %1", note.note().get());
	case VerificationNote::Code::MISMATCHED_SOUND_CHANNEL_COUNTS:
		return compose("The sound assets do not all have the same channel count; the first to differ is %1", filename());
	case VerificationNote::Code::INVALID_MAIN_SOUND_CONFIGURATION:
		return compose("<MainSoundConfiguration> has an invalid value: %1", note.note().get());
	case VerificationNote::Code::MISSING_FONT:
		return compose("The font file for font ID \"%1\" was not found, or was not referred to in the ASSETMAP.", note.note().get());
	case VerificationNote::Code::INVALID_JPEG2000_TILE_PART_SIZE:
		return compose(
			"Frame %1 has an image component that is too large (component %2 is %3 bytes in size).",
			note.frame().get(), note.component().get(), note.size().get()
			);
	case VerificationNote::Code::INCORRECT_SUBTITLE_NAMESPACE_COUNT:
		return compose("The XML in the subtitle asset %1 has more than one namespace declaration.", note.note().get());
	case VerificationNote::Code::MISSING_LOAD_FONT_FOR_FONT:
		return compose("A subtitle or closed caption refers to a font with ID %1 that does not have a corresponding <LoadFont> node", note.id().get());
	case VerificationNote::Code::MISSING_LOAD_FONT:
		return compose("The SMPTE subtitle asset %1 has <Text> nodes but no <LoadFont> node", note.id().get());
	case VerificationNote::Code::MISMATCHED_ASSET_MAP_ID:
		return compose("The asset with ID %1 in the asset map actually has an id of %2", note.id().get(), note.other_id().get());
	case VerificationNote::Code::EMPTY_CONTENT_VERSION_LABEL_TEXT:
		return compose("The <LabelText> in a <ContentVersion> in CPL %1 is empty", note.cpl_id().get());
	case VerificationNote::Code::VALID_CONTENT_VERSION_LABEL_TEXT:
		return compose("CPL has valid <ContentVersion> %1", note.note().get());
	case VerificationNote::Code::INVALID_CPL_NAMESPACE:
		return compose("The namespace %1 in CPL %2 is invalid", note.note().get(), note.cpl_id().get());
	case VerificationNote::Code::MISSING_CPL_CONTENT_VERSION:
		return compose("The CPL %1 has no <ContentVersion> tag", note.cpl_id().get());
	case VerificationNote::Code::INVALID_PKL_NAMESPACE:
		return compose("The namespace %1 in PKL %2 is invalid", note.note().get(), note.file()->filename());
	}

	return "";
}


bool
dcp::operator== (dcp::VerificationNote const& a, dcp::VerificationNote const& b)
{
	return a.type() == b.type() &&
		a.code() == b.code() &&
		a.note() == b.note() &&
		a.file() == b.file() &&
		a.line() == b.line() &&
		a.frame() == b.frame() &&
		a.component() == b.component() &&
		a.size() == b.size() &&
		a.id() == b.id() &&
		a.other_id() == b.other_id() &&
		a.frame_rate() == b.frame_rate() &&
		a.cpl_id() == b.cpl_id() &&
		a.reference_hash() == b.reference_hash() &&
		a.calculated_hash() == b.calculated_hash();
}


bool
dcp::operator!=(dcp::VerificationNote const& a, dcp::VerificationNote const& b)
{
	return !(a == b);
}


bool
dcp::operator< (dcp::VerificationNote const& a, dcp::VerificationNote const& b)
{
	if (a.type() != b.type()) {
		return a.type() < b.type();
	}

	if (a.code() != b.code()) {
		return a.code() < b.code();
	}

	if (a.note() != b.note()) {
		return a.note().get_value_or("") < b.note().get_value_or("");
	}

	if (a.file() != b.file()) {
		return a.file().get_value_or("") < b.file().get_value_or("");
	}

	if (a.line() != b.line()) {
		return a.line().get_value_or(0) < b.line().get_value_or(0);
	}

	if (a.frame() != b.frame()) {
		return a.frame().get_value_or(0) < b.frame().get_value_or(0);
	}

	if (a.component() != b.component()) {
		return a.component().get_value_or(0) < b.component().get_value_or(0);
	}

	if (a.size() != b.size()) {
		return a.size().get_value_or(0) < b.size().get_value_or(0);
	}

	if (a.id() != b.id()) {
		return a.id().get_value_or("") < b.id().get_value_or("");
	}

	if (a.other_id() != b.other_id()) {
		return a.other_id().get_value_or("") < b.other_id().get_value_or("");
	}

	return a.frame_rate().get_value_or(0) != b.frame_rate().get_value_or(0);
}


std::ostream&
dcp::operator<< (std::ostream& s, dcp::VerificationNote const& note)
{
	s << note_to_string (note);
	if (note.note()) {
		s << " [" << note.note().get() << "]";
	}
	if (note.file()) {
		s << " [" << note.file().get() << "]";
	}
	if (note.line()) {
		s << " [" << note.line().get() << "]";
	}
	return s;
}

