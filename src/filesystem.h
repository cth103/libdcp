/*
    Copyright (C) 2012-2023 Carl Hetherington <cth@carlh.net>

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


#ifndef LIBDCP_FILESYSTEM_H
#define LIBDCP_FILESYSTEM_H


#include <boost/filesystem.hpp>


namespace dcp
{
namespace filesystem
{

boost::filesystem::path absolute(boost::filesystem::path const& path);
boost::filesystem::path canonical(boost::filesystem::path const& path);
boost::filesystem::path weakly_canonical(boost::filesystem::path const& path);
boost::filesystem::path change_extension(boost::filesystem::path const& from, std::string const& new_extension);
void copy(boost::filesystem::path const& from, boost::filesystem::path const& to);
void copy_file(boost::filesystem::path const& from, boost::filesystem::path const& to);
void copy_file(boost::filesystem::path const& from, boost::filesystem::path const& to, boost::system::error_code& ec);

enum class CopyOptions
{
	NONE,
	OVERWRITE_EXISTING
};

void copy_file(boost::filesystem::path const& from, boost::filesystem::path const& to, CopyOptions options);
bool create_directory(boost::filesystem::path const& path);
bool create_directory(boost::filesystem::path const& path, boost::system::error_code& ec);
bool create_directories(boost::filesystem::path const& path);
bool create_directories(boost::filesystem::path const& path, boost::system::error_code& ec);
void create_hard_link(boost::filesystem::path const& from, boost::filesystem::path const& to);
void create_hard_link(boost::filesystem::path const& from, boost::filesystem::path const& to, boost::system::error_code& ec);
void create_symlink(boost::filesystem::path const& from, boost::filesystem::path const& to, boost::system::error_code& ec);
void current_path(boost::filesystem::path const& path);
boost::filesystem::path current_path();
bool exists(boost::filesystem::path const& path);
bool exists(boost::filesystem::path const& path, boost::system::error_code& ec);
std::string extension(boost::filesystem::path const& path);
bool is_directory(boost::filesystem::path const& path);
bool is_empty(boost::filesystem::path const& path);
bool is_regular_file(boost::filesystem::path const& path);
uintmax_t file_size(boost::filesystem::path const& path);
uintmax_t file_size(boost::filesystem::path const& path, boost::system::error_code& ec);
uintmax_t hard_link_count(boost::filesystem::path const& path);
std::time_t last_write_time(boost::filesystem::path const& path);
std::time_t last_write_time(boost::filesystem::path const& path, boost::system::error_code& ec);
bool remove(boost::filesystem::path const& path);
bool remove(boost::filesystem::path const& path, boost::system::error_code& ec);
uintmax_t remove_all(boost::filesystem::path const& path);
uintmax_t remove_all(boost::filesystem::path const& path, boost::system::error_code& ec);
void rename(boost::filesystem::path const& old_path, boost::filesystem::path const& new_path);
void rename(boost::filesystem::path const& old_path, boost::filesystem::path const& new_path, boost::system::error_code& ec);
boost::filesystem::space_info space(boost::filesystem::path const& path);


class directory_entry
{
public:
	directory_entry() {}
	directory_entry(boost::filesystem::path const& path);

	boost::filesystem::path path() const;
	operator boost::filesystem::path const&() const;

private:
	boost::filesystem::path _path;
};


class directory_iterator
{
public:
	directory_iterator() = default;
	directory_iterator(boost::filesystem::path const& path);
	directory_iterator(boost::filesystem::path const& path, boost::system::error_code& ec);

	directory_iterator& operator++();
	directory_entry operator*() const;
	directory_entry* operator->() const;
	bool operator!=(directory_iterator const& other) const;

private:
	boost::filesystem::directory_iterator _wrapped;
	mutable directory_entry _entry;
};


directory_iterator const& begin(directory_iterator const& iter);
directory_iterator end(directory_iterator const&);



class recursive_directory_iterator
{
public:
	recursive_directory_iterator() = default;
	recursive_directory_iterator(boost::filesystem::path const& path);

	recursive_directory_iterator& operator++();
	directory_entry operator*() const;
	directory_entry* operator->() const;
	bool operator!=(recursive_directory_iterator const& other) const;

private:
	boost::filesystem::recursive_directory_iterator _wrapped;
	mutable directory_entry _entry;
};


recursive_directory_iterator const& begin(recursive_directory_iterator const& iter);
recursive_directory_iterator end(recursive_directory_iterator const&);


boost::filesystem::path fix_long_path(boost::filesystem::path long_path);
boost::filesystem::path unfix_long_path(boost::filesystem::path long_path);

}
}


#endif
