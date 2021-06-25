/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/types.h
 *  @brief Miscellaneous types
 */


#ifndef LIBDCP_TYPES_H
#define LIBDCP_TYPES_H


#include "warnings.h"
#include <libcxml/cxml.h>
LIBDCP_DISABLE_WARNINGS
#include <asdcp/KLV.h>
LIBDCP_ENABLE_WARNINGS
#include <memory>
#include <boost/function.hpp>
#include <string>


/* MinGW seems to define this, but we want to use it */
#undef ERROR


namespace xmlpp {
	class Element;
}


namespace dcp
{


/** @struct Size
 *  @brief The integer, two-dimensional size of something.
 */
struct Size
{
	Size ()
		: width (0)
		, height (0)
	{}

	Size (int w, int h)
		: width (w)
		, height (h)
	{}

	float ratio () const {
		return float (width) / height;
	}

	int width;
	int height;
};


extern bool operator== (Size const & a, Size const & b);
extern bool operator!= (Size const & a, Size const & b);


/** Identifier for a sound channel */
enum class Channel {
	LEFT = 0,      ///< left
	RIGHT = 1,     ///< right
	CENTRE = 2,    ///< centre
	LFE = 3,       ///< low-frequency effects (sub)
	LS = 4,        ///< left surround
	RS = 5,        ///< right surround
	HI = 6,
	VI = 7,
	/* 8 and 9 are not used */
	BSL = 10,
	BSR = 11,
	MOTION_DATA = 12,
	SYNC_SIGNAL = 13,
	SIGN_LANGUAGE = 14,
	/* 15 is not used */
	CHANNEL_COUNT = 16
};


std::vector<dcp::Channel> used_audio_channels ();


enum class MCASoundField
{
	FIVE_POINT_ONE,
	SEVEN_POINT_ONE
};


extern std::string channel_to_mca_id (Channel c, MCASoundField field);
extern Channel mca_id_to_channel (std::string);
extern std::string channel_to_mca_name (Channel c, MCASoundField field);
extern ASDCP::UL channel_to_mca_universal_label (Channel c, MCASoundField field, ASDCP::Dictionary const* dict);


enum class ContentKind
{
	FEATURE,
	SHORT,
	TRAILER,
	TEST,
	TRANSITIONAL,
	RATING,
	TEASER,
	POLICY,
	PUBLIC_SERVICE_ANNOUNCEMENT,
	ADVERTISEMENT,
	EPISODE,
	PROMO
};


extern std::string content_kind_to_string (ContentKind kind);
extern ContentKind content_kind_from_string (std::string kind);


enum class Effect
{
	NONE,
	BORDER,
	SHADOW
};


extern std::string effect_to_string (Effect e);
extern Effect string_to_effect (std::string s);


enum class HAlign
{
	LEFT,   ///< horizontal position is distance from left of screen to left of subtitle
	CENTER, ///< horizontal position is distance from centre of screen to centre of subtitle
	RIGHT,  ///< horizontal position is distance from right of screen to right of subtitle
};


extern std::string halign_to_string (HAlign a);
extern HAlign string_to_halign (std::string s);


enum class VAlign
{
	TOP,    ///< vertical position is distance from top of screen to top of subtitle
	CENTER, ///< vertical position is distance from centre of screen to centre of subtitle
	BOTTOM  ///< vertical position is distance from bottom of screen to bottom of subtitle
};


extern std::string valign_to_string (VAlign a);
extern VAlign string_to_valign (std::string s);


/** Direction for subtitle test */
enum class Direction
{
	LTR, ///< left-to-right
	RTL, ///< right-to-left
	TTB, ///< top-to-bottom
	BTT  ///< bottom-to-top
};


extern std::string direction_to_string (Direction a);
extern Direction string_to_direction (std::string s);


enum class Eye
{
	LEFT,
	RIGHT
};


/** @class Fraction
 *  @brief A fraction (i.e. a thing with an integer numerator and an integer denominator).
 */
class Fraction
{
public:
	/** Construct a fraction of 0/0 */
	Fraction () {}
	explicit Fraction (std::string s);
	/** Construct a fraction with a specified numerator and denominator.
	 *  @param n Numerator.
	 *  @param d Denominator.
	 */
	Fraction (int n, int d) : numerator (n), denominator (d) {}

	float as_float () const {
		return float (numerator) / denominator;
	}

	std::string as_string () const;

	int numerator = 0;
	int denominator = 0;
};


extern bool operator== (Fraction const & a, Fraction const & b);
extern bool operator!= (Fraction const & a, Fraction const & b);


/** @struct EqualityOptions
 *  @brief  A class to describe what "equality" means for a particular test.
 *
 *  When comparing things, we want to be able to ignore some differences;
 *  this class expresses those differences.
 *
 *  It also contains some settings for how the comparison should be done.
 */
struct EqualityOptions
{
	/** Construct an EqualityOptions where nothing at all can differ */
	EqualityOptions () {}

