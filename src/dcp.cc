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

#include <sstream>
#include <fstream>
#include <iomanip>
#include <cassert>
#include <boost/filesystem.hpp>
#include "dcp.h"
#include "asset.h"
#include "sound_asset.h"
#include "picture_asset.h"
#include "util.h"
#include "tags.h"

using namespace std;
using namespace boost;
using namespace libdcp;

/** Construct a DCP.
 *  @param d Directory to write files to.
 */
DCP::DCP (string d, string n, ContentType c, int fps, int length)
	: _directory (d)
	, _name (n)
	, _content_type (c)
	, _fps (fps)
	, _length (length)
{
	char buffer[64];
	time_t now;
	time (&now);
	struct tm* tm = localtime (&now);
	strftime (buffer, 64, "%Y-%m-%dT%I:%M:%S+00:00", tm);
	_date = string (buffer);
}

void
DCP::add_sound_asset (list<string> const & files)
{
	filesystem::path p;
	p /= _directory;
	p /= "audio.mxf";
	_assets.push_back (shared_ptr<SoundAsset> (new SoundAsset (files, p.string(), _fps, _length)));
}

void
DCP::add_picture_asset (list<string> const & files, int w, int h)
{
	filesystem::path p;
	p /= _directory;
	p /= "video.mxf";
	_assets.push_back (shared_ptr<PictureAsset> (new PictureAsset (files, p.string(), _fps, _length, w, h)));
}

/** Write the required XML files to the directory that was
 *  passed into the constructor.
 */
void
DCP::write_xml () const
{
	string cpl_uuid = make_uuid ();
	string cpl_path = write_cpl (cpl_uuid);
	int cpl_length = filesystem::file_size (cpl_path);
	string cpl_digest = make_digest (cpl_path);

	string pkl_uuid = make_uuid ();
	string pkl_path = write_pkl (pkl_uuid, cpl_uuid, cpl_digest, cpl_length);
	
	write_volindex ();
	write_assetmap (cpl_uuid, cpl_length, pkl_uuid, filesystem::file_size (pkl_path));
}

/** Write the CPL file.
 *  @param cpl_uuid UUID to use.
 *  @return CPL pathname.
 */
string
DCP::write_cpl (string cpl_uuid) const
{
	filesystem::path p;
	p /= _directory;
	stringstream s;
	s << cpl_uuid << "_cpl.xml";
	p /= s.str();
	ofstream cpl (p.string().c_str());
	
	cpl << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    << "<CompositionPlaylist xmlns=\"http://www.smpte-ra.org/schemas/429-7/2006/CPL\">\n"
	    << "  <Id>urn:uuid:" << cpl_uuid << "</Id>\n"
	    << "  <AnnotationText>" << _name << "</AnnotationText>\n"
	    << "  <IssueDate>" << _date << "</IssueDate>\n"
	    << "  <Creator>libdcp " << Tags::instance()->creator << "</Creator>\n"
	    << "  <ContentTitleText>" << _name << "</ContentTitleText>\n"
	    << "  <ContentKind>" << _content_type << "</ContentKind>\n"
	    << "  <ContentVersion>\n"
	    << "    <Id>urn:uri:" << cpl_uuid << "_" << _date << "</Id>\n"
	    << "    <LabelText>" << cpl_uuid << "_" << _date << "</LabelText>\n"
	    << "  </ContentVersion>\n"
	    << "  <RatingList/>\n"
	    << "  <ReelList>\n";

	cpl << "    <Reel>\n"
	    << "      <Id>urn:uuid:" << make_uuid() << "</Id>\n"
	    << "      <AssetList>\n";

	for (list<shared_ptr<Asset> >::const_iterator i = _assets.begin(); i != _assets.end(); ++i) {
		(*i)->write_to_cpl (cpl);
	}

	cpl << "      </AssetList>\n"
	    << "    </Reel>\n"
	    << "  </ReelList>\n"
	    << "</CompositionPlaylist>\n";

	return p.string ();
}

/** Write the PKL file.
 *  @param pkl_uuid UUID to use.
 *  @param cpl_uuid UUID of the CPL file.
 *  @param cpl_digest SHA digest of the CPL file.
 *  @param cpl_length Length of the CPL file in bytes.
 */
