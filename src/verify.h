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


/** @file  src/verify.h
 *  @brief dcp::verify() method and associated code
 */


#ifndef LIBDCP_VERIFY_H
#define LIBDCP_VERIFY_H


#include "decrypted_kdm.h"
#include <boost/any.hpp>
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <map>
#include <string>
#include <vector>


/* windows.h defines this but we want to use it */
#undef ERROR


namespace dcp {


class SubtitleAsset;


class VerificationNote
{
public:
	enum class Type {
		ERROR,
		BV21_ERROR, ///< may not always be considered an error, but violates a "shall" requirement of Bv2.1
		WARNING
	};

	/** Codes for errors or warnings from verifying DCPs.
	 *
	 *  The names should (in general) answer the question "what is wrong?" with an answer that begins "There is a ..."
	 *  e.g. "There is a INCORRECT_CPL_HASH"
	 *       "There is a MISSING_ASSET"
	 *
	 *  In general the pattern should be <negative-adjective> <noun>.
	 *  Some <negative-adjective>s are:
	 *
	 *  - INCORRECT: something, which could have any value, is wrong.
	 *  - INVALID: something, which should only be one of a set of values, is not in the set, or some preformatted
	 *             quantity (e.g. XML) is in the wrong format.
	 *  - MISMATCHED: two things, which should be the same, are not.
	 *  - EMPTY: something, which should have a value, has no value.
	 *  - MISSING: something, which should be present, is not.
	 *  - UNEXPECTED: something, which is present, should not be.
	 *  - FAILED: some part of the verification failed in some serious way.
	 *
	 *  Comments should clarify meaning and also say which of the optional fields (e.g. file)
	 *  are filled in when this code is used.
	 */

