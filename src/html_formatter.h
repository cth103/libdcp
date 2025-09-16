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


#include "verify_report.h"


namespace dcp {


class HTMLFormatter : public Formatter
{
public:
	HTMLFormatter(boost::filesystem::path file)
		: Formatter(file)
	{}

	void heading(std::string const& text) override {
		tagged("h1", text);
	}

	void subheading(std::string const& text) override {
		tagged("h2", text);
	}

	Wrap document() override {
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

	Wrap body() override {
		return wrapped("body");
	}

	Wrap unordered_list() override {
		return wrapped("ul");
	}

	void list_item(std::string const& text, boost::optional<std::string> type = {}) override {
		if (type) {
			_file.puts(dcp::String::compose("<li class=\"%1\">%2</li>\n", *type, text).c_str());
		} else {
			_file.puts(dcp::String::compose("<li>%1</li>\n", text).c_str());
		}
	}

	std::function<std::string (std::string)> process_string() override {
		return [](std::string s) {
			boost::replace_all(s, "<", "&lt;");
			boost::replace_all(s, ">", "&gt;");
			return s;
		};
	}

	std::function<std::string (std::string)> process_filename() override {
		return [](std::string s) {
			return String::compose("<code>%1</code>", s);
		};
	}

private:
	void tagged(std::string tag, std::string content) {
		_file.puts(String::compose("<%1>%2</%3>\n", tag, content, tag).c_str());
	};

	Wrap wrapped(std::string const& tag) {
		_file.puts(String::compose("<%1>", tag).c_str());
		return Wrap(this, String::compose("</%1>", tag));
	};
};

}

