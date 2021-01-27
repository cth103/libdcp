/*
    Copyright (C) 2015-2019 Carl Hetherington <cth@carlh.net>

    This file is part of libdcp.

    libdcp is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    libdcp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdcp.  If not, see <http://www.gnu.org/licenses/>.

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.

    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.
*/


#include "array_data.h"
#include "util.h"
#include "version.h"
#include "j2k_transcode.h"
#include "openjpeg_image.h"
#include <sys/time.h>
#include <iostream>
#include <cstdio>

using std::cout;
using std::cerr;
using std::shared_ptr;

class Timer
{
public:
	Timer ()
		: _total (0)
	{

	}

	void start ()
	{
		gettimeofday (&_start, 0);
	}

	void stop ()
	{
		struct timeval stop;
		gettimeofday (&stop, 0);
		_total += (stop.tv_sec + stop.tv_usec / 1e6) - (_start.tv_sec + _start.tv_usec / 1e6);
	}

	double get ()
	{
		return _total;
	}

private:
	double _total;
	struct timeval _start;
};

/** Run some basic benchmarks of JPEG2000 encoding / decoding */
int
main (int argc, char* argv[])
{
	if (argc < 2) {
		cerr << "Syntax: " << argv[0] << " private-test-path\n";
		exit (EXIT_FAILURE);
	}

	int const count = 100;
	int const j2k_bandwidth = 100000000;

	dcp::ArrayData j2k (boost::filesystem::path (argv[1]) / "thx.j2c");

	Timer decompress;
	Timer compress;

	dcp::ArrayData recomp;
	for (int i = 0; i < count; ++i) {
		decompress.start ();
		shared_ptr<dcp::OpenJPEGImage> xyz = dcp::decompress_j2k (j2k, 0);
		decompress.stop ();
		compress.start ();
		recomp = dcp::compress_j2k (xyz, j2k_bandwidth, 24, false, false);
		compress.stop ();
		cout << (i + 1) << " ";
		cout.flush ();
	}
	cout << "\n";

	cout << "Decompress: " << count / decompress.get() << " fps.\n";
	cout << "Compress:   " << count / compress.get() << " fps.\n";

	FILE* f = fopen ("check.j2c", "wb");
	fwrite (recomp.data(), 1, recomp.size(), f);
	fclose (f);
}
