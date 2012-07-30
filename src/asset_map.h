#include <boost/shared_ptr.hpp>
#include "xml.h"

namespace libdcp {

class Chunk : public XMLNode
{
public:
	Chunk ();
	Chunk (xmlpp::Node const * node);

	std::string path;
	int volume_index;
	int offset;
	int length;
};

class AssetMapAsset : public XMLNode
{
public:
	AssetMapAsset ();
	AssetMapAsset (xmlpp::Node const * node);

	std::string id;
	std::string packing_list;
	std::list<boost::shared_ptr<Chunk> > chunks;
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
	std::list<boost::shared_ptr<AssetMapAsset> > assets;
};

}
