/*
    Copyright (C) 2012-2021 Carl Hetherington <cth@carlh.net>

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


#include "filesystem.h"
#include <boost/algorithm/string.hpp>


bool
dcp::filesystem::exists(boost::filesystem::path const& path)
{
	return boost::filesystem::exists(dcp::filesystem::fix_long_path(path));
}


bool
dcp::filesystem::exists(boost::filesystem::path const& path, boost::system::error_code& ec)
{
	return boost::filesystem::exists(dcp::filesystem::fix_long_path(path), ec);
}


bool
dcp::filesystem::is_directory(boost::filesystem::path const& path)
{
	return boost::filesystem::is_directory(dcp::filesystem::fix_long_path(path));
}


bool
dcp::filesystem::is_empty(boost::filesystem::path const& path)
{
	return boost::filesystem::is_empty(dcp::filesystem::fix_long_path(path));
}


bool
dcp::filesystem::is_regular_file(boost::filesystem::path const& path)
{
	return boost::filesystem::is_regular_file(dcp::filesystem::fix_long_path(path));
}


bool
dcp::filesystem::create_directory(boost::filesystem::path const& path)
{
	return boost::filesystem::create_directory(dcp::filesystem::fix_long_path(path));
}


bool
dcp::filesystem::create_directory(boost::filesystem::path const& path, boost::system::error_code& ec)
{
	return boost::filesystem::create_directory(dcp::filesystem::fix_long_path(path), ec);
}


void
dcp::filesystem::copy(boost::filesystem::path const& from, boost::filesystem::path const& to)
{
	boost::filesystem::copy(dcp::filesystem::fix_long_path(from), dcp::filesystem::fix_long_path(to));
}


void
dcp::filesystem::copy_file(boost::filesystem::path const& from, boost::filesystem::path const& to)
{
	boost::filesystem::copy_file(dcp::filesystem::fix_long_path(from), dcp::filesystem::fix_long_path(to));
}


void
dcp::filesystem::copy_file(boost::filesystem::path const& from, boost::filesystem::path const& to, boost::system::error_code& ec)
{
	boost::filesystem::copy_file(dcp::filesystem::fix_long_path(from), dcp::filesystem::fix_long_path(to), ec);
}


void
dcp::filesystem::copy_file(boost::filesystem::path const& from, boost::filesystem::path const& to, CopyOptions option)
{
#ifdef LIBDCP_HAVE_COPY_OPTIONS
	auto const options = option == CopyOptions::OVERWRITE_EXISTING ? boost::filesystem::copy_options::overwrite_existing : boost::filesystem::copy_options::none;
#else
	auto const options = option == CopyOptions::OVERWRITE_EXISTING ? boost::filesystem::copy_option::overwrite_if_exists : boost::filesystem::copy_option::none;
#endif

	boost::filesystem::copy_file(dcp::filesystem::fix_long_path(from), dcp::filesystem::fix_long_path(to), options);
}


bool
dcp::filesystem::create_directories(boost::filesystem::path const& path)
{
	return boost::filesystem::create_directories(dcp::filesystem::fix_long_path(path));
}


bool
dcp::filesystem::create_directories(boost::filesystem::path const& path, boost::system::error_code& ec)
{
	return boost::filesystem::create_directories(dcp::filesystem::fix_long_path(path), ec);
}


boost::filesystem::path
dcp::filesystem::absolute(boost::filesystem::path const& path)
{
	return dcp::filesystem::unfix_long_path(boost::filesystem::absolute(dcp::filesystem::fix_long_path(path)));
}


boost::filesystem::path
dcp::filesystem::canonical(boost::filesystem::path const& path)
{
	return dcp::filesystem::unfix_long_path(boost::filesystem::canonical(dcp::filesystem::fix_long_path(path)));
}


boost::filesystem::path
dcp::filesystem::weakly_canonical(boost::filesystem::path const& path)
{
#ifdef DCPOMATIC_HAVE_WEAKLY_CANONICAL
	return dcp::filesystem::unfix_long_path(boost::filesystem::weakly_canonical(dcp::filesystem::fix_long_path(path)));
#else
	boost::filesystem::path complete(boost::filesystem::system_complete(dcp::filesystem::fix_long_path(path)));
	boost::filesystem::path result;
	for (auto part: complete) {
		if (part == "..") {
			boost::system::error_code ec;
			if (boost::filesystem::is_symlink(result, ec) || result.filename() == "..") {
				result /= part;
			} else {
				result = result.parent_path();
			}
		} else if (part != ".") {
			result /= part;
		}
	}

	return dcp::filesystem::unfix_long_path(result.make_preferred());
#endif
}


bool
dcp::filesystem::remove(boost::filesystem::path const& path)
{
	return boost::filesystem::remove(dcp::filesystem::fix_long_path(path));
}


bool
dcp::filesystem::remove(boost::filesystem::path const& path, boost::system::error_code& ec)
{
	return boost::filesystem::remove(dcp::filesystem::fix_long_path(path), ec);
}


uintmax_t
dcp::filesystem::remove_all(boost::filesystem::path const& path)
{
	return boost::filesystem::remove_all(dcp::filesystem::fix_long_path(path));
}


uintmax_t
dcp::filesystem::remove_all(boost::filesystem::path const& path, boost::system::error_code& ec)
{
	return boost::filesystem::remove_all(dcp::filesystem::fix_long_path(path), ec);
}


uintmax_t
dcp::filesystem::file_size(boost::filesystem::path const& path)
{
	return boost::filesystem::file_size(dcp::filesystem::fix_long_path(path));
}


uintmax_t
dcp::filesystem::file_size(boost::filesystem::path const& path, boost::system::error_code& ec)
{
	return boost::filesystem::file_size(dcp::filesystem::fix_long_path(path), ec);
}


boost::filesystem::path
dcp::filesystem::current_path()
{
	return dcp::filesystem::unfix_long_path(boost::filesystem::current_path());
}


void
dcp::filesystem::current_path(boost::filesystem::path const& path)
{
	boost::filesystem::current_path(dcp::filesystem::fix_long_path(path));
}


void
dcp::filesystem::create_hard_link(boost::filesystem::path const& from, boost::filesystem::path const& to)
{
	boost::filesystem::create_hard_link(dcp::filesystem::fix_long_path(from), dcp::filesystem::fix_long_path(to));
}


void
dcp::filesystem::create_hard_link(boost::filesystem::path const& from, boost::filesystem::path const& to, boost::system::error_code& ec)
{
	boost::filesystem::create_hard_link(dcp::filesystem::fix_long_path(from), dcp::filesystem::fix_long_path(to), ec);
}


void
dcp::filesystem::create_symlink(boost::filesystem::path const& from, boost::filesystem::path const& to, boost::system::error_code& ec)
{
	boost::filesystem::create_symlink(dcp::filesystem::fix_long_path(from), dcp::filesystem::fix_long_path(to), ec);
}


std::string
dcp::filesystem::extension(boost::filesystem::path const& path)
{
	return dcp::filesystem::fix_long_path(path).extension().string();
}


boost::filesystem::space_info
dcp::filesystem::space(boost::filesystem::path const& path)
{
	return boost::filesystem::space(dcp::filesystem::fix_long_path(path));
}


std::time_t
dcp::filesystem::last_write_time(boost::filesystem::path const& path)
{
	return boost::filesystem::last_write_time(dcp::filesystem::fix_long_path(path));
}


std::time_t
dcp::filesystem::last_write_time(boost::filesystem::path const& path, boost::system::error_code& ec)
{
	return boost::filesystem::last_write_time(dcp::filesystem::fix_long_path(path), ec);
}


uintmax_t
dcp::filesystem::hard_link_count(boost::filesystem::path const& path)
{
	return boost::filesystem::hard_link_count(dcp::filesystem::fix_long_path(path));
}


void
dcp::filesystem::rename(boost::filesystem::path const& old_path, boost::filesystem::path const& new_path)
{
	boost::filesystem::rename(dcp::filesystem::fix_long_path(old_path), dcp::filesystem::fix_long_path(new_path));
}


void
dcp::filesystem::rename(boost::filesystem::path const& old_path, boost::filesystem::path const& new_path, boost::system::error_code& ec)
{
	boost::filesystem::rename(dcp::filesystem::fix_long_path(old_path), dcp::filesystem::fix_long_path(new_path), ec);
}


/* We don't really need this but let's add it for completeness */
boost::filesystem::path
dcp::filesystem::change_extension(boost::filesystem::path const& path, std::string const& new_extension)
{
#ifdef LIBDCP_HAVE_REPLACE_EXTENSION
	auto copy = path;
	copy.replace_extension(new_extension);
	return copy;
#else
	return boost::filesystem::change_extension(path, new_extension);
#endif
}


