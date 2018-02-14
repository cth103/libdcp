/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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
 *  @brief Miscellaneous types.
 */

#ifndef LIBDCP_TYPES_H
#define LIBDCP_TYPES_H

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <string>

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
extern std::ostream& operator<< (std::ostream& s, Size const & a);

/** Identifier for a sound channel */
enum Channel {
	LEFT = 0,      ///< left
	RIGHT = 1,     ///< right
	CENTRE = 2,    ///< centre
	LFE = 3,       ///< low-frequency effects (sub)
	LS = 4,        ///< left surround
	RS = 5,        ///< right surround
	HI = 6,
	VI = 7,
	LC = 8,
	RC = 9,
	BSL = 10,
	BSR = 11
};

enum ContentKind
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
	ADVERTISEMENT
};

enum Effect
{
	NONE,
	BORDER,
	SHADOW
};

extern std::string effect_to_string (Effect e);
extern Effect string_to_effect (std::string s);

enum HAlign
{
	HALIGN_LEFT,   ///< horizontal position is distance from left of screen to left of subtitle
	HALIGN_CENTER, ///< horizontal position is distance from centre of screen to centre of subtitle
	HALIGN_RIGHT,  ///< horizontal position is distance from right of screen to right of subtitle
};

extern std::string halign_to_string (HAlign a);
extern HAlign string_to_halign (std::string s);

enum VAlign
{
	VALIGN_TOP,    ///< vertical position is distance from top of screen to top of subtitle
	VALIGN_CENTER, ///< vertical position is distance from centre of screen to centre of subtitle
	VALIGN_BOTTOM  ///< vertical position is distance from bottom of screen to bottom of subtitle
};

extern std::string valign_to_string (VAlign a);
extern VAlign string_to_valign (std::string s);

/** Direction for subtitle test */
enum Direction
{
	DIRECTION_LTR, ///< left-to-right
	DIRECTION_RTL, ///< right-to-left
	DIRECTION_TTB, ///< top-to-bottom
	DIRECTION_BTT  ///< bottom-to-top
};

extern std::string direction_to_string (Direction a);
extern Direction string_to_direction (std::string s);

enum Eye
{
	EYE_LEFT,
	EYE_RIGHT
};

/** @class Fraction
 *  @brief A fraction (i.e. a thing with an integer numerator and an integer denominator).
 */
class Fraction
{
public:
	/** Construct a fraction of 0/0 */
	Fraction () : numerator (0), denominator (0) {}
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

	int numerator;
	int denominator;
};

extern bool operator== (Fraction const & a, Fraction const & b);
extern bool operator!= (Fraction const & a, Fraction const & b);
extern std::ostream& operator<< (std::ostream& s, Fraction const & f);

/** @struct EqualityOptions
 *  @brief  A class to describe what "equality" means for a particular test.
 *
 *  When comparing things, we want to be able to ignore some differences;
 *  this class expresses those differences.
 */
struct EqualityOptions
{
	/** Construct an EqualityOptions where nothing at all can differ */
	EqualityOptions ()
		: max_mean_pixel_error (0)
		, max_std_dev_pixel_error (0)
		, max_audio_sample_error (0)
		, cpl_annotation_texts_can_differ (false)
		, reel_annotation_texts_can_differ (false)
		, reel_hashes_can_differ (false)
		, issue_dates_can_differ (false)
		, keep_going (false)
	{}

	/** The maximum allowable mean difference in pixel value between two images */
	double max_mean_pixel_error;
	/** The maximum standard deviation of the differences in pixel value between two images */
	double max_std_dev_pixel_error;
	/** The maximum difference in audio sample value between two soundtracks */
	int max_audio_sample_error;
	/** true if the &lt;AnnotationText&gt; nodes of CPLs are allowed to differ */
	bool cpl_annotation_texts_can_differ;
	/** true if the &lt;AnnotationText&gt; nodes of Reels are allowed to differ */
	bool reel_annotation_texts_can_differ;
	/** true if <Hash>es in Reels can differ */
	bool reel_hashes_can_differ;
	/** true if IssueDate nodes can differ */
	bool issue_dates_can_differ;
	bool keep_going;
};

/* I've been unable to make mingw happy with ERROR as a symbol, so
   I'm using a DCP_ prefix here.
*/
enum NoteType {
	DCP_PROGRESS,
	DCP_ERROR,
	DCP_NOTE
};

enum Standard {
	INTEROP,
	SMPTE
};

enum Formulation {
	MODIFIED_TRANSITIONAL_1,
	MULTIPLE_MODIFIED_TRANSITIONAL_1,
	DCI_ANY,
	DCI_SPECIFIC,
	/** For testing: adds no AuthorizedDeviceInfo tag */
	MODIFIED_TRANSITIONAL_TEST
};

/** @class Colour
 *  @brief An RGB colour.
 */
class Colour
{
public:
	Colour ();
	Colour (int r_, int g_, int b_);
	explicit Colour (std::string argb_hex);

	int r; ///< red component, from 0 to 255
	int g; ///< green component, from 0 to 255
	int b; ///< blue component, from 0 to 255

	std::string to_rgb_string () const;
	std::string to_argb_string () const;
};

extern bool operator== (Colour const & a, Colour const & b);
extern bool operator!= (Colour const & a, Colour const & b);
extern std::ostream & operator<< (std::ostream & s, Colour const & c);

typedef boost::function<void (NoteType, std::string)> NoteHandler;

/** Maximum absolute difference between dcp::SubtitleString::aspect_adjust values that
 *  are considered equal.
 */
const float ASPECT_ADJUST_EPSILON = 1e-3;

/** Maximum absolute difference between dcp::SubtitleString alignment values that
 *  are considered equal.
 */
const float ALIGN_EPSILON = 1e-3;

}

#endif
