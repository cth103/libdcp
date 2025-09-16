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
	HTMLFormatter(boost::filesystem::path file);

	void heading(std::string const& text) override;
	void subheading(std::string const& text) override;
	Wrap document() override;
	Wrap body() override;
	Wrap unordered_list() override;
	void list_item(std::string const& text, boost::optional<std::string> type = {}) override;

	std::function<std::string (std::string)> process_string() override;
	std::function<std::string (std::string)> fixed_width() override;

private:
	void tagged(std::string tag, std::string content);
	Wrap wrapped(std::string const& tag);
};

}

