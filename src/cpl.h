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
	int intrinsic_duration;
	int entry_point;
	int duration;
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
	int intrinsic_duration;
	int entry_point;
	int duration;
};

class CPLAssetList : public XMLNode
{
public:
	CPLAssetList () {}
	CPLAssetList (xmlpp::Node const * node);

	boost::shared_ptr<MainPicture> main_picture;
	boost::shared_ptr<MainSound> main_sound;
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

