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
	int64_t size;
	std::string type;
	std::string original_file_name;
};

class PKL : public XMLFile
{
public:
	PKL (std::string file);

	boost::shared_ptr<PKLAsset> asset_from_id (std::string id) const;

	std::string id;
	std::string annotation_text;
	std::string issue_date;
	std::string issuer;
	std::string creator;
	std::list<boost::shared_ptr<PKLAsset> > assets;
};
	
}
