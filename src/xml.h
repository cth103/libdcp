#ifndef LIBDCP_XML_H
#define LIBDCP_XML_H

#include <string>
#include <list>
#include <stdint.h>
#include <glibmm.h>
#include <boost/shared_ptr.hpp>
#include "types.h"
#include "exceptions.h"
#include "dcp_time.h"

namespace xmlpp {
	class Node;
	class DomParser;
}

namespace libdcp {

class XMLNode
{
public:
	XMLNode ();
	XMLNode (xmlpp::Node const * node);

protected:
	std::string string_node (std::string);
	std::string optional_string_node (std::string);
	ContentKind kind_node (std::string);
	Fraction fraction_node (std::string);
	int64_t int64_node (std::string);
	int64_t optional_int64_node (std::string);
	float float_node (std::string);
	void ignore_node (std::string);
	void done ();

	Time time_attribute (std::string);
	float float_attribute (std::string);
	std::string string_attribute (std::string);
	int64_t int64_attribute (std::string);
	int64_t optional_int64_attribute (std::string);

	std::string content ();

	template <class T>
	boost::shared_ptr<T> sub_node (std::string name) {
		return boost::shared_ptr<T> (new T (xml_node (name)));
	}

	template <class T>
	boost::shared_ptr<T> optional_sub_node (std::string name) {
		std::list<xmlpp::Node*> n = xml_nodes (name);
		if (n.size() > 1) {
			throw XMLError ("duplicate XML tag");
		} else if (n.empty ()) {
			return boost::shared_ptr<T> ();
		}
		
		return boost::shared_ptr<T> (new T (n.front ()));
	}
	
	template <class T>
	std::list<boost::shared_ptr<T> > sub_nodes (std::string name) {
		std::list<xmlpp::Node*> n = xml_nodes (name);
		std::list<boost::shared_ptr<T> > r;
		for (typename std::list<xmlpp::Node*>::iterator i = n.begin(); i != n.end(); ++i) {
			r.push_back (boost::shared_ptr<T> (new T (*i)));
		}
		return r;
	}

	template <class T>
	std::list<boost::shared_ptr<T> > sub_nodes (std::string name, std::string sub) {
		XMLNode p (xml_node (name));
		return p.sub_nodes<T> (sub);
	}

	xmlpp::Node const * _node;

private:
	xmlpp::Node* xml_node (std::string);
	std::list<xmlpp::Node*> xml_nodes (std::string);
	std::list<Glib::ustring> _taken;
};

class XMLFile : public XMLNode
{
public:
	XMLFile (std::string file, std::string root_name);
	virtual ~XMLFile ();

private:
	xmlpp::DomParser* _parser;
};

}

#endif
