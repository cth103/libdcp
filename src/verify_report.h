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


#include "compose.hpp"
#include "file.h"
#include "verify.h"
#include <boost/filesystem.hpp>
#include <vector>


namespace dcp {


class Formatter
{
public:
	Formatter(boost::filesystem::path file)
		: _file(file, "w")
	{}

	class Wrap
	{
	public:
		Wrap() = default;

		Wrap(Formatter* formatter, std::string const& close)
			: _formatter(formatter)
			, _close(close)
		{}

		Wrap(Formatter* formatter, std::string const& close, std::function<void ()> closer)
			: _formatter(formatter)
			, _close(close)
			, _closer(closer)
		{}

		Wrap(Wrap&& other)
		{
			std::swap(_formatter, other._formatter);
			std::swap(_close, other._close);
			std::swap(_closer, other._closer);
		}

		~Wrap()
		{
			if (_formatter) {
				_formatter->file().puts(_close.c_str());
			}
			if (_closer) {
				_closer();
			}
		}

	private:
		Formatter* _formatter = nullptr;
		std::string _close;
		std::function<void ()> _closer = nullptr;
	};

	virtual Wrap document() { return {}; }

	virtual void heading(std::string const& text) = 0;
	virtual void subheading(std::string const& text) = 0;
	virtual Wrap body() { return {}; }

	virtual Wrap unordered_list() = 0;
	virtual void list_item(std::string const& text, boost::optional<std::string> type = {}) = 0;

	virtual std::function<std::string (std::string)> process_string() = 0;
	virtual std::function<std::string (std::string)> process_filename() = 0;

	dcp::File& file() {
		return _file;
	}

protected:
	dcp::File _file;
};


class TextFormatter : public Formatter
{
public:
	TextFormatter(boost::filesystem::path file)
		: Formatter(file)
	{}

	void heading(std::string const& text) override {
		print(text);
	}

	void subheading(std::string const& text) override {
		print("");
		print(text);
	}

	Wrap unordered_list() override {
		_indent++;
		return Wrap(this, "", [this]() { _indent--; });
	}

	void list_item(std::string const& text, boost::optional<std::string> type = {}) override {
		LIBDCP_UNUSED(type);
		for (int i = 0; i < _indent * 2; ++i) {
			_file.puts(" ");
		}
		_file.puts("* ");
		print(text);
	}

	std::function<std::string (std::string)> process_string() override {
		return [](std::string s) {
			return s;
		};
	}

	std::function<std::string (std::string)> process_filename() override {
		return [](std::string s) {
			return s;
		};
	}

private:
	void print(std::string const& text) {
		_file.puts(text.c_str());
		_file.puts("\n");
	}

	int _indent = 0;
};


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
			_file.puts(dcp::String::compose("<li class=\"%1\">%2</li>", *type, text).c_str());
		} else {
			_file.puts(dcp::String::compose("<li>%1</li>", text).c_str());
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


extern void verify_report(std::vector<dcp::VerificationResult> const& results, Formatter& formatter);


}

