/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#include <iostream>
#include "dcp.h"
#include "cpl.h"
#include "reel.h"
#include "subtitle_asset.h"
#include "reel_subtitle_asset.h"
#include "exceptions.h"

using std::cout;
using std::cerr;
using std::list;
using std::string;
using boost::shared_ptr;
using namespace dcp;

int
main (int argc, char* argv[])
{
	try {
		if (argc < 2) {
			cerr << "Syntax: " << argv[0] << " <dcp>\n";
			exit (EXIT_FAILURE);
		}
		
		DCP* dcp = new DCP (argv[1]);
		dcp->read (true);
		
		list<shared_ptr<CPL> > cpls = dcp->cpls ();
		for (list<boost::shared_ptr<CPL> >::iterator i = cpls.begin(); i != cpls.end(); ++i) {
			
			list<shared_ptr<Reel> > reels = (*i)->reels ();
			for (list<shared_ptr<Reel> >::iterator j = reels.begin(); j != reels.end(); ++j) {
				
				if ((*j)->main_subtitle()) {
					(*j)->main_subtitle()->subtitle_asset()->write_xml ((*j)->main_subtitle()->subtitle_asset()->file ());
				}
			}
		}
	}

	catch (FileError& e)
	{
		cerr << e.what() << " (" << e.filename() << ") when reading " << argv[1] << "\n";
		exit (EXIT_FAILURE);
	}
	catch (DCPReadError& e)
	{
		cerr << e.what() << " when reading " << argv[1] << "\n";
		exit (EXIT_FAILURE);
	}
	
	return 0;
}
