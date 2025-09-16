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


}

