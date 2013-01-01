#ifndef LIBDCP_CERTIFICATES_H
#define LIBDCP_CERTIFICATES_H

#include <string>
#include <list>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <openssl/x509.h>

class certificates;

namespace libdcp {

class Certificate : public boost::noncopyable
{
public:
	Certificate (X509 *);
	~Certificate ();

	std::string issuer () const;
	std::string serial () const;
	std::string subject () const;

	static std::string name_for_xml (std::string const &);

private:
	X509* _certificate;
};

class CertificateChain
{
public:
	CertificateChain () {}
	CertificateChain (std::string const &);

	boost::shared_ptr<Certificate> root () const;
	boost::shared_ptr<Certificate> leaf () const;

private:
	friend class ::certificates;
	std::list<boost::shared_ptr<Certificate> > _certificates;
};

}

#endif
