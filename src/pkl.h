#include <boost/shared_ptr.hpp>
#include "xml.h"

namespace libdcp {

class PKLAsset : public XMLNode
{
public:
	PKLAsset () {}
	PKLAsset (xmlpp::Node const * node);

	std::string id;
	std::string annotation_text;
	std::string hash;
	int size;
	std::string type;
};

class PKLAssetList : public XMLNode
{
public:
	PKLAssetList ();
	PKLAssetList (xmlpp::Node const * node);

	std::list<boost::shared_ptr<PKLAsset> > assets;
};

class PKL : public XMLFile
{
public:
	PKL (std::string file);

	std::string id;
	std::string annotation_text;
	std::string issue_date;
	std::string issuer;
	std::string creator;
	boost::shared_ptr<PKLAssetList> asset_list;
};
	
}
