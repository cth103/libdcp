#include <iostream>
#include <boost/filesystem.hpp>
#include <getopt.h>
#include "dcp.h"
#include "exceptions.h"

using namespace std;
using namespace boost;
using namespace dcp;

static bool verbose = false;

static void
help (string n)
{
	cerr << "Syntax: " << n << " [OPTION] <DCP> <DCP>\n"
	     << "  -V, --version        show libdcp version\n"
	     << "  -h, --help           show this help\n"
	     << "  -v, --verbose        be verbose\n"
	     << "  -n, --names          allow differing MXF names\n"
	     << "  -m, --mean-pixel     maximum allowed mean pixel error (default 5)\n"
	     << "  -s, --std-dev-pixel  maximum allowed standard deviation of pixel error (default 5)\n"
	     << "\n"
	     << "The <DCP>s are the DCP directories to compare.\n"
	     << "Comparison is of metadata and content, ignoring timestamps\n"
	     << "and differing UUIDs.\n";
}

void
note (NoteType t, string n)
{
	if (t == ERROR || verbose) {
		cout << " " << n << "\n";
	}
}

int
main (int argc, char* argv[])
{
	EqualityOptions options;
	options.max_mean_pixel_error = 5;
	options.max_std_dev_pixel_error = 5;
	
	int option_index = 0;
	while (1) {
		static struct option long_options[] = {
			{ "version", no_argument, 0, 'V'},
			{ "help", no_argument, 0, 'h'},
			{ "verbose", no_argument, 0, 'v'},
			{ "names", no_argument, 0, 'n'},
			{ "mean-pixel", required_argument, 0, 'm'},
			{ "std-dev-pixel", required_argument, 0, 's'},
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long (argc, argv, "Vhvnm:s:", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'V':
			cout << "dcpdiff version " << LIBDCP_VERSION << "\n";
			exit (EXIT_SUCCESS);
		case 'h':
			help (argv[0]);
			exit (EXIT_SUCCESS);
		case 'v':
			verbose = true;
			break;
		case 'n':
			options.mxf_names_can_differ = true;
			break;
		case 'm':
			options.max_mean_pixel_error = atof (optarg);
			break;
		case 's':
			options.max_std_dev_pixel_error = atof (optarg);
			break;
		}
	}

	if (argc <= optind || argc > (optind + 2)) {
		help (argv[0]);
		exit (EXIT_FAILURE);
	}

	if (!filesystem::exists (argv[optind])) {
		cerr << argv[0] << ": DCP " << argv[optind] << " not found.\n";
		exit (EXIT_FAILURE);
	}

	if (!filesystem::exists (argv[optind + 1])) {
		cerr << argv[0] << ": DCP " << argv[optind + 1] << " not found.\n";
		exit (EXIT_FAILURE);
	}

	cout << "reading A.\n";
	DCP* a = 0;
	try {
		a = new DCP (argv[optind]);
		a->read ();
	} catch (FileError& e) {
		cerr << "Could not read DCP " << argv[optind] << "; " << e.what() << " " << e.filename() << "\n";
		exit (EXIT_FAILURE);
	}

	cout << "reading B.\n";
	DCP* b = 0;
	try {
		b = new DCP (argv[optind + 1]);
		b->read ();
	} catch (FileError& e) {
		cerr << "Could not read DCP " << argv[optind + 1] << "; " << e.what() << " " << e.filename() << "\n";
		exit (EXIT_FAILURE);
	}

	/* I think this is just below the LSB at 16-bits (ie the 8th most significant bit at 24-bit) */
	options.max_audio_sample_error = 255;

	bool const equals = a->equals (*b, options, boost::bind (note, _1, _2));

	if (equals) {
		exit (EXIT_SUCCESS);
	}

	exit (EXIT_FAILURE);
}