	// If you change the next line, also look in doc/manual/verifier.py in DCP-o-matic
	// as it looks for it when compiling the manual.  Also, in this enum:
	//   [...]  will be taken as a reference to a section of Bv2.1
	//   _foo_  means foo should be written as a piece of code
	enum class Code {
		/** A general error when reading the DCP
		 *  note contains (probably technical) details
		 */
		FAILED_READ,
		/** The hash of the CPL in the PKL does not agree with the CPL file
		 *  note contains CPL ID
		 *  file contains CPL filename
		 *  calculated_hash contains current hash of the CPL
		 *  reference_hash contains the hash written in the PKL
		 */
		MISMATCHED_CPL_HASHES,
		/** The frame rate given in a reel for the main picture is not 24, 25, 30, 48, 50 or 60
		 *  note contains the invalid frame rate as "<numerator>/<denominator>"
		 */
		INVALID_PICTURE_FRAME_RATE,
		/** The hash of a main picture asset does not agree with the PKL file
		 *  file contains the picture asset filename
		 *  calculated_hash contains the current hash of the picture MXF
		 *  reference_hash contains the hash from the PKL
		 */
		INCORRECT_PICTURE_HASH,
		/** The hash of a main picture is different in the CPL and PKL
		 *  file contains the picture asset filename
		 */
		MISMATCHED_PICTURE_HASHES,
		/** The hash of a main sound asset does not agree with the PKL file
		 *  file contains the sound asset filename
		 *  calculated_hash contains the current hash of the picture MXF
		 *  reference_hash contains the hash from the PKL
		 */
		INCORRECT_SOUND_HASH,
		/** The hash of a main sound is different in the CPL and PKL
		 *  file contains the sound asset filename
		 */
		MISMATCHED_SOUND_HASHES,
		/** An assetmap's _<Path>_ entry is empty */
		EMPTY_ASSET_PATH,
		/** A file mentioned in an asset map cannot be found
		 *  file contains the filename that is missing
		 */
		MISSING_ASSET,
		/** The DCP contains both SMPTE and Interop-standard components */
		MISMATCHED_STANDARD,
		/** Some XML fails to validate against its XSD/DTD
		 *  note contains the (probably technical) details
		 *  file contains the invalid filename
		 *  line contains the line number
		 */
		INVALID_XML,
		/** No _ASSETMAP_ or _ASSETMAP.xml_ was found */
		MISSING_ASSETMAP,
		/** An asset's _IntrinsicDuration_ is less than 1 second
		 *  note contains asset ID
		 */
		INVALID_INTRINSIC_DURATION,
		/** An asset's _Duration_ is less than 1 second
		 *  note contains asset ID
		 */
		INVALID_DURATION,
		/** The JPEG2000 data in at least one picture frame is larger than the equivalent of 250Mbit/s
		 *  file contains the picture asset filename
		 */
		INVALID_PICTURE_FRAME_SIZE_IN_BYTES,
		/** The JPEG2000 data in at least one picture frame is larger than the equivalent of 230Mbit/s
		 *  file contains the picture asset filename
		 */
		NEARLY_INVALID_PICTURE_FRAME_SIZE_IN_BYTES,
		/** An asset that the CPL requires is not in this DCP; the DCP may be a VF
		 *  note contains the asset ID
		 */
		EXTERNAL_ASSET,
		/** A stereoscopic asset has an MXF which is marked as being monoscopic
		 *  file contains the asset filename
		 */
		THREED_ASSET_MARKED_AS_TWOD,
		/** DCP is Interop, not SMPTE [Bv2.1_6.1] */
		INVALID_STANDARD,
		/** A language or territory does not conform to RFC 5646 [Bv2.1_6.2.1]
		 *  note contains the invalid language
		 */
		INVALID_LANGUAGE,
		/** A picture asset does not have one of the required Bv2.1 sizes (in pixels) [Bv2.1_7.1]
		 *  note contains the incorrect size as "<width>x<height>"
		 *  file contains the asset filename
		 */
		INVALID_PICTURE_SIZE_IN_PIXELS,
		/** A picture asset is 2K but is not at 24, 25 or 48 fps as required by Bv2.1 [Bv2.1_7.1]
		 *  note contains the invalid frame rate as "<numerator>/<denominator>"
		 *  file contains the asset filename
		 */
		INVALID_PICTURE_FRAME_RATE_FOR_2K,
		/** A picture asset is 4K but is not at 24fps as required by Bv2.1 [Bv2.1_7.1]
		 *  note contains the invalid frame rate as "<numerator>/<denominator>"
		 *  file contains the asset filename
		 */
		INVALID_PICTURE_FRAME_RATE_FOR_4K,
		/** A picture asset is 4K but is 3D which is not allowed by Bv2.1 [Bv2.1_7.1]
		 *  note contains the invalid frame rate as "<numerator>/<denominator>"
		 *  file contains the asset filename
		 */
		INVALID_PICTURE_ASSET_RESOLUTION_FOR_3D,
		/** A closed caption's XML file is larger than 256KB [Bv2.1_7.2.1]
		 *  note contains the invalid size in bytes
		 *  file contains the asset filename
		 */
		INVALID_CLOSED_CAPTION_XML_SIZE_IN_BYTES,
		/** Any timed text asset's total files is larger than 115MB [Bv2.1_7.2.1]
		 *  note contains the invalid size in bytes
		 *  file contains the asset filename
		 */
		INVALID_TIMED_TEXT_SIZE_IN_BYTES,
		/** The total size of all a timed text asset's fonts is larger than 10MB [Bv2.1_7.2.1]
		 *  note contains the invalid size in bytes
		 *  file contains the asset filename
		 */
		INVALID_TIMED_TEXT_FONT_SIZE_IN_BYTES,
		/** Some SMPTE subtitle XML has no _<Language>_ tag [Bv2.1_7.2.2]
		 *  file contains the asset filename
		 */
		MISSING_SUBTITLE_LANGUAGE,
		/** Not all subtitle assets specify the same _<Language>_ tag [Bv2.1_7.2.2] */
		MISMATCHED_SUBTITLE_LANGUAGES,
		/** Some SMPTE subtitle XML has no _<StartTime>_ tag [Bv2.1_7.2.3]
		 *  file contains the asset filename
		 */
		MISSING_SUBTITLE_START_TIME,
		/** Some SMPTE subtitle XML has a non-zero _<StartTime>_ tag [Bv2.1_7.2.3]
		 *  file contains the asset filename
		 */
		INVALID_SUBTITLE_START_TIME,
		/** The first subtitle or closed caption happens before 4s into the first reel [Bv2.1_7.2.4] */
		INVALID_SUBTITLE_FIRST_TEXT_TIME,
		/** At least one subtitle is less than the minimum of 15 frames suggested by [Bv2.1_7.2.5] */
		INVALID_SUBTITLE_DURATION,
		/** At least one pair of subtitles are separated by less than the the minimum of 2 frames suggested by [Bv2.1_7.2.5] */
		INVALID_SUBTITLE_SPACING,
		/** A subtitle lasts for longer than the reel which contains it */
		SUBTITLE_OVERLAPS_REEL_BOUNDARY,
		/** There are more than 3 subtitle lines in at least one place [Bv2.1_7.2.7] */
		INVALID_SUBTITLE_LINE_COUNT,
		/** There are more than 52 characters in at least one subtitle line [Bv2.1_7.2.7] */
		NEARLY_INVALID_SUBTITLE_LINE_LENGTH,
		/** There are more than 79 characters in at least one subtitle line [Bv2.1_7.2.7] */
		INVALID_SUBTITLE_LINE_LENGTH,
		/** There are more than 3 closed caption lines in at least one place [Bv2.1_7.2.6] */
		INVALID_CLOSED_CAPTION_LINE_COUNT,
		/** There are more than 32 characters in at least one closed caption line [Bv2.1_7.2.6] */
		INVALID_CLOSED_CAPTION_LINE_LENGTH,
		/** The audio sampling rate must be 48kHz [Bv2.1_7.3]
		 *  note contains the invalid frame rate
		 *  file contains the asset filename
		 */
		INVALID_SOUND_FRAME_RATE,
		/** The CPL has no _<AnnotationText>_ tag [Bv2.1_8.1]
		 *  note contains the CPL ID
		 *  file contains the CPL filename
		 */
		MISSING_CPL_ANNOTATION_TEXT,
		/** The _<AnnotationText>_ is not the same as the _<ContentTitleText>_ [Bv2.1_8.1]
		 *  note contains the CPL ID
		 *  file contains the CPL filename
		 */
		MISMATCHED_CPL_ANNOTATION_TEXT,
		/** At least one asset in a reel does not have the same duration as the others */
		MISMATCHED_ASSET_DURATION,
		/** If one reel has a _MainSubtitle_, all must have them */
		MISSING_MAIN_SUBTITLE_FROM_SOME_REELS,
		/** If one reel has at least one _ClosedCaption_, all reels must have the same number of _ClosedCaptions_ */
		MISMATCHED_CLOSED_CAPTION_ASSET_COUNTS,
		/** MainSubtitle in reels must have _<EntryPoint>_ [Bv2.1_8.3.2]
		 *  note contains the asset ID
		 */
		MISSING_SUBTITLE_ENTRY_POINT,
		/** MainSubtitle _<EntryPoint>_ must be zero [Bv2.1_8.3.2]
		 *  note contains the asset ID
		 */
		INCORRECT_SUBTITLE_ENTRY_POINT,
		/** Closed caption in reels must have _<EntryPoint>_ [Bv2.1_8.3.2]
		 *  note contains the asset ID
		 */
		MISSING_CLOSED_CAPTION_ENTRY_POINT,
		/** Closed caption _MainSubtitle_ _<EntryPoint>_ must be zero [Bv2.1_8.3.2]
		 *  note contains the asset ID
		 */
		INCORRECT_CLOSED_CAPTION_ENTRY_POINT,
		/** _<Hash>_ must be present for assets in CPLs
		 * note contains the asset ID
		 */
		MISSING_HASH,
		/** If _ContentKind_ is Feature there must be a FFEC marker */
		MISSING_FFEC_IN_FEATURE,
		/** If _ContentKind_ is Feature there must be a FFMC marker */
		MISSING_FFMC_IN_FEATURE,
		/** There should be a FFOC marker */
		MISSING_FFOC,
		/** There should be a LFOC marker */
		MISSING_LFOC,
		/** The FFOC marker should be 1
		 *  note contains the incorrect value.
		 */
		INCORRECT_FFOC,
		/** The LFOC marker should be the last frame in the reel
		 *  note contains the incorrect value
		 */
		INCORRECT_LFOC,
		/** There must be a _<CompositionMetadataAsset>_
		 *  note contains the CPL ID
		 *  file contains the CPL filename
		 */
		MISSING_CPL_METADATA,
		/** CPL metadata should contain _<VersionNumber>_ of 1, at least
		 *  note contains the CPL ID
		 *  file contains the CPL filename
		 */
		MISSING_CPL_METADATA_VERSION_NUMBER,
		/** There must be an _<ExtensionMetadata>_ in _<CompositionMetadataAsset>_ [Bv2.1_8.6.3]
		 *  note contains the CPL ID
		 *  file contains the CPL filename
		 */
		MISSING_EXTENSION_METADATA,
		/** _<ExtensionMetadata>_ does not have the correct form [Bv2.1_8.6.3]
		 *  note contains details of what's wrong
		 *  file contains the CPL filename
		 */
		INVALID_EXTENSION_METADATA,
		/** A CPL containing encrypted content is not signed [Bv2.1_8.7]
		 *  note contains the CPL ID
		 *  file contains the CPL filename
		 */
		UNSIGNED_CPL_WITH_ENCRYPTED_CONTENT,
		/** A PKL containing encrypted content is not signed [Bv2.1_8.7]
		 *  note contains the PKL ID
		 *  file contains the PKL filename
		 */
		UNSIGNED_PKL_WITH_ENCRYPTED_CONTENT,
		/** If a PKL has one CPL its _<ContentTitleText>_ must be the same as the PKL's _<AnnotationText>_
		 *  note contains the PKL ID
		 *  file contains the PKL filename
		 */
		MISMATCHED_PKL_ANNOTATION_TEXT_WITH_CPL,
		/** Some, but not all content, is encrypted */
		PARTIALLY_ENCRYPTED,
		/** General error during JPEG2000 codestream verification
		 *  frame contains the frame index (counted from 0)
		 *  note contains details
		 */
		INVALID_JPEG2000_CODESTREAM,
		/** Invalid number of guard bits in a 2K JPEG2000 stream (should be 1) [Bv2.1_10.2.1]
		 *  note contains the number of guard bits
		 */
		INVALID_JPEG2000_GUARD_BITS_FOR_2K,
		/** Invalid number of guard bits in a 4K JPEG2000 stream (should be 2) [Bv2.1_10.2.1]
		 *  note contains the number of guard bits
		 */
		INVALID_JPEG2000_GUARD_BITS_FOR_4K,
		/** JPEG2000 tile size is not the same as the image size [Bv2.1_10.2.1] */
		INVALID_JPEG2000_TILE_SIZE,
		/** JPEG2000 code block width is not 32 [Bv2.1_10.2.1]
		 *  note contains the code block width
		 */
		INVALID_JPEG2000_CODE_BLOCK_WIDTH,
		/** JPEG2000 code block height is not 32 [Bv2.1_10.2.1]
		 *  note contains the code block height
		 */
		INVALID_JPEG2000_CODE_BLOCK_HEIGHT,
		/** There must be no POC markers in a 2K codestream [Bv2.1_10.2.1]
		 *  note contains the number of POC markers found
		 */
		INCORRECT_JPEG2000_POC_MARKER_COUNT_FOR_2K,
		/** There must be exactly one POC marker in a 4K codestream [Bv2.1_10.2.1]
		 *  note contains the number of POC markers found
		 */
		INCORRECT_JPEG2000_POC_MARKER_COUNT_FOR_4K,
		/** A POC marker has incorrect content [Bv2.1_10.2.1]
		 *  note contains details
		 */
		INCORRECT_JPEG2000_POC_MARKER,
		/** A POC marker was found outside the main head [Bv2.1_10.2.1] */
		INVALID_JPEG2000_POC_MARKER_LOCATION,
		/** Invalid number of tile parts for 2K JPEG2000 stream (should be 3) [Bv2.1_10.2.1]
		 *  note contains the number of tile parts
		 */
		INVALID_JPEG2000_TILE_PARTS_FOR_2K,
		/** Invalid number of tile parts for 4K JPEG2000 stream (should be 6) [Bv2.1_10.2.1]
		 *  note contains the number of tile parts
		 */
		INVALID_JPEG2000_TILE_PARTS_FOR_4K,
		/** No TLM marker was found [Bv2.1_10.2.1] */
		MISSING_JPEG200_TLM_MARKER,
		/** The MXF _ResourceID_ of a timed text resource was not the same as that of the contained XML essence [Bv2.1_10.4.3] */
		MISMATCHED_TIMED_TEXT_RESOURCE_ID,
		/** The AssetID of a timed text MXF is the same as its _ResourceID_ or that of the contained XML essence [Bv2.1_10.4.2] */
		INCORRECT_TIMED_TEXT_ASSET_ID,
		/** The ContainerDuration of a timed text MXF is not the same as the _Duration_ in its reel [Bv2.1_10.4.3]
		 *  note contains the reel duration, followed by a space, followed by the MXF duration
		 *  file contains the asset filename
		 */
		MISMATCHED_TIMED_TEXT_DURATION,
		/** Something could not be verified because content is encrypted and no key is available */
		MISSED_CHECK_OF_ENCRYPTED,
		/** Some timed-text XML has an empty <_Text_> node */
		EMPTY_TEXT,
		/** Some closed captions do not have the same vertical alignment within a <_Subtitle_> node */
		MISMATCHED_CLOSED_CAPTION_VALIGN,
		/** Some closed captions are not listed in the XML in the order of their vertical position */
		INCORRECT_CLOSED_CAPTION_ORDERING,
		/** Some <MainMarkers> asset has an <EntryPoint> that should not be there */
		UNEXPECTED_ENTRY_POINT,
		/** Some <MainMarkers> asset has an <Duration> that should not be there */
		UNEXPECTED_DURATION,
		/** A <ContentKind> has been specified with either no scope or the SMPTE 429-7 scope, but which is not one of those allowed */
		INVALID_CONTENT_KIND,
		/** Either the width or height of a <MainPictureActiveArea> in a CPL is either not an even number, or bigger than the corresponding asset dimension.
		 *  note contains details of what is wrong
		 *  file contains the CPL filename
		 */
		INVALID_MAIN_PICTURE_ACTIVE_AREA,
		/** A PKL has more than one asset with the same ID
		 *  note contains the PKL ID
		 *  file contains the PKL filename
		 */
		DUPLICATE_ASSET_ID_IN_PKL,
		/** An ASSETMAP has more than one asset with the same ID
		 *  note contains the ASSETMAP ID
		 *  file contains the ASSETMAP filename
		 */
		DUPLICATE_ASSET_ID_IN_ASSETMAP,
		/** An Interop subtitle asset has no subtitles
		 *  note contains the asset ID
		 *  file contains the asset filename
		 */
		MISSING_SUBTITLE,
		/** A SMPTE subtitle asset as an <IssueDate> which is not of the form yyyy-mm-ddThh:mm:ss
		 *  I can find no reference in a standard to this being required, but the Deluxe delivery
		 *  specifications require it and their QC will fail DCPs that don't have it.
		 *  note contains the incorrect <IssueDate>
		 */
		INVALID_SUBTITLE_ISSUE_DATE,
		/** The sound assets in the CPL do not have the same audio channel count.
		 *  file contains the filename of the first asset to differ
		 */
		MISMATCHED_SOUND_CHANNEL_COUNTS,
		/** The CPL contains a MainSoundConfiguration tag which does not describe the number of
		 *  channels in the audio assets.
		 *  note contains details of what is wrong
		 *  file contains the CPL filename
		 */
		INVALID_MAIN_SOUND_CONFIGURATION,
		/** An interop subtitle file has a <LoadFont> node which refers to a font file that is not found.
		 *  note contains the <LoadFont> ID
		 */
		MISSING_FONT,
		/** A tile part in a JPEG2000 frame is too big.
		 *  frame contains the frame index (counted from 0)
		 *  component contains the component index (0, 1 or 2)
		 *  size contains the invalid size in bytes.
		 */
		INVALID_JPEG2000_TILE_PART_SIZE,
		/** A subtitle XML root node has more than one namespace (xmlns) declaration.
		 *  note contains the asset ID
		 */
		INCORRECT_SUBTITLE_NAMESPACE_COUNT,
		/** A subtitle or closed caption file has a <Font> tag which refers to a font that is not
		 *  first introduced with a <LoadFont>.
		 *  id contains the ID of the <Font> tag.
		 */
		MISSING_LOAD_FONT_FOR_FONT,
		/** A SMPTE subtitle asset has at least one <Text> element but no <LoadFont>
		 *  id contains the ID of the subtitle asset.
		 */
		MISSING_LOAD_FONT,
		/** An ID in an asset map does not match the ID obtained from reading the actual file.
		 *  id contains the ID from the asset map.
		 *  other_id contains the ID from the file.
		 */
		MISMATCHED_ASSET_MAP_ID,
		/** The <LabelText> inside a <ContentVersion> is empty
		 *  note contains the CPL ID
		 *  file contains the CPL filename
		 */
		EMPTY_CONTENT_VERSION_LABEL_TEXT,
		/** The CPL namespace is not valid.
		 *  note contains the invalid namespace
		 *  file contains the CPL filename
		 */
		INVALID_CPL_NAMESPACE,
		/** A SMPTE CPL does not contain a _<ContentVersion>_ tag
		 *  note contains the CPL ID
		 *  file contains the CPL filename
		 */
		MISSING_CPL_CONTENT_VERSION,
		/** The PKL namespace is not valid.
		 *  note contains the invalid namespace
		 *  file contains the PKL filename
		 */
		INVALID_PKL_NAMESPACE
	};

