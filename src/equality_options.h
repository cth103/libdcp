/*
    Copyright (C) 2012-2023 Carl Hetherington <cth@carlh.net>

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


/** @file  src/equality_options.h
 *  @brief Class to describe what equality means when calling Asset::equals().
 */


#ifndef LIBDCP_EQUALITY_OPTIONS_H
#define LIBDCP_EQUALITY_OPTIONS_H


namespace dcp {


/** @class EqualityOptions
 *  @brief A class to describe what "equality" means for a particular test.
 *
 *  When comparing things, we want to be able to ignore some differences;
 *  this class expresses those differences.
 *
 *  It also contains some settings for how the comparison should be done.
 */
class EqualityOptions
{
public:
	/** Construct an EqualityOptions where nothing at all can differ */
	EqualityOptions() = default;

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
	/** true if asset hashes can differ */
	bool asset_hashes_can_differ = false;
	/** true if IssueDate nodes can differ */
	bool issue_dates_can_differ = false;
	bool load_font_nodes_can_differ = false;
	bool sound_assets_can_differ = false;
	bool keep_going = false;
	/** true to save the last pair of different image subtitles to the current working directory */
	bool export_differing_subtitles = false;
	/** The maximum allowable absolute difference between the vertical position of subtitles */
	float max_subtitle_vertical_position_error = 0;
};


}


#endif