#ifdef DCPOMATIC_WINDOWS

dcp::filesystem::directory_iterator::directory_iterator(boost::filesystem::path const& path)
	: _wrapped(dcp::filesystem::fix_long_path(path))
{

}


dcp::filesystem::directory_iterator::directory_iterator(boost::filesystem::path const& path, boost::system::error_code& ec)
	: _wrapped(dcp::filesystem::fix_long_path(path), ec)
{

}


boost::filesystem::path
dcp::filesystem::directory_entry::path() const
{
	return dcp::filesystem::unfix_long_path(_path);
}


dcp::filesystem::directory_entry::operator boost::filesystem::path const &() const
{
	return dcp::filesystem::unfix_long_path(_path);
}


dcp::filesystem::recursive_directory_iterator::recursive_directory_iterator(boost::filesystem::path const& path)
	: _wrapped(dcp::filesystem::fix_long_path(path))
{

}

#else

dcp::filesystem::directory_iterator::directory_iterator(boost::filesystem::path const& path)
	: _wrapped(path)
{

}


dcp::filesystem::directory_iterator::directory_iterator(boost::filesystem::path const& path, boost::system::error_code& ec)
	: _wrapped(path, ec)
{

}


boost::filesystem::path
dcp::filesystem::directory_entry::path() const
{
	return _path;
}


