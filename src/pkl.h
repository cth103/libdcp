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

class PKL : public XMLFile
{
public:
	PKL (std::string file);

	std::string id;
	std::string annotation_text;
	std::string issue_date;
	std::string issuer;
	std::string creator;
	std::list<boost::shared_ptr<PKLAsset> > assets;
};
	
}
