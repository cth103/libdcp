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


#include "exceptions.h"
#include "pdf_formatter.h"
#include <fmt/format.h>


using std::unique_ptr;
using namespace dcp;


int constexpr dpi = 72;

/* These are in inches */
float constexpr page_width = 8.27;
float constexpr horizontal_margin = 0.5;
float constexpr page_height = 11.69;
float constexpr vertical_margin = 1.0;


static
void
error_handler(HPDF_STATUS error, HPDF_STATUS detail, void*)
{
	throw MiscError(fmt::format("Could not create PDF {} {}", error, detail));
}


PDFFormatter::PDFFormatter(boost::filesystem::path file)
	: _file(file)
	, _pdf(HPDF_New(error_handler, nullptr))
	, _normal_font(HPDF_GetFont(_pdf, "Helvetica", nullptr))
	, _fixed_font(HPDF_GetFont(_pdf, "Courier", nullptr))
	, _bold_font(HPDF_GetFont(_pdf, "Helvetica-Bold", nullptr))
{
	add_page();
}


PDFFormatter::~PDFFormatter()
{
	HPDF_Free(_pdf);
}


void
PDFFormatter::add_page()
{
	_page = HPDF_AddPage(_pdf);
	HPDF_Page_Concat(_page, 1, 0, 0, 1, horizontal_margin * dpi, (page_height - vertical_margin) * dpi);
	_y = 0;
}


void
PDFFormatter::heading(std::string const& text)
{
	move_down(16 * 1.4);
	HPDF_Page_SetFontAndSize(_page, _bold_font, 20);
	HPDF_Page_SetRGBFill(_page, 0, 0, 0);
	HPDF_Page_BeginText(_page);
	HPDF_Page_TextOut(_page, 0, _y, text.c_str());
	HPDF_Page_EndText(_page);
	move_down(20 * 1.4);
}


void
PDFFormatter::subheading(std::string const& text)
{
	move_down(12 * 1.4);
	HPDF_Page_SetFontAndSize(_page, _bold_font, 16);
	HPDF_Page_SetRGBFill(_page, 0, 0, 0);
	HPDF_Page_BeginText(_page);
	HPDF_Page_TextOut(_page, 0, _y, text.c_str());
	HPDF_Page_EndText(_page);
	move_down(16 * 1.4);
}


unique_ptr<Formatter::Wrap>
PDFFormatter::unordered_list()
{
	return unique_ptr<PDFFormatter::Wrap>(new PDFFormatter::Wrap(this));
}


void
PDFFormatter::move_down(float spacing)
{
	_y -= spacing;
	if (_y < ((-page_height + vertical_margin * 2) * dpi)) {
		add_page();
	}
}


void
PDFFormatter::wrapped_text(float x, float first_line_indent, float font_size, dcp::Colour colour, std::string const& text, float width, float line_spacing)
{
	class Block
	{
	public:
		enum class Style {
			NORMAL,
			FIXED
		};

		Block(std::string text_, Style style_)
			: text(text_)
			, style(style_)
		{}

		std::string text;
		Style style;
	};

	std::map<std::string, Block::Style> tags = {
		{ "[fixed]", Block::Style::FIXED },
		{ "[/fixed]", Block::Style::NORMAL }
	};

	std::vector<Block> blocks;
	Block block("", Block::Style::NORMAL);
	auto work = text;
	while (true) {
		std::pair<std::string, Block::Style> found_tag;
		auto found_start = std::string::npos;

		for (auto const& tag: tags) {
			auto const start = work.find(tag.first);
			if (start < found_start) {
				found_tag = tag;
				found_start = start;
			}
		}

		if (found_start == std::string::npos) {
			break;
		}

		if (found_start > 0) {
			block.text += work.substr(0, found_start);
			blocks.push_back(block);
			block = Block("", found_tag.second);
		}
		work = work.substr(found_start + found_tag.first.length());
	}

	if (!work.empty()) {
		block.text += work;
		blocks.push_back(block);
	}

	std::map<Block::Style, HPDF_Font*> fonts = {
		{ Block::Style::NORMAL, &_normal_font },
		{ Block::Style::FIXED, &_fixed_font }
	};

	auto px = x + first_line_indent;
	for (auto const& block: blocks) {

		int offset = 0;
		int left = block.text.length();
		while (left) {
			HPDF_Page_SetFontAndSize(_page, *fonts[block.style], font_size);

			HPDF_REAL text_width;
			int fits = HPDF_Font_MeasureText(
				*fonts[block.style],
				reinterpret_cast<unsigned char const*>(block.text.substr(offset).c_str()),
				block.text.length() - offset,
				width - px,
				font_size,
				0,
				0,
				true,
				&text_width);

			if (fits == 0) {
				/* Try again without word-wrap */
				fits = HPDF_Font_MeasureText(
					*fonts[block.style],
					reinterpret_cast<unsigned char const*>(block.text.substr(offset).c_str()),
					block.text.length() - offset,
					width - px,
					font_size,
					0,
					0,
					false,
					&text_width);
			}

			HPDF_Page_SetRGBFill(_page, colour.r / 255.0, colour.g / 255.0, colour.b / 255.0);
			HPDF_Page_BeginText(_page);
			HPDF_Page_TextOut(_page, px, _y, block.text.substr(offset, fits).c_str());
			HPDF_Page_EndText(_page);
			if (fits < static_cast<int>(block.text.length()) - offset) {
				px = x;
				move_down(line_spacing);
			} else {
				px += text_width;
			}

			offset += fits;
			left -= fits;
		}
	}

	move_down(line_spacing);
}


void
PDFFormatter::list_item(std::string const& text, boost::optional<std::string> type)
{
	float const indent = 16 * _indent;
	float constexpr dot_radius = 1.5;
	float constexpr font_size = 10;

	HPDF_Page_Circle(_page, indent + dot_radius, _y + font_size / 3, dot_radius);
	HPDF_Page_Fill(_page);

	dcp::Colour colour(0, 0, 0);
	if (type && *type == "ok") {
		colour = dcp::Colour(0, 153, 0);
	} else if (type && *type == "warning") {
		colour = dcp::Colour(255, 127, 102);
	} else if (type && (*type == "error" || *type == "bv21-error")) {
		colour = dcp::Colour(153, 0, 0);
	}

	wrapped_text(indent, dot_radius * 6, font_size, colour, text, (page_width - horizontal_margin * 2) * dpi, font_size * 1.2);
}


std::function<std::string (std::string)>
PDFFormatter::process_string()
{
	return [](std::string s) {
		return s;
	};
}


std::function<std::string (std::string)>
PDFFormatter::fixed_width()
{
	return [](std::string s) {
		return String::compose("[fixed]%1[/fixed]", s);
	};
}


void
PDFFormatter::finish()
{
	HPDF_SaveToFile(_pdf, _file.string().c_str());
}


void
PDFFormatter::indent(int amount)
{
	_indent += amount;
}

