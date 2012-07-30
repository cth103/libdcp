#include <boost/shared_ptr.hpp>
#include "xml.h"

namespace libdcp {

class AssetMapAsset : public XMLNode
{
public:
	AssetMapAsset ();
	AssetMapAsset (xmlpp::Node const * node);

	std::string id;
	std::string packing_list;
	boost::shared_ptr<ChunkList> 
};

class AssetMapAssetList : public XMLNode
{
public:
	AssetMapAssetList ();
	AssetMapAssetList (xmlpp::Node const * node);

	std::list<boost::shared_ptr<AssetMapAsset> > assets;
};

class AssetMap : public XMLFile
{
public:
	AssetMap (std::string file);

	std::string id;
	std::string creator;
	int volume_count;
	std::string issue_date;
	std::string issuer;
	boost::shared_ptr<AssetMapAssetList> asset_list;
};
