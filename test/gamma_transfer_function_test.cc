/*
    Copyright (C) 2015 Carl Hetherington <cth@carlh.net>

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

#include "gamma_transfer_function.h"
#include "modified_gamma_transfer_function.h"
#include <boost/test/unit_test.hpp>

using boost::shared_ptr;

/** Check GammaTransferFunction::about_equal */
BOOST_AUTO_TEST_CASE (gamma_transfer_function_test)
{
	shared_ptr<dcp::GammaTransferFunction> a (new dcp::GammaTransferFunction (1.2));
	shared_ptr<dcp::GammaTransferFunction> b (new dcp::GammaTransferFunction (1.2));
	BOOST_CHECK (a->about_equal (b, 1e-6));

	a.reset (new dcp::GammaTransferFunction (1.2));
	a.reset (new dcp::GammaTransferFunction (1.3));
	BOOST_CHECK (a->about_equal (b, 0.2));
	BOOST_CHECK (!a->about_equal (b, 0.05));

	shared_ptr<dcp::ModifiedGammaTransferFunction> c (new dcp::ModifiedGammaTransferFunction (2.4, 0.05, 1, 2));
	BOOST_CHECK (!a->about_equal (c, 1));
}
