/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <boost/filesystem.hpp>

namespace xmlpp {
	class Element;
}

extern boost::filesystem::path private_test;
extern void check_xml (xmlpp::Element* ref, xmlpp::Element* test, std::list<std::string> ignore);
extern void check_xml (std::string ref, std::string test, std::list<std::string> ignore);
extern void check_file (boost::filesystem::path ref, boost::filesystem::path check);
