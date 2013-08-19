/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

#include <cmath>
#include <boost/filesystem.hpp>
#include <libxml++/libxml++.h>
#include "KM_prng.h"
#include "dcp.h"
#include "util.h"
#include "metadata.h"
#include "types.h"
#include "exceptions.h"
#include "subtitle_asset.h"
#include "picture_asset.h"
#include "sound_asset.h"
#include "reel.h"
#include "certificates.h"
#include "crypt_chain.h"
#include "gamma_lut.h"
#include "cpl.h"
#include "encryption.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libdcp_test
#include <boost/test/unit_test.hpp>

using std::string;
using std::cout;
using std::vector;
using std::list;
using boost::shared_ptr;

struct TestConfig
{
	TestConfig()
	{
		libdcp::init ();
	}
};

BOOST_GLOBAL_FIXTURE (TestConfig);

static string
j2c (int)
{
	return "test/data/32x32_red_square.j2c";
}

static string
wav (libdcp::Channel)
{
	return "test/data/1s_24-bit_48k_silence.wav";
}

static string test_corpus = "../libdcp-test";

#include "bias_to_string_test.cc"
#include "lut_test.cc"
#include "util_test.cc"
#include "decryption_test.cc"
#include "kdm_test.cc"
#include "dcp_test.cc"
#include "error_test.cc"
#include "read_dcp_test.cc"
#include "subtitle_tests.cc"
#include "dcp_time_test.cc"
#include "color_test.cc"
#include "recovery_test.cc"
#include "certificates_test.cc"

//BOOST_AUTO_TEST_CASE (crypt_chain)
//{
//	boost::filesystem::remove_all ("build/test/crypt");
//	boost::filesystem::create_directory ("build/test/crypt");
//	libdcp::make_crypt_chain ("build/test/crypt");
//}

//#include "encryption_test.cc"
