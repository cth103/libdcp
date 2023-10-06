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


#ifndef LIBDCP_FILE_H
#define LIBDCP_FILE_H


#include <boost/filesystem/path.hpp>


namespace dcp {


/** A wrapper for stdio files that gives RAII and allows us to open files with
 *  UTF-8 names on Windows.
 */
class File
{
public:
	/** @param path Path to open
	 *  @param mode mode flags, as for fopen(3)
	 */
	File(boost::filesystem::path, std::string mode);

	File(File&& other);
	File& operator=(File&& other);

	~File();

	File(File const&) = delete;
	File& operator=(File const&) = delete;

	operator bool() const;

	/** fwrite() wrapper */
	size_t write(const void *ptr, size_t size, size_t nmemb);
	/** fread() wrapper */
	size_t read(void *ptr, size_t size, size_t nmemb);
	/** feof() wrapper */
	int eof();
	/** fgets() wrapper */
	char *gets(char *s, int size);
	/** fputs() wrapper */
	int puts(char const *s);
	/** fseek/fseeki64 wrapper */
	int seek(int64_t offset, int whence);
	/** ftell/ftelli64 wrapper */
	int64_t tell();
	/** ferror wrapper */
	int error();

	void checked_write(void const * ptr, size_t size);
	void checked_read(void* ptr, size_t size);

	/** Close the file; it is not necessary to call this as the
	 *  destructor will do it if required.
	 */
	void close();

	boost::filesystem::path path() const {
		return _path;
	}

	/** Take ownership of the underlying FILE*;
	 *  the File object will not closed it after this call.
	 */
	FILE* take();

	FILE* get() {
		return _file;
	}

	/** @return Error returned by the fopen / _wfopen call;
	 *  errno on POSIX, GetLastError() on Windows.
	 */
	int open_error() const {
		return _open_error;
	}

private:
	boost::filesystem::path _path;
	FILE* _file = nullptr;
	int _open_error = 0;
};


}


#endif
