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


#include "dcp_time.h"
#include "decrypted_kdm.h"
#include "types.h"
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


class DCP;
class TextAsset;


class VerificationNote
{
public:
	enum class Type {
		OK,
		ERROR,
		BV21_ERROR, ///< may not always be considered an error, but violates a "shall" requirement of Bv2.1
		WARNING
	};

	/** Codes for successful checks, errors or warnings from verifying DCPs.
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
	//   Only the first line of each comment will be taken as a description of the code.
	enum class Code {
		/** A general error when reading the DCP
		 *  error contains (probably technical) details
		 */
		FAILED_READ,
		/** The hash of a CPL in the PKL agrees with the CPL file */
		MATCHING_CPL_HASHES,
		/** The hash of the CPL in the PKL does not agree with the CPL file
		 *  note contains CPL ID
		 *  file contains CPL filename
		 *  calculated_hash contains current hash of the CPL
		 *  reference_hash contains the hash written in the PKL
		 */
		MISMATCHED_CPL_HASHES,
		/** The frame rate given in a reel for the main picture is not 24, 25, 30, 48, 50 or 60
		 *  frame_rate contains the invalid frame rate
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_PICTURE_FRAME_RATE,
		/** The hash of a main picture asset agrees with the PKL file.
		 *  reel_index contains the reel index (starting from 0)
		 */
		CORRECT_PICTURE_HASH,
		/** The hash of a main picture asset does not agree with the PKL file
		 *  file contains the picture asset filename
		 *  calculated_hash contains the current hash of the picture MXF
		 *  reference_hash contains the hash from the PKL
		 *  reel_index contains the reel index (starting from 0)
		 */
		INCORRECT_PICTURE_HASH,
		/** The hash of a main picture is different in the CPL and PKL
		 *  file contains the picture asset filename
		 *  reel_index contains the reel index (starting from 0)
		 */
		MISMATCHED_PICTURE_HASHES,
		/** The hash of a main sound asset does not agree with the PKL file
		 *  file contains the sound asset filename
		 *  calculated_hash contains the current hash of the picture MXF
		 *  reference_hash contains the hash from the PKL
		 *  reel_index contains the reel index (starting from 0)
		 */
		INCORRECT_SOUND_HASH,
		/** The hash of a main sound is different in the CPL and PKL
		 *  file contains the sound asset filename
		 *  reel_index contains the reel index (starting from 0)
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
		 *  error contains the (probably technical) details
		 *  file contains the invalid filename
		 *  line contains the line number
		 *  reel_index contains the reel index (starting from 0), if applicable
		 */
		INVALID_XML,
		/** No _ASSETMAP_ or _ASSETMAP.xml_ was found */
		MISSING_ASSETMAP,
		/** An asset's _IntrinsicDuration_ is less than 1 second
		 *  asset_id contains asset ID
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_INTRINSIC_DURATION,
		/** An asset's _Duration_ is less than 1 second
		 *  asset_id contains asset ID
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_DURATION,
		/** The JPEG2000 data in all picture frames of an asset is smaller than the maximum size.
		 *  reel_index contains the reel index (starting from 0)
		 */
		VALID_PICTURE_FRAME_SIZES_IN_BYTES,
		/** The JPEG2000 data in at least one picture frame is larger than the equivalent of 250Mbit/s
		 *  file contains the picture asset filename
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_PICTURE_FRAME_SIZE_IN_BYTES,
		/** The JPEG2000 data in at least one picture frame is larger than the equivalent of 230Mbit/s
		 *  file contains the picture asset filename
		 *  reel_index contains the reel index (starting from 0)
		 */
		NEARLY_INVALID_PICTURE_FRAME_SIZE_IN_BYTES,
		/** An asset that the CPL requires is not in this DCP; the DCP may be a VF
		 *  asset_id contains the asset ID
		 */
		EXTERNAL_ASSET,
		/** A stereoscopic asset has an MXF which is marked as being monoscopic
		 *  file contains the asset filename
		 */
		THREED_ASSET_MARKED_AS_TWOD,
		/** DCP is Interop, not SMPTE [Bv2.1_6.1] */
		INVALID_STANDARD,
		/** A language or territory does not conform to RFC 5646 [Bv2.1_6.2.1]
		 *  language contains the invalid language
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_LANGUAGE,
		/** A CPL has a valid release territory
		 *  territory contains the territory
		 */
		VALID_RELEASE_TERRITORY,
		/** A picture asset does not have one of the required Bv2.1 sizes (in pixels) [Bv2.1_7.1]
		 *  size_in_pixels contains the incorrect size
		 *  file contains the asset filename
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_PICTURE_SIZE_IN_PIXELS,
		/** A picture asset is 2K but is not at 24, 25 or 48 fps as required by Bv2.1 [Bv2.1_7.1]
		 *  frame_rate contains the invalid frame rate
		 *  file contains the asset filename
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_PICTURE_FRAME_RATE_FOR_2K,
		/** A picture asset is 4K but is not at 24fps as required by Bv2.1 [Bv2.1_7.1]
		 *  frame_rate contains the invalid frame rate
		 *  file contains the asset filename
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_PICTURE_FRAME_RATE_FOR_4K,
		/** A picture asset is 4K but is 3D which is not allowed by Bv2.1 [Bv2.1_7.1]
		 *  file contains the asset filename
		 */
		INVALID_PICTURE_ASSET_RESOLUTION_FOR_3D,
		/** A closed caption's XML file is larger than 256KB [Bv2.1_7.2.1]
		 *  size_in_bytes contains the invalid size in bytes
		 *  file contains the asset filename
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_CLOSED_CAPTION_XML_SIZE_IN_BYTES,
		/** Any timed text asset's total files is larger than 115MB [Bv2.1_7.2.1]
		 *  size_in_bytes contains the invalid size in bytes
		 *  file contains the asset filename
		 */
		INVALID_TIMED_TEXT_SIZE_IN_BYTES,
		/** The total size of all a timed text asset's fonts is larger than 10MB [Bv2.1_7.2.1]
		 *  size_in_bytes contains the invalid size in bytes
		 *  file contains the asset filename
		 */
		INVALID_TIMED_TEXT_FONT_SIZE_IN_BYTES,
		/** Some SMPTE subtitle XML has no _<Language>_ tag [Bv2.1_7.2.2]
		 *  file contains the asset filename
		 *  reel_index contains the reel index (starting from 0)
		 */
		MISSING_SUBTITLE_LANGUAGE,
		/** Not all subtitle assets specify the same _<Language>_ tag [Bv2.1_7.2.2]
		 *  reel_index contains the reel index (starting from 0)
		 */
		MISMATCHED_SUBTITLE_LANGUAGES,
		/** Some SMPTE subtitle XML has no _<StartTime>_ tag [Bv2.1_7.2.3]
		 *  file contains the asset filename
		 *  reel_index contains the reel index (starting from 0)
		 */
		MISSING_SUBTITLE_START_TIME,
		/** Some SMPTE subtitle XML has a non-zero _<StartTime>_ tag [Bv2.1_7.2.3]
		 *  file contains the asset filename
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_SUBTITLE_START_TIME,
		/** The first subtitle or closed caption happens before 4s into the first reel [Bv2.1_7.2.4] */
		INVALID_SUBTITLE_FIRST_TEXT_TIME,
		/** At least one subtitle has a zero or negative duration */
		INVALID_SUBTITLE_DURATION,
		/** At least one subtitle is less than the minimum of 15 frames suggested by [Bv2.1_7.2.5] */
		INVALID_SUBTITLE_DURATION_BV21,
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
		 *  frame_rate contains the invalid frame rate
		 *  file contains the asset filename
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_SOUND_FRAME_RATE,
		/** The audio bit depth must be 24
		 *  bit_depth contains the invalid bit depth
		 *  file contains the asset filename
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_SOUND_BIT_DEPTH,
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
		/** A CPL has an annotation text which matches the _<ContentTitleText>_ */
		VALID_CPL_ANNOTATION_TEXT,
		/** At least one asset in a reel does not have the same duration as the others.
		 *  reel_index contains the reel index (starting from 0)
		 */
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
		 *  note contains the asset ID
		 *  reel_index contains the reel index (starting from 0)
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
		/** A PKL _<AnnotationText>_ matches a CPL _<ContentTitleText>_ */
		MATCHING_PKL_ANNOTATION_TEXT_WITH_CPL,
		/** All content is encrypted */
		ALL_ENCRYPTED,
		/** No content is encrypted */
		NONE_ENCRYPTED,
		/** Some, but not all content, is encrypted */
		PARTIALLY_ENCRYPTED,
		/** General error during JPEG2000 codestream verification
		 *  frame contains the frame index (counted from 0)
		 *  note contains details
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_JPEG2000_CODESTREAM,
		/** Invalid number of guard bits in a 2K JPEG2000 stream (should be 1) [Bv2.1_10.2.1]
		 *  note contains the number of guard bits
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_JPEG2000_GUARD_BITS_FOR_2K,
		/** Invalid number of guard bits in a 4K JPEG2000 stream (should be 2) [Bv2.1_10.2.1]
		 *  note contains the number of guard bits
		 *  reel_index contains the reel index (starting from 0)
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
		/** Invalid _Rsiz_ (capabilities) value in 2K JPEG2000 stream */
		INVALID_JPEG2000_RSIZ_FOR_2K,
		/** Invalid _Rsiz_ (capabilities) value in 4K JPEG2000 stream */
		INVALID_JPEG2000_RSIZ_FOR_4K,
		/** No TLM marker was found [Bv2.1_10.2.1] */
		MISSING_JPEG2000_TLM_MARKER,
		/** The MXF _ResourceID_ of a timed text resource was not the same as that of the contained XML essence [Bv2.1_10.4.3]
		 *  reel_index contains the reel index (starting from 0)
		 */
		MISMATCHED_TIMED_TEXT_RESOURCE_ID,
		/** The AssetID of a timed text MXF is the same as its _ResourceID_ or that of the contained XML essence [Bv2.1_10.4.2]
		 *  reel_index contains the reel index (starting from 0)
		 */
		INCORRECT_TIMED_TEXT_ASSET_ID,
		/** The ContainerDuration of a timed text MXF is not the same as the _Duration_ in its reel [Bv2.1_10.4.3]
		 *  note contains the reel duration, followed by a space, followed by the MXF duration
		 *  file contains the asset filename
		 *  reel_index contains the reel index (starting from 0)
		 */
		MISMATCHED_TIMED_TEXT_DURATION,
		/** Something could not be verified because content is encrypted and no key is available
		 *  reel_index contains the reel index (starting from 0)
		 */
		MISSED_CHECK_OF_ENCRYPTED,
		/** Some timed-text XML has an empty _<Text>_ node */
		EMPTY_TEXT,
		/** Some closed captions do not have the same vertical alignment within a _<Subtitle>_ node */
		MISMATCHED_CLOSED_CAPTION_VALIGN,
		/** Some closed captions are not listed in the XML in the order of their vertical position */
		INCORRECT_CLOSED_CAPTION_ORDERING,
		/** Some _<MainMarkers>_ asset has an _<EntryPoint>_ that should not be there
		 *  reel_index contains the reel index (starting from 0)
		 */
		UNEXPECTED_ENTRY_POINT,
		/** Some _<MainMarkers>_ asset has an _<Duration>_ that should not be there
		 *  reel_index contains the reel index (starting from 0)
		 */
		UNEXPECTED_DURATION,
		/** A _<ContentKind>_ has been specified with either no scope or the SMPTE 429-7 scope, but which is not one of those allowed */
		INVALID_CONTENT_KIND,
		/** A valid _<ContentKind>_ was seen */
		VALID_CONTENT_KIND,
		/** Either the width or height of a _<MainPictureActiveArea>_ in a CPL is either not an even number, or bigger than the corresponding asset dimension
		 *  note contains details of what is wrong
		 *  file contains the CPL filename
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_MAIN_PICTURE_ACTIVE_AREA,
		/** A valid _<MainPictureActiveArea>_ was seen */
		VALID_MAIN_PICTURE_ACTIVE_AREA,
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
		 *  reel_index contains the reel index (starting from 0)
		 */
		MISSING_SUBTITLE,
		/** A SMPTE subtitle asset as an _<IssueDate>_ which is not of the form yyyy-mm-ddThh:mm:ss
		 *  I can find no reference in a standard to this being required, but the Deluxe delivery
		 *  specifications require it and their QC will fail DCPs that don't have it.
		 *  note contains the incorrect <IssueDate>
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_SUBTITLE_ISSUE_DATE,
		/** The sound assets in the CPL do not have the same audio channel count
		 *  file contains the filename of the first asset to differ
		 *  reel_index contains the reel index (starting from 0)
		 */
		MISMATCHED_SOUND_CHANNEL_COUNTS,
		/** The CPL contains a _<MainSoundConfiguration>_ tag which does not describe the number of channels in the audio assets, or which is in some way badly formatted
		 *  note contains details of what is wrong
		 *  file contains the CPL filename
		 *  cpl_id contains the CPL ID
		 */
		INVALID_MAIN_SOUND_CONFIGURATION,
		/** An interop subtitle file has a _<LoadFont>_ node which refers to a font file that is not found
		 *  note contains the <LoadFont> ID
		 *  reel_index contains the reel index (starting from 0)
		 */
		MISSING_FONT,
		/** A tile part in a JPEG2000 frame is too big
		 *  frame contains the frame index (counted from 0)
		 *  component contains the component index (0, 1 or 2)
		 *  size contains the invalid size in bytes.
		 *  reel_index contains the reel index (starting from 0)
		 */
		INVALID_JPEG2000_TILE_PART_SIZE,
		/** A subtitle XML root node has more than one namespace (xmlns) declaration.
		 *  note contains the asset ID
		 *  reel_index contains the reel index (starting from 0)
		 */
		INCORRECT_SUBTITLE_NAMESPACE_COUNT,
		/** A subtitle or closed caption file has a _<Font>_ tag which refers to a font that is not first introduced with a _<LoadFont>_
		 *  id contains the ID of the <Font> tag.
		 */
		MISSING_LOAD_FONT_FOR_FONT,
		/** A SMPTE subtitle asset has at least one _<Text>_ element but no <LoadFont>
		 *  id contains the ID of the subtitle asset.
		 *  reel_index contains the reel index (starting from 0)
		 */
		MISSING_LOAD_FONT,
		/** An ID in an asset map does not match the ID obtained from reading the actual file
		 *  id contains the ID from the asset map.
		 *  other_id contains the ID from the file.
		 */
		MISMATCHED_ASSET_MAP_ID,
		/** The <LabelText> inside a _<ContentVersion>_ is empty
		 *  note contains the CPL ID
		 *  file contains the CPL filename
		 */
		EMPTY_CONTENT_VERSION_LABEL_TEXT,
		/** A <LabelText> inside a _<ContentVersion>_ is valid */
		VALID_CONTENT_VERSION_LABEL_TEXT,
		/** The CPL namespace is not valid
		 *  note contains the invalid namespace
		 *  file contains the CPL filename
		 */
		INVALID_CPL_NAMESPACE,
		/** A SMPTE CPL does not contain a _<ContentVersion>_ tag
		 *  file contains the CPL filename
		 */
		MISSING_CPL_CONTENT_VERSION,
		/** The PKL namespace is not valid
		 *  note contains the invalid namespace
		 *  file contains the PKL filename
		 */
		INVALID_PKL_NAMESPACE,
	};

	VerificationNote(Code code)
		: _code(code)
	{}

	VerificationNote(Code code, std::string note)
		: _code(code)
	{
		_data[Data::NOTE] = note;
	}

	VerificationNote(Code code, boost::filesystem::path file)
		: _code(code)
	{
		_data[Data::FILE] = file;
	}

	VerificationNote(Code code, std::string note, boost::filesystem::path file)
		: _code(code)
	{
		_data[Data::NOTE] = note;
		_data[Data::FILE] = file;
	}

	VerificationNote(Code code, boost::filesystem::path file, uint64_t line)
		: _code (code)
	{
		_data[Data::FILE] = file;
		_data[Data::LINE] = line;
	}

	VerificationNote(Code code, std::string note, boost::filesystem::path file, uint64_t line)
		: _code (code)
	{
		_data[Data::NOTE] = note;
		_data[Data::FILE] = file;
		_data[Data::LINE] = line;
	}

	Type type() const;

	Code code () const {
		return _code;
	}

