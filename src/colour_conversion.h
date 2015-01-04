/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#include <boost/shared_ptr.hpp>
#include <boost/numeric/ublas/matrix.hpp>

namespace dcp {

class TransferFunction;

class ColourConversion
{
public:
	ColourConversion ()
		: _matrix (3, 3)
	{}
	
	ColourConversion (
		boost::shared_ptr<const TransferFunction> in,
		double const matrix[3][3],
		boost::shared_ptr<const TransferFunction> out
		);

	boost::shared_ptr<const TransferFunction> in () const {
		return _in;
	}

	boost::numeric::ublas::matrix<double> matrix () const {
		return _matrix;
	}

	boost::shared_ptr<const TransferFunction> out () const {
		return _out;
	}

	void set_in (boost::shared_ptr<const TransferFunction> f) {
		_in = f;
	}

	void set_matrix (boost::numeric::ublas::matrix<double> m) {
		_matrix = m;
	}

	void set_out (boost::shared_ptr<const TransferFunction> f) {
		_out = f;
	}

	bool about_equal (ColourConversion const & other, float epsilon) const;

	static ColourConversion const & srgb_to_xyz ();
	static ColourConversion const & xyz_to_srgb ();
	static ColourConversion const & rec709_to_xyz ();

protected:
	boost::shared_ptr<const TransferFunction> _in;
	boost::numeric::ublas::matrix<double> _matrix;
	boost::shared_ptr<const TransferFunction> _out;
};

}
