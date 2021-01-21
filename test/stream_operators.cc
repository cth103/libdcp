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


/** @file  test/stream_operators.cc
 *  @brief A collection of operator<< methods so that we can use boost test macros with libdcp's types.
 */


#include "stream_operators.h"
#include "types.h"
#include "verify.h"
#include <iostream>


using std::ostream;


ostream&
dcp::operator<< (ostream& s, Size const & a)
{
	s << a.width << "x" << a.height;
	return s;
}


ostream&
dcp::operator<< (ostream& s, Fraction const & f)
{
	s << f.numerator << "/" << f.denominator;
	return s;
}


ostream &
dcp::operator<< (ostream& s, Colour const & c)
{
	s << "(" << c.r << ", " << c.g << ", " << c.b << ")";
	return s;
}


ostream&
dcp::operator<< (std::ostream& s, Effect e)
{
	s << effect_to_string(e);
	return s;
}


ostream&
dcp::operator<< (ostream& s, ContentKind c)
{
	s << content_kind_to_string(c);
	return s;
}


ostream &
dcp::operator<< (ostream& s, Rating const & r)
{
	s << r.agency << " " << r.label;
	return s;
}


ostream&
dcp::operator<<(ostream& s, Status t)
{
	s << status_to_string(t);
	return s;
}


ostream&
dcp::operator<<(ostream& s, Channel c)
{
	switch (c) {
	case Channel::LEFT:
		s << "left(0)";
		break;
	case Channel::RIGHT:
		s << "right(1)";
		break;
	case Channel::CENTRE:
		s << "centre(2)";
		break;
	case Channel::LFE:
		s << "lfe(3)";
		break;
	case Channel::LS:
		s << "ls(4)";
		break;
	case Channel::RS:
		s << "rs(5)";
		break;
	case Channel::HI:
		s << "hi(6)";
		break;
	case Channel::VI:
		s << "vi(7)";
		break;
	case Channel::BSL:
		s << "bsl(10)";
		break;
	case Channel::BSR:
		s << "bsr(11)";
		break;
	case Channel::MOTION_DATA:
		s << "motion_data(12)";
		break;
	case Channel::SYNC_SIGNAL:
		s << "sync_signal(13)";
		break;
	case Channel::SIGN_LANGUAGE:
		s << "sign_language(14)";
		break;
	case Channel::CHANNEL_COUNT:
		s << "(16)";
		break;
	}
	return s;
}


ostream&
dcp::operator<< (ostream& s, NoteType t)
{
	switch (t) {
	case NoteType::PROGRESS:
		s << "progress";
		break;
	case NoteType::ERROR:
		s << "error";
		break;
	case NoteType::NOTE:
		s << "note";
		break;
	}
	return s;
}


ostream&
dcp::operator<< (ostream& s, MCASoundField f)
{
	switch (f) {
	case MCASoundField::FIVE_POINT_ONE:
		s << "5.1";
		break;
	case MCASoundField::SEVEN_POINT_ONE:
		s << "7.1";
		break;
	}
	return s;
}


ostream&
dcp::operator<< (ostream& s, Standard t)
{
	switch (t) {
	case Standard::INTEROP:
		s << "interop";
		break;
	case Standard::SMPTE:
		s << "smpte";
		break;
	}
	return s;
}


ostream&
dcp::operator<< (ostream& s, VerificationNote::Type t)
{
	switch (t) {
	case VerificationNote::Type::ERROR:
		s << "error";
		break;
	case VerificationNote::Type::BV21_ERROR:
		s << "bv21_error";
		break;
	case VerificationNote::Type::WARNING:
		s << "warning";
		break;
	}
	return s;
}


ostream&
dcp::operator<< (ostream& s, VerificationNote::Code c)
{
	s << static_cast<int>(c);
	return s;
}
