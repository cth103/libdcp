/*
    Copyright (C) 2012-2020 Carl Hetherington <cth@carlh.net>

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
*/


#include "cpl.h"
#include "dcp.h"
#include "reel.h"
#include "reel_subtitle_asset.h"
#include "subtitle.h"
#include "reel_asset.h"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/test/unit_test.hpp>

namespace xmlpp {
	class Element;
}

namespace dcp {
	class DCP;
	class MonoPictureAsset;
	class SoundAsset;
}

extern boost::filesystem::path private_test;
extern boost::filesystem::path xsd_test;

extern void check_xml (xmlpp::Element* ref, xmlpp::Element* test, std::vector<std::string> ignore_tags, bool ignore_whitespace = false);
extern void check_xml (std::string ref, std::string test, std::vector<std::string> ignore, bool ignore_whitespace = false);
extern void check_file (boost::filesystem::path ref, boost::filesystem::path check);
extern std::shared_ptr<dcp::MonoPictureAsset> simple_picture (
	boost::filesystem::path path,
	std::string suffix,
	int frames = 24,
	boost::optional<dcp::Key> key = boost::optional<dcp::Key>()
	);
extern std::shared_ptr<dcp::SoundAsset> simple_sound (
	boost::filesystem::path path,
	std::string suffix,
	dcp::MXFMetadata mxf_meta,
	std::string language,
	int frames = 24,
	int sample_rate = 48000,
	boost::optional<dcp::Key> key = boost::optional<dcp::Key>(),
	int channels = 6
	);
extern std::shared_ptr<dcp::Subtitle> simple_subtitle ();
extern std::shared_ptr<dcp::ReelMarkersAsset> simple_markers (int frames = 24);
extern std::shared_ptr<dcp::DCP> make_simple (
	boost::filesystem::path path,
	int reels = 1,
	int frames = 24,
	dcp::Standard = dcp::Standard::SMPTE,
	boost::optional<dcp::Key> key = boost::optional<dcp::Key>()
	);
extern std::shared_ptr<dcp::DCP> make_simple_with_interop_subs (boost::filesystem::path path);
extern std::shared_ptr<dcp::DCP> make_simple_with_smpte_subs (boost::filesystem::path path);
extern std::shared_ptr<dcp::DCP> make_simple_with_interop_ccaps (boost::filesystem::path path);
extern std::shared_ptr<dcp::DCP> make_simple_with_smpte_ccaps (boost::filesystem::path path);
extern std::shared_ptr<dcp::OpenJPEGImage> black_image (dcp::Size size = dcp::Size(1998, 1080));
extern std::shared_ptr<dcp::ReelAsset> black_picture_asset (boost::filesystem::path dir, int frames = 24);
extern boost::filesystem::path find_file (boost::filesystem::path dir, std::string filename_part);

/** Creating an object of this class will make asdcplib's random number generation
 *  (more) predictable.
 */
class RNGFixer
{
public:
	RNGFixer ();
	~RNGFixer ();
};


/** Class that can alter a file by searching and replacing strings within it.
 *  On destruction modifies the file whose name was given to the constructor.
 */
class Editor
{
public:
	Editor(boost::filesystem::path path);
	~Editor();

	class ChangeChecker
	{
	public:
		ChangeChecker(Editor* editor)
			: _editor(editor)
		{
			_old_content = _editor->_content;
		}

		~ChangeChecker()
		{
			BOOST_REQUIRE(_old_content != _editor->_content);
		}
	private:
		Editor* _editor;
		std::string _old_content;
	};

	void replace(std::string a, std::string b);
	void delete_first_line_containing(std::string s);
	void delete_lines(std::string from, std::string to);
	void insert(std::string after, std::string line);
	void delete_lines_after(std::string after, int lines_to_delete);

private:
	friend class ChangeChecker;

	std::vector<std::string> as_lines() const;

	boost::filesystem::path _path;
	std::string _content;
};


