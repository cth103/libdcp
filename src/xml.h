#ifndef LIBDCP_XML_H
#define LIBDCP_XML_H

#include <string>
#include <list>
#include <stdint.h>
#include <glibmm.h>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
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
	std::string string_child (std::string);
	std::string optional_string_child (std::string);
	ContentKind kind_child (std::string);
	Fraction fraction_child (std::string);
	int64_t int64_child (std::string);
	int64_t optional_int64_child (std::string);
	float float_child (std::string);
	void ignore_child (std::string);
	void done ();

	Time time_attribute (std::string);
	float float_attribute (std::string);
	std::string string_attribute (std::string);
	std::string optional_string_attribute (std::string);
	int64_t int64_attribute (std::string);
	int64_t optional_int64_attribute (std::string);
	boost::optional<bool> optional_bool_attribute (std::string);
	boost::optional<Color> optional_color_attribute (std::string);

	std::string content ();

	template <class T>
	boost::shared_ptr<T> type_child (std::string name) {
		return boost::shared_ptr<T> (new T (node_child (name)));
	}

	template <class T>
	boost::shared_ptr<T> optional_type_child (std::string name) {
		std::list<xmlpp::Node*> n = node_children (name);
		if (n.size() > 1) {
			throw XMLError ("duplicate XML tag");
		} else if (n.empty ()) {
			return boost::shared_ptr<T> ();
		}
		
		return boost::shared_ptr<T> (new T (n.front ()));
	}
	
	template <class T>
	std::list<boost::shared_ptr<T> > type_children (std::string name) {
		std::list<xmlpp::Node*> n = node_children (name);
		std::list<boost::shared_ptr<T> > r;
		for (typename std::list<xmlpp::Node*>::iterator i = n.begin(); i != n.end(); ++i) {
			r.push_back (boost::shared_ptr<T> (new T (*i)));
		}
		return r;
	}

	template <class T>
	std::list<boost::shared_ptr<T> > type_grand_children (std::string name, std::string sub) {
		XMLNode p (node_child (name));
		return p.type_children<T> (sub);
	}

	xmlpp::Node const * _node;

private:
	xmlpp::Node* node_child (std::string);
	std::list<xmlpp::Node*> node_children (std::string);
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
