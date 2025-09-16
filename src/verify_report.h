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
	class Wrap
	{
	public:
		virtual ~Wrap() {}
	};

	virtual std::unique_ptr<Wrap> document() { return {}; }

	virtual void heading(std::string const& text) = 0;
	virtual void subheading(std::string const& text) = 0;
	virtual std::unique_ptr<Wrap> body() { return {}; }

	virtual std::unique_ptr<Wrap> unordered_list() = 0;
	virtual void list_item(std::string const& text, boost::optional<std::string> type = {}) = 0;

	virtual std::function<std::string (std::string)> process_string() = 0;
	virtual std::function<std::string (std::string)> fixed_width() = 0;

	virtual void finish() {}
};


extern void verify_report(std::vector<dcp::VerificationResult> const& results, Formatter& formatter);


}


#endif

