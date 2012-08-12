#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include "xml.h"

namespace libdcp {

class MainPicture : public XMLNode
{
public:
	MainPicture () {}
	MainPicture (xmlpp::Node const * node);

	std::string id;
	std::string annotation_text;
	Fraction edit_rate;
	int64_t intrinsic_duration;
	int64_t entry_point;
	int64_t duration;
	Fraction frame_rate;
	Fraction screen_aspect_ratio;
};

class MainSound : public XMLNode
{
public:
	MainSound () {}
	MainSound (xmlpp::Node const * node);

	std::string id;
	std::string annotation_text;
	Fraction edit_rate;
	int64_t intrinsic_duration;
	int64_t entry_point;
	int64_t duration;
};

class MainSubtitle : public XMLNode
{
public:
	MainSubtitle () {}
	MainSubtitle (xmlpp::Node const * node);

	std::string id;
	std::string annotation_text;
	Fraction edit_rate;
	int64_t intrinsic_duration;
	int64_t entry_point;
	int64_t duration;
};

class CPLAssetList : public XMLNode
{
public:
	CPLAssetList () {}
	CPLAssetList (xmlpp::Node const * node);

	boost::shared_ptr<MainPicture> main_picture;
	boost::shared_ptr<MainSound> main_sound;
	boost::shared_ptr<MainSubtitle> main_subtitle;
};

class Reel : public XMLNode
{
public:
	Reel () {}
	Reel (xmlpp::Node const * node);

	std::string id;
	boost::shared_ptr<CPLAssetList> asset_list;
};

class ContentVersion : public XMLNode
{
public:
	ContentVersion () {}
	ContentVersion (xmlpp::Node const * node);

	std::string id;
	std::string label_text;
};

class CPL : public XMLFile
{
public:
	CPL (std::string file);

	std::string id;
	std::string annotation_text;
	std::string issue_date;
	std::string creator;
	std::string content_title_text;
	ContentKind content_kind;
	boost::shared_ptr<ContentVersion> content_version;
	std::list<boost::shared_ptr<Reel> > reels;
};

}

