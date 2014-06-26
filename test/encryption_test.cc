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

#include <boost/test/unit_test.hpp>
#include "kdm.h"
#include "KM_util.h"
#include "metadata.h"
#include "certificates.h"
#include "dcp.h"
#include "signer.h"
#include "cpl.h"
#include "mono_picture_asset.h"
#include "sound_asset.h"
#include "reel.h"
#include "test.h"
#include "signer_chain.h"

using boost::shared_ptr;

/* Load a certificate chain from build/test/data/ *.pem and then build
   an encrypted DCP and a KDM using it.
*/
BOOST_AUTO_TEST_CASE (encryption)
{
	boost::filesystem::remove_all ("build/test/signer");
	boost::filesystem::create_directory ("build/test/signer");
	libdcp::make_signer_chain ("build/test/signer", "openssl");
	
	Kumu::libdcp_test = true;

	libdcp::MXFMetadata mxf_metadata;
	mxf_metadata.company_name = "OpenDCP";
	mxf_metadata.product_name = "OpenDCP";
	mxf_metadata.product_version = "0.0.25";

	libdcp::XMLMetadata xml_metadata;
	xml_metadata.issuer = "OpenDCP 0.0.25";
	xml_metadata.creator = "OpenDCP 0.0.25";
	xml_metadata.issue_date = "2012-07-17T04:45:18+00:00";
	
	boost::filesystem::remove_all ("build/test/DCP/bar");
	boost::filesystem::create_directories ("build/test/DCP/bar");
	libdcp::DCP d ("build/test/DCP/bar");

	/* Use test/ref/crypt so this test is repeatable */
	libdcp::CertificateChain chain;
	chain.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate (boost::filesystem::path ("test/ref/crypt/ca.self-signed.pem"))));
	chain.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate (boost::filesystem::path ("test/ref/crypt/intermediate.signed.pem"))));
	chain.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate (boost::filesystem::path ("test/ref/crypt/leaf.signed.pem"))));

	shared_ptr<libdcp::Signer> signer (
		new libdcp::Signer (
			chain,
			"test/ref/crypt/leaf.key"
			)
		);

	shared_ptr<libdcp::CPL> cpl (new libdcp::CPL ("build/test/DCP/bar", "A Test DCP", libdcp::FEATURE, 24, 24));

	libdcp::Key key;
	
	shared_ptr<libdcp::MonoPictureAsset> mp (new libdcp::MonoPictureAsset ("build/test/DCP/bar", "video.mxf"));
	mp->set_progress (&d.Progress);
	mp->set_edit_rate (24);
	mp->set_intrinsic_duration (24);
	mp->set_duration (24);
	mp->set_size (libdcp::Size (32, 32));
	mp->set_metadata (mxf_metadata);
	mp->set_key (key);
	mp->create (j2c);

	shared_ptr<libdcp::SoundAsset> ms (new libdcp::SoundAsset ("build/test/DCP/bar", "audio.mxf"));
	ms->set_progress (&d.Progress);
	ms->set_edit_rate (24);
	ms->set_intrinsic_duration (24);
	mp->set_duration (24);
	ms->set_channels (2);
	ms->set_metadata (mxf_metadata);
	ms->set_key (key);
	ms->create (wav);
	
	cpl->add_reel (shared_ptr<libdcp::Reel> (new libdcp::Reel (mp, ms, shared_ptr<libdcp::SubtitleAsset> ())));
	d.add_cpl (cpl);

	d.write_xml (false, xml_metadata, signer);

	boost::filesystem::path cpl_path = boost::filesystem::path ("build/test/DCP/bar") / (cpl->id() + "_cpl.xml");

	libdcp::KDM kdm (
		cpl_path,
		signer,
		signer->certificates().leaf(),
		key,
		boost::posix_time::time_from_string ("2013-01-01 00:00:00"),
		boost::posix_time::time_from_string ("2013-01-08 00:00:00"),
		"libdcp",
		"2012-07-17T04:45:18+00:00",
		libdcp::KDM::MODIFIED_TRANSITIONAL_1
		);

	kdm.as_xml ("build/test/bar.kdm.xml");
	
	int r = system (
		"xmllint --path schema --nonet --noout --schema schema/SMPTE-430-1-2006-Amd-1-2009-KDM.xsd build/test/bar.kdm.xml "
		"> build/test/xmllint.log 2>&1 < /dev/null"
		);

#ifdef DCPOMATIC_POSIX	
	BOOST_CHECK_EQUAL (WEXITSTATUS (r), 0);
#else
	BOOST_CHECK_EQUAL (r, 0);
#endif	
		
	r = system ("xmlsec1 verify "
		"--pubkey-cert-pem test/ref/crypt/leaf.signed.pem "
		"--trusted-pem test/ref/crypt/intermediate.signed.pem "
		"--trusted-pem test/ref/crypt/ca.self-signed.pem "
		"--id-attr:Id http://www.smpte-ra.org/schemas/430-3/2006/ETM:AuthenticatedPublic "
		"--id-attr:Id http://www.smpte-ra.org/schemas/430-3/2006/ETM:AuthenticatedPrivate "
		    "build/test/bar.kdm.xml > build/test/xmlsec1.log 2>&1 < /dev/null");
	
#ifdef DCPOMATIC_POSIX	
	BOOST_CHECK_EQUAL (WEXITSTATUS (r), 0);
#else
	BOOST_CHECK_EQUAL (r, 0);
#endif	
}