private:
	enum class Data {
		ANNOTATION_TEXT,
		ASSET_ID,
		BIT_DEPTH,
		CALCULATED_HASH,
		CAPABILITIES,
		CODE_BLOCK_HEIGHT,
		CODE_BLOCK_WIDTH,
		COMPONENT,
		CONTENT_KIND,
		CONTENT_VERSION,
		CPL_ID,
		DURATION,
		ERROR,
		FILE,  ///< path of file containing the error
		FRAME,
		FRAME_RATE,
		GUARD_BITS,
		ISSUE_DATE,
		LANGUAGE,
		LINE,  ///< error line number within the FILE
		LOAD_FONT_ID,
		MAIN_PICTURE_ACTIVE_AREA,
		NOTE,  ///< further information about the error
		OTHER_ASSET_ID,
		OTHER_DURATION,
		PKL_ID,
		POC_MARKER,
		POC_MARKERS,
		REEL_INDEX, ///< reel index, counting from 0
		REFERENCE_HASH,
		SIZE_IN_BYTES,
		SIZE_IN_PIXELS,
		TERRITORY,
		TILE_PARTS,
		TIME,
		XML_NAMESPACE,
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

	VerificationNote& set_size_in_bytes(uint64_t size) {
		_data[Data::SIZE_IN_BYTES] = size;
		return *this;
	}

	boost::optional<uint64_t> size_in_bytes() const {
		return data<uint64_t>(Data::SIZE_IN_BYTES);
	}

	VerificationNote& set_load_font_id(std::string id) {
		_data[Data::LOAD_FONT_ID] = id;
		return *this;
	}

	boost::optional<std::string> load_font_id() const {
		return data<std::string>(Data::LOAD_FONT_ID);
	}

	VerificationNote& set_asset_id(std::string id) {
		_data[Data::ASSET_ID] = id;
		return *this;
	}

	boost::optional<std::string> asset_id() const {
		return data<std::string>(Data::ASSET_ID);
	}

	VerificationNote& set_other_asset_id(std::string other_id) {
		_data[Data::OTHER_ASSET_ID] = other_id;
		return *this;
	}

	boost::optional<std::string> other_asset_id() const {
		return data<std::string>(Data::OTHER_ASSET_ID);
	}

	VerificationNote& set_frame_rate(dcp::Fraction frame_rate) {
		_data[Data::FRAME_RATE] = frame_rate;
		return *this;
	}

	boost::optional<dcp::Fraction> frame_rate() const {
		return data<dcp::Fraction>(Data::FRAME_RATE);
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

	VerificationNote& set_cpl_id(std::string id) {
		_data[Data::CPL_ID] = id;
		return *this;
	}

	boost::optional<std::string> cpl_id() const {
		return data<std::string>(Data::CPL_ID);
	}

	VerificationNote& set_pkl_id(std::string id) {
		_data[Data::PKL_ID] = id;
		return *this;
	}

	boost::optional<std::string> pkl_id() const {
		return data<std::string>(Data::PKL_ID);
	}

	VerificationNote& set_reel_index(int index) {
		_data[Data::REEL_INDEX] = index;
		return *this;
	}

	boost::optional<int> reel_index() const {
		return data<int>(Data::REEL_INDEX);
	}

	VerificationNote& set_error(std::string error) {
		_data[Data::ERROR] = std::move(error);
		return *this;
	}

	boost::optional<std::string> error() const {
		return data<std::string>(Data::ERROR);
	}

	VerificationNote& set_language(std::string language) {
		_data[Data::LANGUAGE] = std::move(language);
		return *this;
	}

	boost::optional<std::string> language() const {
		return data<std::string>(Data::LANGUAGE);
	}

	VerificationNote& set_territory(std::string territory) {
		_data[Data::TERRITORY] = std::move(territory);
		return *this;
	}

	boost::optional<std::string> territory() const {
		return data<std::string>(Data::TERRITORY);
	}

	VerificationNote& set_size_in_pixels(dcp::Size size_in_pixels) {
		_data[Data::SIZE_IN_PIXELS] = size_in_pixels;
		return *this;
	}

	boost::optional<dcp::Size> size_in_pixels() const {
		return data<dcp::Size>(Data::SIZE_IN_PIXELS);
	}

	VerificationNote& set_bit_depth(int bit_depth) {
		_data[Data::BIT_DEPTH] = std::move(bit_depth);
		return *this;
	}

	boost::optional<int> bit_depth() const {
		return data<int>(Data::BIT_DEPTH);
	}

	VerificationNote& set_annotation_text(std::string annotation_text) {
		_data[Data::ANNOTATION_TEXT] = std::move(annotation_text);
		return *this;
	}

	boost::optional<std::string> annotation_text() const {
		return data<std::string>(Data::ANNOTATION_TEXT);
	}

	VerificationNote& set_time(dcp::Time time) {
		_data[Data::TIME] = std::move(time);
		return *this;
	}

	boost::optional<dcp::Time> time() const {
		return data<dcp::Time>(Data::TIME);
	}

	VerificationNote& set_guard_bits(int guard_bits) {
		_data[Data::GUARD_BITS] = guard_bits;
		return *this;
	}

	boost::optional<int> guard_bits() const {
		return data<int>(Data::GUARD_BITS);
	}

	VerificationNote& set_code_block_width(int code_block_width) {
		_data[Data::CODE_BLOCK_WIDTH] = code_block_width;
		return *this;
	}

	boost::optional<int> code_block_width() const {
		return data<int>(Data::CODE_BLOCK_WIDTH);
	}

	VerificationNote& set_code_block_height(int code_block_height) {
		_data[Data::CODE_BLOCK_HEIGHT] = code_block_height;
		return *this;
	}

	boost::optional<int> code_block_height() const {
		return data<int>(Data::CODE_BLOCK_HEIGHT);
	}

	VerificationNote& set_poc_marker(int poc_marker) {
		_data[Data::POC_MARKER] = poc_marker;
		return *this;
	}

	boost::optional<int> poc_marker() const {
		return data<int>(Data::POC_MARKER);
	}

	VerificationNote& set_poc_markers(int poc_markers) {
		_data[Data::POC_MARKERS] = poc_markers;
		return *this;
	}

	boost::optional<int> poc_markers() const {
		return data<int>(Data::POC_MARKERS);
	}

	VerificationNote& set_tile_parts(int tile_parts) {
		_data[Data::TILE_PARTS] = tile_parts;
		return *this;
	}

	boost::optional<int> tile_parts() const {
		return data<int>(Data::TILE_PARTS);
	}

	VerificationNote& set_capabilities(int capabilities) {
		_data[Data::CAPABILITIES] = capabilities;
		return *this;
	}

	boost::optional<int> capabilities() const {
		return data<int>(Data::CAPABILITIES);
	}

	VerificationNote& set_duration(int64_t duration) {
		_data[Data::DURATION] = std::move(duration);
		return *this;
	}

	boost::optional<int64_t> duration() const {
		return data<int64_t>(Data::DURATION);
	}

	VerificationNote& set_other_duration(int64_t other_duration) {
		_data[Data::OTHER_DURATION] = std::move(other_duration);
		return *this;
	}

	boost::optional<int64_t> other_duration() const {
		return data<int64_t>(Data::OTHER_DURATION);
	}

	VerificationNote& set_content_kind(std::string content_kind) {
		_data[Data::CONTENT_KIND] = std::move(content_kind);
		return *this;
	}

	boost::optional<std::string> content_kind() const {
		return data<std::string>(Data::CONTENT_KIND);
	}

	VerificationNote& set_main_picture_active_area(dcp::Size main_picture_active_area) {
		_data[Data::MAIN_PICTURE_ACTIVE_AREA] = std::move(main_picture_active_area);
		return *this;
	}

	boost::optional<dcp::Size> main_picture_active_area() const {
		return data<dcp::Size>(Data::MAIN_PICTURE_ACTIVE_AREA);
	}

	VerificationNote& set_issue_date(std::string issue_date) {
		_data[Data::ISSUE_DATE] = std::move(issue_date);
		return *this;
	}

	boost::optional<std::string> issue_date() const {
		return data<std::string>(Data::ISSUE_DATE);
	}

	VerificationNote& set_content_version(std::string content_version) {
		_data[Data::CONTENT_VERSION] = std::move(content_version);
		return *this;
	}

	boost::optional<std::string> content_version() const {
		return data<std::string>(Data::CONTENT_VERSION);
	}

	VerificationNote& set_xml_namespace(std::string xml_namespace) {
		_data[Data::XML_NAMESPACE] = std::move(xml_namespace);
		return *this;
	}

	boost::optional<std::string> xml_namespace() const {
		return data<std::string>(Data::XML_NAMESPACE);
	}

private:
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
	///< true to do some time-consuming detailed picture checks (e.g. J2K bitstream)
	bool check_picture_details = true;
};


struct VerificationResult
{
	std::vector<VerificationNote> notes;
	std::vector<std::shared_ptr<dcp::DCP>> dcps;
};


VerificationResult verify(
	std::vector<boost::filesystem::path> directories,
	std::vector<dcp::DecryptedKDM> kdms,
	std::function<void (std::string, boost::optional<boost::filesystem::path>)> stage,
	std::function<void (float)> progress,
	VerificationOptions options = {},
	boost::optional<boost::filesystem::path> xsd_dtd_directory = boost::optional<boost::filesystem::path>()
	);

std::string note_to_string(
	dcp::VerificationNote note,
	std::function<std::string (std::string)> process_string = [](std::string s) { return s; },
	std::function<std::string (std::string)> process_filename = [](std::string s) { return s; }
	);

bool operator== (dcp::VerificationNote const& a, dcp::VerificationNote const& b);
bool operator!=(dcp::VerificationNote const& a, dcp::VerificationNote const& b);
bool operator< (dcp::VerificationNote const& a, dcp::VerificationNote const& b);

std::ostream& operator<<(std::ostream& s, dcp::VerificationNote const& note);

}

#endif
