#include <sstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/asn1.h>
#include "certificates.h"
#include "exceptions.h"

using std::list;
using std::string;
using std::stringstream;
using std::vector;
using boost::shared_ptr;
using namespace libdcp;

/** @param c X509 certificate, which this object will take ownership of */
Certificate::Certificate (X509* c)
	: _certificate (c)
{
	
}

Certificate::~Certificate ()
{
	X509_free (_certificate);
}

string
Certificate::issuer () const
{
	X509_NAME* n = X509_get_issuer_name (_certificate);
	assert (n);

	char b[256];
	X509_NAME_oneline (n, b, 256);
	return b;
}

string
Certificate::name_for_xml (string const & n)
{
	stringstream x;
	
	vector<string> p;
	boost::split (p, n, boost::is_any_of ("/"));
	for (vector<string>::const_reverse_iterator i = p.rbegin(); i != p.rend(); ++i) {
		x << *i << ",";
	}

	return x.str().substr(0, x.str().length() - 2);
}

string
Certificate::subject () const
{
	X509_NAME* n = X509_get_subject_name (_certificate);
	assert (n);

	char b[256];
	X509_NAME_oneline (n, b, 256);
	return b;
}

string
Certificate::serial () const
{
	ASN1_INTEGER* s = X509_get_serialNumber (_certificate);
	assert (s);
	
	BIGNUM* b = ASN1_INTEGER_to_BN (s, 0);
	char* c = BN_bn2dec (b);
	BN_free (b);
	
	string st (c);
	OPENSSL_free (c);

	return st;
}
		

/** @param filename Text file of PEM-format certificates,
 *  in the order:
 *
 *  1. self-signed root certificate
 *  2. intermediate certificate signed by root certificate
 *  ...
 *  n. leaf certificate signed by previous intermediate.
 */

CertificateChain::CertificateChain (string const & filename)
{
	FILE* f = fopen (filename.c_str(), "r");
	if (!f) {
		throw FileError ("could not open file", filename);
	}
	
	while (1) {
		X509* c = 0;
		if (!PEM_read_X509 (f, &c, 0, 0)) {
			break;
		}

		_certificates.push_back (shared_ptr<Certificate> (new Certificate (c)));
	}
}

shared_ptr<Certificate>
CertificateChain::root () const
{
	assert (!_certificates.empty());
	return _certificates.front ();
}

shared_ptr<Certificate>
CertificateChain::leaf () const
{
	assert (_certificates.size() >= 2);
	return _certificates.back ();
}

list<shared_ptr<Certificate> >
CertificateChain::leaf_to_root () const
{
	list<shared_ptr<Certificate> > c = _certificates;
	c.reverse ();
	return c;
}
