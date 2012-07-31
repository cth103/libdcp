#include <getopt.h>
#include "dcp.h"

using namespace std;
using namespace libdcp;

static void
help (string n)
{
	cerr << "Syntax: " << n << " [OPTION] <DCP> <DCP>\n"
	     << "  -v, --version      show DVD-o-matic version\n"
	     << "  -h, --help         show this help\n"
	     << "\n"
	     << "The <DCP>s are the DCP directories to compare.\n";
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

	if (argc <= optind || argc > (optind + 2)) {
		help (argv[0]);
		exit (EXIT_FAILURE);
	}

	DCP a (argv[optind]);
	DCP b (argv[optind + 1]);

	list<string> notes = a.equals (b, EqualityFlags (LIBDCP_METADATA | MXF_BITWISE));
	if (notes.empty ()) {
		cout << "DCPs identical\n";
		exit (EXIT_SUCCESS);
	}

	for (list<string>::iterator i = notes.begin(); i != notes.end(); ++i) {
		cout << "  " << *i << "\n";
	}

	exit (EXIT_FAILURE);
}