	/** The maximum allowable mean difference in pixel value between two images */
	double max_mean_pixel_error = 0;
	/** The maximum standard deviation of the differences in pixel value between two images */
	double max_std_dev_pixel_error = 0;
	/** The maximum difference in audio sample value between two soundtracks */
	int max_audio_sample_error = 0;
	/** true if the &lt;AnnotationText&gt; nodes of CPLs are allowed to differ */
	bool cpl_annotation_texts_can_differ = false;
	/** true if the &lt;AnnotationText&gt; nodes of Reels are allowed to differ */
	bool reel_annotation_texts_can_differ = false;
	/** true if <Hash>es in Reels can differ */
	bool reel_hashes_can_differ = false;
	/** true if IssueDate nodes can differ */
	bool issue_dates_can_differ = false;
	bool load_font_nodes_can_differ = false;
	bool keep_going = false;
	/** true to save the first pair of differeng image subtitles to the current working directory */
	bool export_differing_subtitles = false;
};


enum class NoteType {
	PROGRESS,
	ERROR,
	NOTE
};


enum class Standard {
	INTEROP,
	SMPTE
};


enum class Formulation {
	MODIFIED_TRANSITIONAL_1,
	MULTIPLE_MODIFIED_TRANSITIONAL_1,
	DCI_ANY,
	DCI_SPECIFIC,
	/** For testing: adds no AuthorizedDeviceInfo tag */
	MODIFIED_TRANSITIONAL_TEST
};


/** @class Colour
 *  @brief An RGB colour
 */
class Colour
{
public:
	/** Construct a Colour, initialising it to black */
	Colour ();

	/** Construct a Colour from R, G and B.  The values run between
	 *  0 and 255.
	 */
	Colour (int r_, int g_, int b_);

	/** Construct a Colour from an ARGB hex string; the alpha value is ignored.
	 *  @param argb_hex A string of the form AARRGGBB, where e.g. RR is a two-character
	 *  hex value.
	 */
	explicit Colour (std::string argb_hex);

	int r = 0; ///< red component, from 0 to 255
	int g = 0; ///< green component, from 0 to 255
	int b = 0; ///< blue component, from 0 to 255

	/** @return An RGB string of the form RRGGBB, where e.g. RR is a two-character
	 *  hex value.
	 */
	std::string to_rgb_string () const;

	/** @return An ARGB string of the form AARRGGBB, where e.g. RR is a two-character
	 *  hex value.  The alpha value will always be FF (ie 255; maximum alpha).
	 */
	std::string to_argb_string () const;
};


extern bool operator== (Colour const & a, Colour const & b);
extern bool operator!= (Colour const & a, Colour const & b);


typedef boost::function<void (NoteType, std::string)> NoteHandler;


/** Maximum absolute difference between dcp::SubtitleString::aspect_adjust values that
 *  are considered equal
 */
constexpr float ASPECT_ADJUST_EPSILON = 1e-3;


/** Maximum absolute difference between dcp::SubtitleString alignment values that
 *  are considered equal.
 */
constexpr float ALIGN_EPSILON = 1e-3;


enum class Marker {
	FFOC, ///< first frame of composition
	LFOC, ///< last frame of composition
	FFTC, ///< first frame of title credits
	LFTC, ///< last frame of title credits
	FFOI, ///< first frame of intermission
	LFOI, ///< last frame of intermission
	FFEC, ///< first frame of end credits
	LFEC, ///< last frame of end credits
	FFMC, ///< first frame of moving credits
	LFMC  ///< last frame of moving credits
};


std::string marker_to_string (Marker);
Marker marker_from_string (std::string);


class Rating
{
public:
	Rating (std::string agency_, std::string label_)
		: agency (agency_)
		, label (label_)
	{}

	explicit Rating (cxml::ConstNodePtr node);

	void as_xml (xmlpp::Element* parent) const;

	/** URI of the agency issuing the rating */
	std::string agency;
	/** Rating (e.g. PG, PG-13, 12A etc) */
	std::string label;
};


extern bool operator== (Rating const & a, Rating const & b);


enum class Status
{
	FINAL, ///< final version
	TEMP,  ///< temporary version (picture/sound unfinished)
	PRE    ///< pre-release (picture/sound finished)
};


extern std::string status_to_string (Status s);
extern Status string_to_status (std::string s);


class ContentVersion
{
public:
	ContentVersion ();

	explicit ContentVersion (cxml::ConstNodePtr node);

	explicit ContentVersion (std::string label_text_);

	ContentVersion (std::string id_, std::string label_text_)
		: id (id_)
		, label_text (label_text_)
	{}

	void as_xml (xmlpp::Element* parent) const;

	std::string id;
	std::string label_text;
};


class Luminance
{
public:
	enum class Unit {
		CANDELA_PER_SQUARE_METRE,
		FOOT_LAMBERT
	};

	Luminance (cxml::ConstNodePtr node);

	Luminance (float value, Unit unit);

	void set_value (float v);
	void set_unit (Unit u) {
		_unit = u;
	}

	float value () const {
		return _value;
	}

	Unit unit () const {
		return _unit;
	}

	float value_in_foot_lamberts () const;

	void as_xml (xmlpp::Element* parent, std::string ns) const;

	static std::string unit_to_string (Unit u);
	static Unit string_to_unit (std::string u);

private:
	float _value;
	Unit _unit;
};


bool operator== (Luminance const& a, Luminance const& b);


class MainSoundConfiguration
{
public:
	MainSoundConfiguration (std::string);
	MainSoundConfiguration (MCASoundField field_, int channels);

	MCASoundField field () const {
		return _field;
	}

	int channels () const {
		return _channels.size();
	}

	boost::optional<Channel> mapping (int index) const;
	void set_mapping (int index, Channel channel);

	std::string to_string () const;

private:
	MCASoundField _field;
	std::vector<boost::optional<Channel>> _channels;
};


}


#endif
