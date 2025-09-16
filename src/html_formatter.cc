/*
    Copyright (C) 2022 Carl Hetherington <cth@carlh.net>

    This file is part of DCP-o-matic.

    DCP-o-matic is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DCP-o-matic is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DCP-o-matic.  If not, see <http://www.gnu.org/licenses/>.

*/


#include "html_formatter.h"
#include <boost/filesystem.hpp>


using namespace dcp;



HTMLFormatter::HTMLFormatter(boost::filesystem::path file)
	: Formatter(file)
{

}


void
HTMLFormatter::heading(std::string const& text)
{
	tagged("h1", text);
}


void
HTMLFormatter::subheading(std::string const& text)
{
	tagged("h2", text);
}


Formatter::Wrap
HTMLFormatter::document()
{
	auto html = wrapped("html");
	auto head = wrapped("head");
	auto style = wrapped("style");
	_file.puts("li {\n"
		   "  margin: 2px;\n"
		   "  padding: 2px 2px 2px 1em;\n"
		   "}\n"
		  );
	_file.puts("li.ok {\n"
		   "  background-color: #00ff00;\n"
		   "}\n"
		   "li.warning {\n"
		   "  background-color: #ffa500;\n"
		   "}\n"
		   "li.error {\n"
		   "  background-color: #ff0000;\n"
		   "}\n"
		   "li.bv21-error {\n"
		   "  background-color: #ff6666;\n"
		   "}\n"
		   "ul {\n"
		   "  list-style: none;\n"
		   "}\n"
		  );
	return html;
}


Formatter::Wrap
HTMLFormatter::body()
{
	return wrapped("body");
}


Formatter::Wrap
HTMLFormatter::unordered_list()
{
	return wrapped("ul");
}


void
HTMLFormatter::list_item(std::string const& text, boost::optional<std::string> type)
{
	if (type) {
		_file.puts(dcp::String::compose("<li class=\"%1\">%2</li>\n", *type, text).c_str());
	} else {
		_file.puts(dcp::String::compose("<li>%1</li>\n", text).c_str());
	}
}


std::function<std::string (std::string)>
HTMLFormatter::process_string()
{
	return [](std::string s) {
		boost::replace_all(s, "<", "&lt;");
		boost::replace_all(s, ">", "&gt;");
		return s;
	};
}


std::function<std::string (std::string)>
HTMLFormatter::process_filename()
{
	return [](std::string s) {
		return String::compose("<code>%1</code>", s);
	};
}


void
HTMLFormatter::tagged(std::string tag, std::string content)
{
	_file.puts(String::compose("<%1>%2</%3>\n", tag, content, tag).c_str());
}


HTMLFormatter::Wrap
HTMLFormatter::wrapped(std::string const& tag)
{
	_file.puts(String::compose("<%1>", tag).c_str());
	return Wrap(this, String::compose("</%1>", tag));
}

