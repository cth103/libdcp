#include <iostream>
#include <cstdlib>
#include <boost/filesystem.hpp>
#include <getopt.h>
#include "dcp.h"
#include "exceptions.h"
#include "reel.h"

using namespace std;
using namespace boost;
using namespace libdcp;

static void
help (string n)
{
	cerr << "Syntax: " << n << " <DCP>\n";
}

int
main (int argc, char* argv[])
{
	int option_index = 0;
	while (1) {
		static struct option long_options[] = {
			{ "version", no_argument, 0, 'v'},
			{ "help", no_argument, 0, 'h'},
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long (argc, argv, "vh", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'v':
			cout << "dcpdiff version " << LIBDCP_VERSION << "\n";
			exit (EXIT_SUCCESS);
		case 'h':
			help (argv[0]);
			exit (EXIT_SUCCESS);
		}
	}

	if (argc <= optind || argc > (optind + 1)) {
		help (argv[0]);
		exit (EXIT_FAILURE);
	}

	if (!filesystem::exists (argv[optind])) {
		cerr << argv[0] << ": DCP " << argv[optind] << " not found.\n";
		exit (EXIT_FAILURE);
	}

	list<string> missing_mxfs;

	DCP* dcp = 0;
	try {
		dcp = new DCP (argv[optind], false);
	} catch (FileError& e) {
		cerr << "Could not read DCP " << argv[optind] << "; " << e.what() << " " << e.filename() << "\n";
		exit (EXIT_FAILURE);
	}

	cout << "DCP: " << argv[optind] << "\n"
	     << "\tLength: " << dcp->length() << "\n"
	     << "\tFrames per second: " << dcp->frames_per_second() << "\n";

	if (!missing_mxfs.empty ()) {
		cout << "\tmissing MXFs: ";
		for (list<string>::const_iterator i = missing_mxfs.begin(); i != missing_mxfs.end(); ++i) {
			cout << *i << " " ;
		}
		cout << "\n";
	}

	list<shared_ptr<const Reel> > reels = dcp->reels ();

	int R = 1;
	for (list<shared_ptr<const Reel> >::const_iterator i = reels.begin(); i != reels.end(); ++i) {
		cout << "Reel " << R << "\n";
		cout << "\tContains: ";
		if ((*i)->main_picture()) {
			cout << "picture ";
		}
		if ((*i)->main_sound()) {
			cout << "sound ";
		}
		if ((*i)->main_subtitle()) {
			cout << "subtitle ";
		}
		cout << "\n";
		++R;
	}

	return 0;
}
