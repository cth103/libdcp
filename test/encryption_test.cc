/*
    Copyright (C) 2013-2014 Carl Hetherington <cth@carlh.net>

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

#include "kdm.h"
#include "KM_util.h"
#include "metadata.h"
#include "certificates.h"
#include "dcp.h"
#include "signer.h"
#include "cpl.h"
#include "mono_picture_mxf.h"
#include "picture_mxf_writer.h"
#include "sound_mxf.h"
#include "reel.h"
#include "test.h"
#include "file.h"
#include "signer_chain.h"
#include "subtitle_content.h"
#include "reel_mono_picture_asset.h"
#include "reel_sound_asset.h"
#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>

using boost::shared_ptr;

/* Load a certificate chain from build/test/data/ *.pem and then build
   an encrypted DCP and a KDM using it.
*/
BOOST_AUTO_TEST_CASE (encryption_test)
{
	boost::filesystem::remove_all ("build/test/signer");
	boost::filesystem::create_directory ("build/test/signer");
	dcp::make_signer_chain ("build/test/signer", "openssl");
	
	Kumu::libdcp_test = true;

	dcp::MXFMetadata mxf_metadata;
	mxf_metadata.company_name = "OpenDCP";
	mxf_metadata.product_name = "OpenDCP";
	mxf_metadata.product_version = "0.0.25";

	dcp::XMLMetadata xml_metadata;
	xml_metadata.issuer = "OpenDCP 0.0.25";
	xml_metadata.creator = "OpenDCP 0.0.25";
	xml_metadata.issue_date = "2012-07-17T04:45:18+00:00";
	
	boost::filesystem::remove_all ("build/test/DCP/bar");
	boost::filesystem::create_directories ("build/test/DCP/bar");
	dcp::DCP d ("build/test/DCP/bar");

	dcp::CertificateChain chain;
	chain.add (shared_ptr<dcp::Certificate> (new dcp::Certificate (boost::filesystem::path ("build/test/signer/ca.self-signed.pem"))));
	chain.add (shared_ptr<dcp::Certificate> (new dcp::Certificate (boost::filesystem::path ("build/test/signer/intermediate.signed.pem"))));
	chain.add (shared_ptr<dcp::Certificate> (new dcp::Certificate (boost::filesystem::path ("build/test/signer/leaf.signed.pem"))));

	shared_ptr<dcp::Signer> signer (
		new dcp::Signer (
			chain,
			"build/test/signer/leaf.key"
			)
		);

	shared_ptr<dcp::CPL> cpl (new dcp::CPL ("A Test DCP", dcp::FEATURE));

	dcp::Key key;
	
	shared_ptr<dcp::MonoPictureMXF> mp (new dcp::MonoPictureMXF (dcp::Fraction (24, 1)));
	mp->set_progress (&d.Progress);
	mp->set_metadata (mxf_metadata);
	mp->set_key (key);

	shared_ptr<dcp::PictureMXFWriter> writer = mp->start_write ("build/test/DCP/bar/video.mxf", dcp::SMPTE, false);
	dcp::File j2c ("test/data/32x32_red_square.j2c");
	for (int i = 0; i < 24; ++i) {
		writer->write (j2c.data (), j2c.size ());
	}
	writer->finalize ();

	cpl->add (shared_ptr<dcp::Reel> (new dcp::Reel (
						 shared_ptr<dcp::ReelMonoPictureAsset> (new dcp::ReelMonoPictureAsset (mp, 0)),
						 shared_ptr<dcp::ReelSoundAsset> (),
						 shared_ptr<dcp::ReelSubtitleAsset> ()
						 )));
	d.add (cpl);
	d.write_xml (dcp::SMPTE, xml_metadata, signer);

	dcp::KDM kdm (
		cpl,
		signer,
		signer->certificates().leaf(),
		boost::posix_time::time_from_string ("2013-01-01 00:00:00"),
		boost::posix_time::time_from_string ("2013-01-08 00:00:00"),
		"libdcp",
		"2012-07-17T04:45:18+00:00"
		);

	kdm.as_xml ("build/test/bar.kdm.xml");
	system ("xmllint --path schema --nonet --noout --schema schema/SMPTE-430-1-2006-Amd-1-2009-KDM.xsd build/test/bar.kdm.xml");
	system ("xmlsec1 verify "
		"--pubkey-cert-pem build/test/signer/leaf.signed.pem "
		"--trusted-pem build/test/signer/intermediate.signed.pem "
		"--trusted-pem build/test/signer/ca.self-signed.pem "
		"--id-attr:Id http://www.smpte-ra.org/schemas/430-3/2006/ETM:AuthenticatedPublic "
		"--id-attr:Id http://www.smpte-ra.org/schemas/430-3/2006/ETM:AuthenticatedPrivate "
		"build/test/bar.kdm.xml");
}
