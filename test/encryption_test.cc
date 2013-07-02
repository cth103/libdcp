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

/* Load a certificate chain from build/test/data/.pem and then build
   an encrypted DCP and a KDM using it.
*/
BOOST_AUTO_TEST_CASE (encryption)
{
	Kumu::libdcp_test = true;

	libdcp::MXFMetadata mxf_metadata;
	mxf_metadata.company_name = "OpenDCP";
	mxf_metadata.product_name = "OpenDCP";
	mxf_metadata.product_version = "0.0.25";

	libdcp::XMLMetadata xml_metadata;
	xml_metadata.issuer = "OpenDCP 0.0.25";
	xml_metadata.creator = "OpenDCP 0.0.25";
	xml_metadata.issue_date = "2012-07-17T04:45:18+00:00";
	
	boost::filesystem::remove_all ("build/test/bar");
	boost::filesystem::create_directories ("build/test/bar");
	libdcp::DCP d ("build/test/DCP/bar");

	libdcp::CertificateChain chain;
	chain.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate ("build/test/data/ca.self-signed.pem")));
	chain.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate ("build/test/data/intermediate.signed.pem")));
	chain.add (shared_ptr<libdcp::Certificate> (new libdcp::Certificate ("build/test/data/leaf.signed.pem")));

	shared_ptr<libdcp::Encryption> crypt (
		new libdcp::Encryption (
			chain,
			"test/data/signer.key"
			)
		);

	shared_ptr<libdcp::CPL> cpl (new libdcp::CPL ("build/test/bar", "A Test DCP", libdcp::FEATURE, 24, 24));
	
	shared_ptr<libdcp::MonoPictureAsset> mp (new libdcp::MonoPictureAsset (
							 j2c,
							 "build/test/bar",
							 "video.mxf",
							 &d.Progress,
							 24,
							 24,
							 true,
							 libdcp::Size (32, 32),
							 mxf_metadata
							 ));

	shared_ptr<libdcp::SoundAsset> ms (new libdcp::SoundAsset (
						   wav,
						   "build/test/bar",
						   "audio.mxf",
						   &(d.Progress),
						   24,
						   24,
						   2,
						   true,
						   mxf_metadata
						   ));
	
	cpl->add_reel (shared_ptr<libdcp::Reel> (new libdcp::Reel (mp, ms, shared_ptr<libdcp::SubtitleAsset> ())));
	d.add_cpl (cpl);

	d.write_xml (xml_metadata, crypt);

	shared_ptr<xmlpp::Document> kdm = cpl->make_kdm (
		crypt->certificates,
		crypt->signer_key,
		crypt->certificates.leaf(),
		boost::posix_time::time_from_string ("2013-01-01 00:00:00"),
		boost::posix_time::time_from_string ("2013-01-08 00:00:00"),
		mxf_metadata,
		xml_metadata
		);

	kdm->write_to_file_formatted ("build/test/bar.kdm.xml", "UTF-8");
}
