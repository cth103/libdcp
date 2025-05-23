/*
    Copyright (C) 2021 Carl Hetherington <cth@carlh.net>

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


#if defined(__clang__)
#define LIBDCP_DISABLE_WARNINGS \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"") \
  _Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"") \
  _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"") \
  _Pragma("GCC diagnostic ignored \"-Waddress\"") \
  _Pragma("GCC diagnostic ignored \"-Wparentheses\"") \
  _Pragma("GCC diagnostic ignored \"-Woverloaded-virtual\"") \
  _Pragma("GCC diagnostic ignored \"-Wtautological-overlap-compare\"")
#else
#if __GNUC__ >= 14
#define LIBDCP_DISABLE_WARNINGS \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"") \
  _Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"") \
  _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"") \
  _Pragma("GCC diagnostic ignored \"-Waddress\"") \
  _Pragma("GCC diagnostic ignored \"-Wparentheses\"") \
  _Pragma("GCC diagnostic ignored \"-Wdeprecated-copy\"") \
  _Pragma("GCC diagnostic ignored \"-Wsuggest-override\"") \
  _Pragma("GCC diagnostic ignored \"-Wunused-function\"") \
  _Pragma("GCC diagnostic ignored \"-Woverloaded-virtual\"") \
  _Pragma("GCC diagnostic ignored \"-Wignored-qualifiers\"") \
  _Pragma("GCC diagnostic ignored \"-Wtemplate-id-cdtor\"")
#elif __GNUC__ >= 9
#define LIBDCP_DISABLE_WARNINGS \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"") \
  _Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"") \
  _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"") \
  _Pragma("GCC diagnostic ignored \"-Waddress\"") \
  _Pragma("GCC diagnostic ignored \"-Wparentheses\"") \
  _Pragma("GCC diagnostic ignored \"-Wdeprecated-copy\"") \
  _Pragma("GCC diagnostic ignored \"-Wsuggest-override\"") \
  _Pragma("GCC diagnostic ignored \"-Wunused-function\"") \
  _Pragma("GCC diagnostic ignored \"-Woverloaded-virtual\"") \
  _Pragma("GCC diagnostic ignored \"-Wignored-qualifiers\"")
#elif __GNUC__ >= 5
#define LIBDCP_DISABLE_WARNINGS \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"") \
  _Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"") \
  _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"") \
  _Pragma("GCC diagnostic ignored \"-Waddress\"") \
  _Pragma("GCC diagnostic ignored \"-Wparentheses\"") \
  _Pragma("GCC diagnostic ignored \"-Wsuggest-override\"")
#else
#define LIBDCP_DISABLE_WARNINGS \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"") \
  _Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"") \
  _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"") \
  _Pragma("GCC diagnostic ignored \"-Waddress\"") \
  _Pragma("GCC diagnostic ignored \"-Wparentheses\"")
#endif
#endif

#define LIBDCP_ENABLE_WARNINGS \
  _Pragma("GCC diagnostic pop")

