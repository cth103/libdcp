#include <iostream>
#include <cstdlib>
#include <boost/filesystem.hpp>
#include <getopt.h>
#include "dcp.h"
#include "exceptions.h"
#include "reel.h"
#include "sound_asset.h"
#include "picture_asset.h"
#include "subtitle_asset.h"
#include "cpl.h"

using std::string;
using std::cerr;
using std::cout;
using std::list;
using boost::shared_ptr;
using namespace libdcp;

static void
help (string n)
{
	cerr << "Syntax: " << n << " [options] <DCP>\n"
	     << "  -s, --subtitles  list all subtitles\n";
}

int
main (int argc, char* argv[])
{
	bool subtitles = false;
	
	int option_index = 0;
	while (1) {
		static struct option long_options[] = {
			{ "version", no_argument, 0, 'v'},
			{ "help", no_argument, 0, 'h'},
			{ "subtitles", no_argument, 0, 's'},
			{ 0, 0, 0, 0 }
		};

		int c = getopt_long (argc, argv, "vhs", long_options, &option_index);

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
		case 's':
			subtitles = true;
			break;
		}
	}

	if (argc <= optind || argc > (optind + 1)) {
		help (argv[0]);
		exit (EXIT_FAILURE);
	}

	if (!boost::filesystem::exists (argv[optind])) {
		cerr << argv[0] << ": DCP " << argv[optind] << " not found.\n";
		exit (EXIT_FAILURE);
	}

	DCP* dcp = 0;
	try {
		dcp = new DCP (argv[optind]);
		dcp->read (false);
	} catch (FileError& e) {
		cerr << "Could not read DCP " << argv[optind] << "; " << e.what() << " " << e.filename() << "\n";
		exit (EXIT_FAILURE);
	}

	cout << "DCP: " << boost::filesystem::path(argv[optind]).filename().string() << "\n";

	list<shared_ptr<CPL> > cpls = dcp->cpls ();

	for (list<shared_ptr<CPL> >::iterator i = cpls.begin(); i != cpls.end(); ++i) {
		cout << "  CPL: " << (*i)->name() << "\n"
		     << "    Length: " << (*i)->length() << "\n"
		     << "    Frames per second: " << (*i)->frames_per_second() << "\n";
		
		list<shared_ptr<Reel> > reels = (*i)->reels ();

		int R = 1;
		for (list<shared_ptr<Reel> >::const_iterator j = reels.begin(); j != reels.end(); ++j) {
			cout << "    Reel " << R << "\n";
			
			if ((*j)->main_picture()) {
				cout << "      Picture:  " << (*j)->main_picture()->size().width << "x" << (*j)->main_picture()->size().height << "\n";
			}
			if ((*j)->main_sound()) {
				cout << "      Sound:    " << (*j)->main_sound()->channels() << " channels at " << (*j)->main_sound()->sampling_rate() << "Hz\n";
			}
			if ((*j)->main_subtitle()) {
				list<shared_ptr<Subtitle> > subs = (*j)->main_subtitle()->subtitles ();
				cout << "      Subtitle: " << subs.size() << " subtitles in " << (*j)->main_subtitle()->language() << "\n";
				if (subtitles) {
					for (list<shared_ptr<Subtitle> >::const_iterator k = subs.begin(); k != subs.end(); ++k) {
						cout << "        " << (*k)->text() << "\n";
						cout << "          "
						     << "font:" << (*k)->font() << "; "
						     << "italic:" << (*k)->italic() << "; "
						     << "color:" << (*k)->color() << "; "
						     << "in:" << (*k)->in() << "; "
						     << "out:" << (*k)->out() << "; "
						     << "v_position:" << (*k)->v_position() << "; "
						     << "v_align:" << (*k)->v_align() << "; "
						     << "effect:" << (*k)->effect() << "; "
						     << "effect_color:" << (*k)->effect_color() << "; "
						     << "fade_up_time:" << (*k)->fade_up_time() << "; "
						     << "fade_down_time:" << (*k)->fade_down_time() << "; "
						     << "size: " << (*k)->size() << "\n";
					}
				}
			}
			++R;
		}
	}

	return 0;
}
