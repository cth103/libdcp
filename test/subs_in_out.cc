#include <iostream>
#include "subtitle_asset.h"

using namespace std;

int main (int argc, char* argv[])
{
	if (argc < 2) {
		cerr << "Syntax: " << argv[0] << " <subtitle file>\n";
		exit (EXIT_FAILURE);
	}
	
	libdcp::SubtitleAsset s ("foo", "bar", "baz");
	s.read_xml (argv[1]);
	cout << s.xml_as_string ();
	return 0;
}
