/*
    Copyright (C) 2013 Carl Hetherington <cth@carlh.net>

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

#ifndef LIBDCP_XML_KDM_SMPTE_H
#define LIBDCP_XML_KDM_SMPTE_H

#include <string>
#include <list>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>

namespace libdcp {
namespace xml {	

class Signer
{
public:
	Signer () {}
	Signer (boost::shared_ptr<const cxml::Node>);

	void as_xml (xmlpp::Element *) const;
	
	std::string x509_issuer_name;
	std::string x509_serial_number;
};

class Recipient
{
public:
	Recipient () {}
	Recipient (boost::shared_ptr<const cxml::Node>);

	void as_xml (xmlpp::Element *) const;
	
	Signer x509_issuer_serial;
	std::string x509_subject_name;
};

class AuthorizedDeviceInfo
{
public:
	AuthorizedDeviceInfo () {}
	AuthorizedDeviceInfo (boost::shared_ptr<const cxml::Node>);
	
	void as_xml (xmlpp::Element *) const;

	std::string device_list_identifier;
	std::string device_list_description;
	std::list<std::string> device_list;
};

class TypedKeyId
{
public:
	TypedKeyId () {}
	TypedKeyId (boost::shared_ptr<const cxml::Node>);
	
	void as_xml (xmlpp::Element *) const;

	std::string key_type;
	std::string key_id;
};

class AuthenticatedPublic
{
public:
	AuthenticatedPublic () {}
	AuthenticatedPublic (boost::shared_ptr<const cxml::Node>);

	void as_xml (xmlpp::Element *) const;
	
	std::string message_id;
	std::string message_type;
	boost::optional<std::string> annotation_text;
	std::string issue_date;
	Signer signer;
	Recipient recipient;
	std::string composition_playlist_id;
	std::string content_title_text;
	std::string content_keys_not_valid_before;
	std::string content_keys_not_valid_after;
	AuthorizedDeviceInfo authorized_device_info;
	std::list<TypedKeyId> key_id_list;
	std::list<std::string> forensic_mark_flag_list;
};

class AuthenticatedPrivate
{
public:
	AuthenticatedPrivate () {}
	AuthenticatedPrivate (boost::shared_ptr<const cxml::Node>);
	
	void as_xml (xmlpp::Element *) const;
	
	std::list<std::string> encrypted_keys;
};

class X509Data
{
public:
	X509Data () {}
	X509Data (boost::shared_ptr<const cxml::Node>);
	
	void as_xml (xmlpp::Element *) const;
	
	Signer x509_issuer_serial;
	std::string x509_certificate;
};

class Reference
{
public:
	Reference () {}
	Reference (boost::shared_ptr<const cxml::Node>);

	void as_xml (xmlpp::Element *) const;
	
	std::string uri;
	std::string digest_value;
};

class Signature
{
public:
	Signature () {}
	Signature (boost::shared_ptr<const cxml::Node>);

	void as_xml (xmlpp::Element *) const;
	
	std::list<Reference> signed_info;
	std::string signature_value;
	std::list<X509Data> key_info;
};

class DCinemaSecurityMessage
{
public:
	DCinemaSecurityMessage (boost::filesystem::path);

	void as_xml (boost::filesystem::path) const;
	
	AuthenticatedPublic authenticated_public;
	AuthenticatedPrivate authenticated_private;
	Signature signature;
};

}
}

#endif
	
