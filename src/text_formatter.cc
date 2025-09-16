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


#include "text_formatter.h"


using std::unique_ptr;
using namespace dcp;


TextFormatter::TextFormatter(boost::filesystem::path file)
	: StreamFormatter(file)
{

}


void
TextFormatter::heading(std::string const& text)
{
	print(text);
}


void
TextFormatter::subheading(std::string const& text)
{
	print("");
	print(text);
}


unique_ptr<Formatter::Wrap>
TextFormatter::unordered_list()
{
	_indent++;
	return unique_ptr<Formatter::Wrap>(new Wrap(this, "", [this]() { _indent--; }));
}


void
TextFormatter::list_item(std::string const& text, boost::optional<std::string> type)
{
	LIBDCP_UNUSED(type);
	for (int i = 0; i < _indent * 2; ++i) {
		_file.puts(" ");
	}
	_file.puts("* ");
	print(text);
}


std::function<std::string (std::string)>
TextFormatter::process_string()
{
	return [](std::string s) {
		return s;
	};
}


std::function<std::string (std::string)>
TextFormatter::fixed_width()
{
	return [](std::string s) {
		return s;
	};
}


void
TextFormatter::print(std::string const& text)
{
	_file.puts(text.c_str());
	_file.puts("\n");
}
