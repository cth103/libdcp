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

#include <string>
#include <list>
#include <boost/shared_ptr.hpp>

namespace libdcp
{

class Asset;	

/** A class to create a DCP */	
class DCP
{
public:
	enum ContentType
	{
		FEATURE,
		SHORT,
		TRAILER,
		TEST,
		TRANSITIONAL,
		RATING,
		TEASER,
		POLICY,
		PUBLIC_SERVICE_ANNOUNCEMENT,
		ADVERTISEMENT
	};
	
	DCP (std::string, std::string, ContentType, int, int);

	void add_sound_asset (std::list<std::string> const &);
	void add_picture_asset (std::list<std::string> const &, int, int);

	void write_xml () const;

private:

	std::string write_cpl (std::string) const;
	std::string write_pkl (std::string, std::string, std::string, int) const;
	void write_volindex () const;
	void write_assetmap (std::string, int, std::string, int) const;

	static std::string content_type_string (ContentType);

	/** the directory that we are writing to */
	std::string _directory;
	/** the name of the DCP */
	std::string _name;
	/** the content type of the DCP */
	ContentType _content_type;
	/** frames per second */
	int _fps;
	/** length in frames */
	int _length;
	/** assets */
	std::list<boost::shared_ptr<Asset> > _assets;
};

}
