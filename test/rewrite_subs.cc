#include <iostream>
#include "dcp.h"
#include "cpl.h"
#include "reel.h"
#include "subtitle_asset.h"
#include "exceptions.h"

using std::cout;
using std::cerr;
using std::list;
using boost::shared_ptr;
using namespace libdcp;

int
main (int argc, char* argv[])
{
	try {
		if (argc < 2) {
			cerr << "Syntax: " << argv[0] << " <dcp>\n";
			exit (EXIT_FAILURE);
		}
		
		DCP* dcp = new DCP (argv[1]);
		dcp->read (false);
		
		list<shared_ptr<CPL> > cpls = dcp->cpls ();
		for (list<boost::shared_ptr<CPL> >::iterator i = cpls.begin(); i != cpls.end(); ++i) {
			
			list<shared_ptr<Reel> > reels = (*i)->reels ();
			for (list<shared_ptr<Reel> >::iterator j = reels.begin(); j != reels.end(); ++j) {
				
				if ((*j)->main_subtitle()) {
					(*j)->main_subtitle()->write_xml ();
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
