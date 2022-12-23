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
#include <boost/algorithm/string.hpp>
#include <stdio.h>


using namespace dcp;


File::~File()
{
	close();
}


File::File(boost::filesystem::path path, std::string mode)
	: _path(path)
{
#ifdef LIBDCP_WINDOWS
	std::wstring mode_wide(mode.begin(), mode.end());
	/* c_str() here should give a UTF-16 string */
	_file = _wfopen(fix_long_path(path).c_str(), mode_wide.c_str());
#else
        _file = fopen(path.c_str(), mode.c_str());
#endif
}


File::File(File&& other)
	: _path(other._path)
	, _file(other._file)
{
	other._file = nullptr;
}


File&
File::operator=(File&& other)
{
	if (*this != other) {
		close();
		_file = other._file;
		other._file = nullptr;
	}
	return *this;
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
File::gets(char* s, int size)
{
	DCP_ASSERT(_file);
	return fgets(s, size, _file);
}


int
File::puts(char const* s)
{
	DCP_ASSERT(_file);
	return fputs(s, _file);
}


File::operator bool() const
{
	return _file != nullptr;
}


void
File::checked_write(void const * ptr, size_t size)
{
	size_t N = write(ptr, 1, size);
	if (N != size) {
		if (ferror(_file)) {
			throw FileError("fwrite error", _path, errno);
		} else {
			throw FileError("Unexpected short write", _path, 0);
		}
	}
}


void
File::checked_read(void* ptr, size_t size)
{
	size_t N = read(ptr, 1, size);
	if (N != size) {
		if (ferror(_file)) {
			throw FileError("fread error %1", _path, errno);
		} else {
			throw FileError("Unexpected short read", _path, 0);
		}
	}
}


FILE*
File::take()
{
	auto give = _file;
	_file = nullptr;
	return give;
}


int
File::seek(int64_t offset, int whence)
{
	DCP_ASSERT(_file);
#ifdef LIBDCP_WINDOWS
	return _fseeki64(_file, offset, whence);
#else
	return fseek(_file, offset, whence);
#endif
}


int64_t
File::tell()
{
	DCP_ASSERT(_file);
#ifdef LIBDCP_WINDOWS
	return _ftelli64(_file);
#else
	return ftell(_file);
#endif
}



int
File::error ()
{
	DCP_ASSERT(_file);
	return ferror(_file);
}


/** Windows can't "by default" cope with paths longer than 260 characters, so if you pass such a path to
 *  any boost::filesystem method it will fail.  There is a "fix" for this, which is to prepend
 *  the string \\?\ to the path.  This will make it work, so long as:
 *  - the path is absolute.
 *  - the path only uses backslashes.
 *  - individual path components are "short enough" (probably less than 255 characters)
 *
 *  See https://www.boost.org/doc/libs/1_57_0/libs/filesystem/doc/reference.html under
 *  "Warning: Long paths on Windows" for some details.
 *
 *  Our fopen_boost uses this method to get this fix, but any other calls to boost::filesystem
 *  will not unless this method is explicitly called to pre-process the pathname.
 */
boost::filesystem::path
dcp::fix_long_path (boost::filesystem::path long_path)
{
#ifdef LIBDCP_WINDOWS
	using namespace boost::filesystem;

	if (boost::algorithm::starts_with(long_path.string(), "\\\\")) {
		/* This could mean it starts with \\ (i.e. a SMB path) or \\?\ (a long path)
		 * or a variety of other things... anyway, we'll leave it alone.
		 */
		return long_path;
	}

	/* We have to make the path canonical but we can't call canonical() on the long path
	 * as it will fail.  So we'll sort of do it ourselves (possibly badly).
	 */
	path fixed = "\\\\?\\";
	if (long_path.is_absolute()) {
		fixed += long_path.make_preferred();
	} else {
		fixed += boost::filesystem::current_path() / long_path.make_preferred();
	}
	return fixed;
#else
	return long_path;
#endif
}
