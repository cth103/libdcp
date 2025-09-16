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


#ifndef DCP_VERIFY_REPORT_H
#define DCP_VERIFY_REPORT_H


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


extern void verify_report(std::vector<dcp::VerificationResult> const& results, Formatter& formatter);


}


#endif

