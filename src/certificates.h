/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_CERTIFICATES_H
#define LIBDCP_CERTIFICATES_H

#include <string>
#include <list>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <openssl/x509.h>

class certificates;

namespace xmlpp {
	class Element;
}

namespace libdcp {

class Certificate : public boost::noncopyable
{
public:
	Certificate ()
		: _certificate (0)
	{}

	Certificate (std::string const &);
	Certificate (X509 *);
	~Certificate ();

	std::string certificate () const;
	std::string issuer () const;
	std::string serial () const;
	std::string subject () const;

	std::string thumbprint () const;

	static std::string name_for_xml (std::string const &);

private:
	X509* _certificate;
};

class CertificateChain
{
public:
	CertificateChain () {}

	void add (boost::shared_ptr<Certificate>);

	boost::shared_ptr<Certificate> root () const;
	boost::shared_ptr<Certificate> leaf () const;

	std::list<boost::shared_ptr<Certificate> > leaf_to_root () const;

private:
	friend class ::certificates;
	std::list<boost::shared_ptr<Certificate> > _certificates;
};

}

#endif