	VerificationNote (Type type, Code code)
		: _type (type)
		, _code (code)
	{}

	VerificationNote (Type type, Code code, std::string note)
		: _type (type)
		, _code (code)
	{
		_data[Data::NOTE] = note;
	}

	VerificationNote (Type type, Code code, boost::filesystem::path file)
		: _type (type)
		, _code (code)
	{
		_data[Data::FILE] = file;
	}

	VerificationNote (Type type, Code code, std::string note, boost::filesystem::path file)
		: _type (type)
		, _code (code)
	{
		_data[Data::NOTE] = note;
		_data[Data::FILE] = file;
	}

	VerificationNote (Type type, Code code, std::string note, boost::filesystem::path file, uint64_t line)
		: _type (type)
		, _code (code)
	{
		_data[Data::NOTE] = note;
		_data[Data::FILE] = file;
		_data[Data::LINE] = line;
	}

	Type type () const {
		return _type;
	}

	Code code () const {
		return _code;
	}

private:
	enum class Data {
		NOTE,  ///< further information about the error
		FILE,  ///< path of file containing the error
		LINE,  ///< error line number within the FILE
		FRAME,
		COMPONENT,
		SIZE,
		ID,
		OTHER_ID,
		FRAME_RATE,
		CALCULATED_HASH,
		REFERENCE_HASH
	};