dcp::filesystem::directory_entry::operator boost::filesystem::path const &() const
{
	return _path;
}


dcp::filesystem::recursive_directory_iterator::recursive_directory_iterator(boost::filesystem::path const& path)
	: _wrapped(path)
{

}

#endif


dcp::filesystem::directory_entry::directory_entry(boost::filesystem::path const& path)
	: _path(path)
{

}


dcp::filesystem::directory_iterator&
dcp::filesystem::directory_iterator::operator++()
{
	++_wrapped;
	return *this;
}


dcp::filesystem::directory_entry
dcp::filesystem::directory_iterator::operator*() const
{
	_entry = dcp::filesystem::directory_entry(*_wrapped);
	return _entry;
}


dcp::filesystem::directory_entry*
dcp::filesystem::directory_iterator::operator->() const
{
	_entry = dcp::filesystem::directory_entry(_wrapped->path());
	return &_entry;
}

bool
dcp::filesystem::directory_iterator::operator!=(dcp::filesystem::directory_iterator const& other) const
{
	return _wrapped != other._wrapped;
}


dcp::filesystem::directory_iterator const&
dcp::filesystem::begin(dcp::filesystem::directory_iterator const& iter)
{
	return iter;
}


dcp::filesystem::directory_iterator
dcp::filesystem::end(dcp::filesystem::directory_iterator const&)
{
	return dcp::filesystem::directory_iterator();
}


dcp::filesystem::recursive_directory_iterator&
dcp::filesystem::recursive_directory_iterator::operator++()
{
	++_wrapped;
	return *this;
}


bool
dcp::filesystem::recursive_directory_iterator::operator!=(dcp::filesystem::recursive_directory_iterator const& other) const
{
	return _wrapped != other._wrapped;
}


dcp::filesystem::directory_entry
dcp::filesystem::recursive_directory_iterator::operator*() const
{
	_entry = dcp::filesystem::directory_entry(_wrapped->path());
	return _entry;
}


dcp::filesystem::directory_entry*
dcp::filesystem::recursive_directory_iterator::operator->() const
{
	_entry = dcp::filesystem::directory_entry(_wrapped->path());
	return &_entry;
}


dcp::filesystem::recursive_directory_iterator const&
dcp::filesystem::begin(dcp::filesystem::recursive_directory_iterator const& iter)
{
	return iter;
}


dcp::filesystem::recursive_directory_iterator
dcp::filesystem::end(dcp::filesystem::recursive_directory_iterator const&)
{
	return dcp::filesystem::recursive_directory_iterator();
}


/** Windows can't "by default" cope with paths longer than 260 characters, so if you pass such a path to
 *  any boost::filesystem method it will fail.  There is a "fix" for this, which is to prepend
 *  the string \\?\ to the path.  This will make it work, so long as:
 *  - the path is absolute.
 *  - the path contains no .. parts.
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
dcp::filesystem::fix_long_path(boost::filesystem::path long_path)
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
		fixed += filesystem::current_path() / long_path.make_preferred();
	}
	return fixed;
#else
	return long_path;
#endif
}


boost::filesystem::path
dcp::filesystem::unfix_long_path(boost::filesystem::path long_path)
{
#ifdef LIBDCP_WINDOWS
	if (boost::algorithm::starts_with(long_path.string(), "\\\\?\\")) {
		return long_path.string().substr(4);
	}
#endif
	return long_path;
}
