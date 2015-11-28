/*
    Copyright (C) 2015 Carl Hetherington <cth@carlh.net>

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

#include "data.h"
#include "test.h"
#include "util.h"
#include "version.h"
#include <sys/time.h>
#include <iostream>

using std::cout;
using std::cerr;
using boost::shared_ptr;

/** Run some basic benchmarks of JPEG2000 encoding / decoding */
int
main (int argc, char* argv[])
{
	if (argc < 2) {
		cerr << "Syntax: " << argv[0] << " private-test-path\n";
		exit (EXIT_FAILURE);
	}

	int const decompress_count = 100;
	int const compress_count = 100;
	int const j2k_bandwidth = 100000000;

	struct timeval start;
	dcp::Data j2k (boost::filesystem::path (argv[1]) / "thx.j2c");

	gettimeofday (&start, 0);

	shared_ptr<dcp::OpenJPEGImage> xyz;
	for (int i = 0; i < decompress_count; ++i) {
		xyz = dcp::decompress_j2k (j2k, 0);
		cout << (i + 1) << " ";
	}
	cout << "\n";

	struct timeval stop;
	gettimeofday (&stop, 0);

	double start_seconds = start.tv_sec + double(start.tv_usec) / 1000000;
	double stop_seconds = stop.tv_sec + double(stop.tv_usec) / 1000000;
	if (dcp::built_with_debug) {
		cout << "Decompress (debug build): ";
	} else {
		cout << "Decompress: ";
	}
	cout << decompress_count / (stop_seconds - start_seconds) << " fps.\n";

	gettimeofday (&start, 0);

	for (int i = 0; i < compress_count; ++i) {
		dcp::compress_j2k (xyz, j2k_bandwidth, 24, false, false);
		cout << (i + 1) << " ";
	}
	cout << "\n";

	gettimeofday (&stop, 0);

	start_seconds = start.tv_sec + double(start.tv_usec) / 1000000;
	stop_seconds = stop.tv_sec + double(stop.tv_usec) / 1000000;
	if (dcp::built_with_debug) {
		cout << "Compress (debug build) ";
	} else {
		cout << "Compress ";
	}

	cout << (j2k_bandwidth / 1000000) << "Mbps: "
	     << compress_count / (stop_seconds - start_seconds) << " fps.";
}