	template <class T>
	boost::optional<T> data(Data key) const
	{
		auto iter = _data.find(key);
		if (iter == _data.end()) {
			return {};
		}
		return boost::any_cast<T>(iter->second);
	}

public:
	boost::optional<std::string> note () const {
		return data<std::string>(Data::NOTE);
	}

	boost::optional<boost::filesystem::path> file () const {
		return data<boost::filesystem::path>(Data::FILE);
	}

	boost::optional<uint64_t> line () const {
		return data<uint64_t>(Data::LINE);
	}

	VerificationNote& set_frame(int frame) {
		_data[Data::FRAME] = frame;
		return *this;
	}

	boost::optional<int> frame() const {
		return data<int>(Data::FRAME);
	}

	VerificationNote& set_component(int component) {
		_data[Data::COMPONENT] = component;
		return *this;
	}

	boost::optional<int> component() const {
		return data<int>(Data::COMPONENT);
	}

	VerificationNote& set_size(int size) {
		_data[Data::SIZE] = size;
		return *this;
	}

	boost::optional<int> size() const {
		return data<int>(Data::SIZE);
	}

	VerificationNote& set_id(std::string id) {
		_data[Data::ID] = id;
		return *this;
	}

	boost::optional<std::string> id() const {
		return data<std::string>(Data::ID);
	}