std::string
DCP::write_pkl (string pkl_uuid, string cpl_uuid, string cpl_digest, int cpl_length) const
{
	filesystem::path p;
	p /= _directory;
	stringstream s;
	s << pkl_uuid << "_pkl.xml";
	p /= s.str();
	ofstream pkl (p.string().c_str());

	pkl << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    << "<PackingList xmlns=\"http://www.smpte-ra.org/schemas/429-8/2007/PKL\">\n"
	    << "  <Id>urn:uuid:" << pkl_uuid << "</Id>\n"
	    << "  <AnnotationText>" << _name << "</AnnotationText>\n"
	    << "  <IssueDate>" << _date << "</IssueDate>\n"
	    << "  <Issuer>" << Tags::instance()->issuer << "</Issuer>\n"
	    << "  <Creator>" << Tags::instance()->creator << "</Creator>\n"
	    << "  <AssetList>\n";

	for (list<shared_ptr<Asset> >::const_iterator i = _assets.begin(); i != _assets.end(); ++i) {
		(*i)->write_to_pkl (pkl);
	}

	pkl << "    <Asset>\n"
	    << "      <Id>urn:uuid" << cpl_uuid << "</Id>\n"
	    << "      <Hash>" << cpl_digest << "</Hash>\n"
	    << "      <Size>" << cpl_length << "</Size>\n"
	    << "      <Type>text/xml</Type>\n"
	    << "    </Asset>\n";

	pkl << "  </AssetList>\n"
	    << "</PackingList>\n";

	return p.string ();
}

void
DCP::write_volindex () const
{
	filesystem::path p;
	p /= _directory;
	p /= "VOLINDEX.xml";
	ofstream vi (p.string().c_str());

	vi << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	   << "<VolumeIndex xmlns=\"http://www.smpte-ra.org/schemas/429-9/2007/AM\">\n"
	   << "  <Index>1</Index>\n"
	   << "</VolumeIndex>\n";
}

void
DCP::write_assetmap (string cpl_uuid, int cpl_length, string pkl_uuid, int pkl_length) const
{
	filesystem::path p;
	p /= _directory;
	p /= "ASSETMAP.xml";
	ofstream am (p.string().c_str());

	am << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	   << "<AssetMap xmlns=\"http://www.smpte-ra.org/schemas/429-9/2007/AM\">\n"
	   << "  <Id>urn:uuid:" << make_uuid() << "</Id>\n"
	   << "  <Creator>" << Tags::instance()->creator << "</Creator>\n"
	   << "  <VolumeCount>1</VolumeCount>\n"
	   << "  <IssueDate>" << _date << "</IssueDate>\n"
	   << "  <Issuer>" << Tags::instance()->issuer << "</Issuer>\n"
	   << "  <AssetList>\n";

	am << "    <Asset>\n"
	   << "      <Id>urn:uuid:" << pkl_uuid << "</Id>\n"
	   << "      <PackingList>true</PackingList>\n"
	   << "      <ChunkList>\n"
	   << "        <Chunk>\n"
	   << "          <Path>" << pkl_uuid << "_pkl.xml</Path>\n"
	   << "          <VolumeIndex>1</VolumeIndex>\n"
	   << "          <Offset>0</Offset>\n"
	   << "          <Length>" << pkl_length << "</Length>\n"
	   << "        </Chunk>\n"
	   << "      </ChunkList>\n"
	   << "    </Asset>\n";

	am << "    <Asset>\n"
	   << "      <Id>urn:uuid:" << cpl_uuid << "</Id>\n"
	   << "      <ChunkList>\n"
	   << "        <Chunk>\n"
	   << "          <Path>" << cpl_uuid << "_cpl.xml</Path>\n"
	   << "          <VolumeIndex>1</VolumeIndex>\n"
	   << "          <Offset>0</Offset>\n"
	   << "          <Length>" << cpl_length << "</Length>\n"
	   << "        </Chunk>\n"
	   << "      </ChunkList>\n"
	   << "    </Asset>\n";
	
	for (list<shared_ptr<Asset> >::const_iterator i = _assets.begin(); i != _assets.end(); ++i) {
		(*i)->write_to_assetmap (am);
	}

	am << "  </AssetList>\n"
	   << "</AssetMap>\n";
}


string
DCP::content_type_string (ContentType t)
{
	switch (t) {
	case FEATURE:
		return "feature";
	case SHORT:
		return "short";
	case TRAILER:
		return "trailer";
	case TEST:
		return "test";
	case TRANSITIONAL:
		return "transitional";
	case RATING:
		return "rating";
	case TEASER:
		return "teaser";
	case POLICY:
		return "policy";
	case PUBLIC_SERVICE_ANNOUNCEMENT:
		return "psa";
	case ADVERTISEMENT:
		return "advertisement";
	}

	assert (false);
}
		
