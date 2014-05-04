#include <iostream>
#include "subtitle_content.h"

using namespace std;

int main (int argc, char* argv[])
{
	if (argc < 2) {
		cerr << "Syntax: " << argv[0] << " <subtitle file>\n";
		exit (EXIT_FAILURE);
	}
	
	dcp::SubtitleContent s (argv[1], false);
	cout << s.xml_as_string ();
	return 0;
}