	VerificationNote& set_other_id(std::string other_id) {
		_data[Data::OTHER_ID] = other_id;
		return *this;
	}

	boost::optional<std::string> other_id() const {
		return data<std::string>(Data::OTHER_ID);
	}

	VerificationNote& set_frame_rate(int frame_rate) {
		_data[Data::FRAME_RATE] = frame_rate;
		return *this;
	}

	boost::optional<int> frame_rate() const {
		return data<int>(Data::FRAME_RATE);
	}

	VerificationNote& set_calculated_hash(std::string hash) {
		_data[Data::CALCULATED_HASH] = hash;
		return *this;
	}

	boost::optional<std::string> calculated_hash() const {
		return data<std::string>(Data::CALCULATED_HASH);
	}

	VerificationNote& set_reference_hash(std::string hash) {
		_data[Data::REFERENCE_HASH] = hash;
		return *this;
	}

	boost::optional<std::string> reference_hash() const {
		return data<std::string>(Data::REFERENCE_HASH);
	}

private:
	Type _type;
	Code _code;
	std::map<Data, boost::any> _data;
};


struct VerificationOptions
{
	///< If set, any assets larger than this number of bytes will not have their hashes checked
	boost::optional<boost::uintmax_t> maximum_asset_size_for_hash_check;
	///< true to check asset hashes (except those which match maximum_asset_size_for_hash_check)
	///< false to check no asset hashes.
	bool check_asset_hashes = true;
};


std::vector<VerificationNote> verify (
	std::vector<boost::filesystem::path> directories,
	std::vector<dcp::DecryptedKDM> kdms,
	boost::function<void (std::string, boost::optional<boost::filesystem::path>)> stage,
	boost::function<void (float)> progress,
	VerificationOptions options = {},
	boost::optional<boost::filesystem::path> xsd_dtd_directory = boost::optional<boost::filesystem::path>()
	);

std::string note_to_string (dcp::VerificationNote note);

bool operator== (dcp::VerificationNote const& a, dcp::VerificationNote const& b);
bool operator< (dcp::VerificationNote const& a, dcp::VerificationNote const& b);

std::ostream& operator<<(std::ostream& s, dcp::VerificationNote const& note);


struct LinesCharactersResult
{
	bool warning_length_exceeded = false;
	bool error_length_exceeded = false;
	bool line_count_exceeded = false;
};


extern void verify_text_lines_and_characters(
	std::shared_ptr<const dcp::SubtitleAsset> asset,
	int warning_length,
	int error_length,
	dcp::LinesCharactersResult* result
	);

}


#endif
