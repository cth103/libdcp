/*
    Copyright (C) 2025 Carl Hetherington <cth@carlh.net>

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
#include <hpdf.h>


namespace dcp {


class PDFFormatter : public Formatter
{
public:
	PDFFormatter(boost::filesystem::path file);
	~PDFFormatter();

	class Wrap : public Formatter::Wrap
	{
	public:
		Wrap(PDFFormatter* formatter)
			: _formatter(formatter)
		{
			_formatter->indent(1);
		}

		~Wrap()
		{
			_formatter->indent(-1);
		}

	private:
		PDFFormatter* _formatter;
	};

	void heading(std::string const& text) override;
	void subheading(std::string const& text) override;
	std::unique_ptr<Formatter::Wrap> unordered_list() override;
	void list_item(std::string const& text, boost::optional<std::string> type = {}) override;
	std::function<std::string (std::string)> process_string() override;
	std::function<std::string (std::string)> fixed_width() override;
	void finish() override;

	void indent(int amount);

private:
	void add_page();
	void move_down(float spacing);
	void wrapped_text(float x, float first_line_indent, float font_size, dcp::Colour colour, std::string const& text, float width, float line_spacing);

	boost::filesystem::path _file;
	HPDF_Doc _pdf;
	HPDF_Page _page;
	float _y;
	HPDF_Font _normal_font;
	HPDF_Font _fixed_font;
	HPDF_Font _bold_font;

	int _indent = 0;
};


}

