/*
    Copyright (C) 2022 Carl Hetherington <cth@carlh.net>

    This file is part of libdcp.

    libdcp is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    libdcp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdcp.  If not, see <http://www.gnu.org/licenses/>.

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
*/


#include "dcp_assert.h"
#include "file.h"


using namespace dcp;


File::~File()
{
	close();
}


File::File(boost::filesystem::path path, std::string mode)
{
#ifdef LIBDCP_WINDOWS
	std::wstring mode_wide(mode.begin(), mode.end());
	/* c_str() here should give a UTF-16 string */
        _file = _wfopen(path.c_str(), mode_wide.c_str());
#else
        _file = fopen(path.c_str(), mode.c_str());
#endif
}


void
File::close()
{
	if (_file) {
		fclose(_file);
		_file = nullptr;
	}
}


size_t
File::write(const void *ptr, size_t size, size_t nmemb)
{
	DCP_ASSERT(_file);
	return fwrite(ptr, size, nmemb, _file);
}


size_t
File::read(void *ptr, size_t size, size_t nmemb)
{
	DCP_ASSERT(_file);
	return fread(ptr, size, nmemb, _file);
}


int
File::eof()
{
	DCP_ASSERT(_file);
	return feof(_file);
}


char *
File::gets(char *s, int size)
{
	DCP_ASSERT(_file);
	return fgets(s, size, _file);
}


File::operator bool() const
{
	return _file != nullptr;
}

