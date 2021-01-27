/*
    Copyright (C) 2021 Carl Hetherington <cth@carlh.net>

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


/** @file  src/verify_j2k.cc
 *  @brief Verification that JPEG2000 files meet requirements
 */


#include "compose.hpp"
#include "data.h"
#include "raw_convert.h"
#include "verify.h"
#include "verify_j2k.h"
#include <memory>
#include <vector>


using std::shared_ptr;
using std::map;
using std::runtime_error;
using std::string;
using std::vector;
using boost::optional;
using dcp::raw_convert;


class InvalidCodestream : public runtime_error
{
public:
	InvalidCodestream (string note)
		: runtime_error(note)
	{}
};


void
dcp::verify_j2k (shared_ptr<const Data> j2k, vector<VerificationNote>& notes)
{
	try {
		auto ptr = j2k->data();
		auto end = ptr + j2k->size();

		map<string, uint8_t> markers = {
			{ "SOC", 0x4f },
			{ "SIZ", 0x51 },
			{ "COD", 0x52 },
			{ "COC", 0x53 },
			{ "TLM", 0x55 },
			{ "QCD", 0x5c },
			{ "QCC", 0x5d },
			{ "POC", 0x5f },
			{ "COM", 0x64 },
			{ "SOT", 0x90 },
			{ "SOD", 0x93 },
			{ "EOC", 0xd9 },
		};

		auto marker_name_from_id = [&markers](uint8_t b) -> optional<string> {
			for (auto const& i: markers) {
				if (i.second == b) {
					return i.first;
				}
			}
			return {};
		};

		auto require_marker = [&](string name) {
			if (ptr == end || *ptr != 0xff) {
				throw InvalidCodestream ("missing marker start byte");
			}
			++ptr;
			if (ptr == end || *ptr != markers[name]) {
				throw InvalidCodestream ("missing_marker " + name);
			}
			++ptr;
		};

		auto get_8 = [&]() {
			if (ptr >= end) {
				throw InvalidCodestream ("unexpected end of file");
			}
			return *ptr++;
		};

		auto get_16 = [&]() {
			if (ptr >= (end - 1)) {
				throw InvalidCodestream ("unexpected end of file");
			}
			auto const a = *ptr++;
			auto const b = *ptr++;
			return b | (a << 8);
		};

		auto get_32 = [&]() -> uint32_t {
			if (ptr >= (end - 3)) {
				throw InvalidCodestream ("unexpected end of file");
			}
			auto const a = *ptr++;
			auto const b = *ptr++;
			auto const c = *ptr++;
			auto const d = *ptr++;
			return d | (c << 8) | (b << 16) | (a << 24);
		};

		auto require_8 = [&](uint8_t value, string note) {
			auto v = get_8 ();
			if (v != value) {
				throw InvalidCodestream (String::compose(note, v));
			}
		};

		auto require_16 = [&](uint16_t value, string note) {
			auto v = get_16 ();
			if (v != value) {
				throw InvalidCodestream (String::compose(note, v));
			}
		};

		auto require_32 = [&](uint32_t value, string note) {
			auto v = get_32 ();
			if (v != value) {
				throw InvalidCodestream (String::compose(note, v));
			}
		};

		require_marker ("SOC");
		require_marker ("SIZ");
		auto L_siz = get_16();
		if (L_siz != 47) {
			throw InvalidCodestream("unexpected SIZ size " + raw_convert<string>(L_siz));
		}

		get_16(); // CA: codestream capabilities
		auto const image_width = get_32();
		auto const image_height = get_32();
		auto const fourk = image_width > 2048;
		require_32 (0, "invalid top-left image x coordinate %1");
		require_32 (0, "invalid top-left image y coordinate %1");
		auto const tile_width = get_32();
		auto const tile_height = get_32();
		if (tile_width != image_width || tile_height != image_height) {
			notes.push_back ({ VerificationNote::Type::BV21_ERROR, VerificationNote::Code::INVALID_JPEG2000_TILE_SIZE });
		}
		require_32 (0, "invalid tile anchor x coordinate %1");
		require_32 (0, "invalid tile anchor y coordinate %1");
		require_16 (3, "invalid component count %1");
		for (auto i = 0; i < 3; ++i) {
			require_8 (12 - 1, "invalid bit depth %1");
			require_8 (1, "invalid horizontal subsampling factor %1");
			require_8 (1, "invalid vertical subsampling factor %1");
		}

		auto num_COD = 0;
		auto num_QCD = 0;
		/** number of POC markers in the main header */
		auto num_POC_in_main = 0;
		/** number of POC markers after the main header */
		auto num_POC_after_main = 0;
		bool main_header_finished = false;
		bool tlm = false;

		while (ptr < end)
		{
			require_8(0xff, "missing marker start byte");
			auto marker_id = get_8();
			auto marker_name = marker_name_from_id (marker_id);
			if (!marker_name) {
				char buffer[16];
				snprintf (buffer, 16, "%2x", marker_id);
				throw InvalidCodestream(String::compose("unknown marker %1", buffer));
			} else if (*marker_name == "SOT") {
				require_16(10, "invalid SOT size %1");
				get_16(); // tile index
				get_32(); // tile part length
				get_8(); // tile part index
				auto tile_parts = get_8();
				if (!fourk && tile_parts != 3) {
					notes.push_back ({ VerificationNote::Type::BV21_ERROR, VerificationNote::Code::INVALID_JPEG2000_TILE_PARTS_FOR_2K, raw_convert<string>(tile_parts) });
				}
				if (fourk && tile_parts != 6) {
					notes.push_back ({ VerificationNote::Type::BV21_ERROR, VerificationNote::Code::INVALID_JPEG2000_TILE_PARTS_FOR_2K, raw_convert<string>(tile_parts) });
				}
				main_header_finished = true;
			} else if (*marker_name == "SOD") {
				while (ptr < (end - 1) && (ptr[0] != 0xff || ptr[1] < 0x90)) {
					++ptr;
				}
			} else if (*marker_name == "SIZ") {
				throw InvalidCodestream ("duplicate SIZ marker");
			} else if (*marker_name == "COD") {
				num_COD++;
				get_16(); // length
				require_8(1, "invalid coding style %1");
				require_8(4, "invalid progression order %1"); // CPRL
				require_16(1, "invalid quality layers count %1");
				require_8(1, "invalid multi-component transform flag %1");
				require_8(fourk ? 6 : 5, "invalid number of transform levels %1");
				auto log_code_block_width = get_8();
				if (log_code_block_width != 3) {
					notes.push_back ({ VerificationNote::Type::BV21_ERROR, VerificationNote::Code::INVALID_JPEG2000_CODE_BLOCK_WIDTH, raw_convert<string>(4 * (2 << log_code_block_width)) });
				}
				auto log_code_block_height = get_8();
				if (log_code_block_height != 3) {
					notes.push_back ({ VerificationNote::Type::BV21_ERROR, VerificationNote::Code::INVALID_JPEG2000_CODE_BLOCK_HEIGHT, raw_convert<string>(4 * (2 << log_code_block_height)) });
				}
				require_8(0, "invalid mode variations");
				require_8(0, "invalid wavelet transform type %1"); // 9/7 irreversible
				require_8(0x77, "invalid precinct size %1");
				require_8(0x88, "invalid precinct size %1");
				require_8(0x88, "invalid precinct size %1");
				require_8(0x88, "invalid precinct size %1");
				require_8(0x88, "invalid precinct size %1");
				require_8(0x88, "invalid precinct size %1");
				if (fourk) {
					require_8(0x88, "invalid precinct size %1");
				}
			} else if (*marker_name == "QCD") {
				num_QCD++;
				auto const L_qcd = get_16();
				auto quantization_style = get_8();
				int guard_bits = (quantization_style >> 5) & 7;
				if (fourk && guard_bits != 2) {
					notes.push_back ({ VerificationNote::Type::BV21_ERROR, VerificationNote::Code::INVALID_JPEG2000_GUARD_BITS_FOR_4K, raw_convert<string>(guard_bits) });
				}
				if (!fourk && guard_bits != 1) {
					notes.push_back ({ VerificationNote::Type::BV21_ERROR, VerificationNote::Code::INVALID_JPEG2000_GUARD_BITS_FOR_2K, raw_convert<string>(guard_bits) });
				}
				ptr += L_qcd - 3;
			} else if (*marker_name == "COC") {
				get_16(); // length
				require_8(0, "invalid COC component number");
				require_8(1, "invalid coding style %1");
				require_8(5, "invalid number of transform levels %1");
				require_8(3, "invalid code block width exponent %1");
				require_8(3, "invalid code block height exponent %1");
				require_8(0, "invalid mode variations");
				require_8(0x77, "invalid precinct size %1");
				require_8(0x88, "invalid precinct size %1");
				require_8(0x88, "invalid precinct size %1");
				require_8(0x88, "invalid precinct size %1");
				require_8(0x88, "invalid precinct size %1");
				require_8(0x88, "invalid precinct size %1");
			} else if (*marker_name == "TLM") {
				auto const len = get_16();
				ptr += len - 2;
				tlm = true;
			} else if (*marker_name == "QCC" || *marker_name == "COM") {
				auto const len = get_16();
				ptr += len - 2;
			} else if (*marker_name == "POC") {
				if (main_header_finished) {
					num_POC_after_main++;
				} else {
					num_POC_in_main++;
				}

				auto require_8_poc = [&](uint16_t value, string note) {
					if (get_8() != value) {
						notes.push_back ({ VerificationNote::Type::BV21_ERROR, VerificationNote::Code::INCORRECT_JPEG2000_POC_MARKER, String::compose(note, value) });
					}
				};

				auto require_16_poc = [&](uint16_t value, string note) {
					if (get_16() != value) {
						notes.push_back ({ VerificationNote::Type::BV21_ERROR, VerificationNote::Code::INCORRECT_JPEG2000_POC_MARKER, String::compose(note, value) });
					}
				};

				require_16_poc(16, "invalid length %1");
				require_8_poc(0, "invalid RSpoc %1");
				require_8_poc(0, "invalid CSpoc %1");
				require_16_poc(1, "invalid LYEpoc %1");
				require_8_poc(6, "invalid REpoc %1");
				require_8_poc(3, "invalid CEpoc %1");
				require_8_poc(4, "invalid Ppoc %1");
				require_8_poc(6, "invalid RSpoc %1");
				require_8_poc(0, "invalid CSpoc %1");
				require_16_poc(1, "invalid LYEpoc %1");
				require_8_poc(7, "invalid REpoc %1");
				require_8_poc(3, "invalid CEpoc %1");
				require_8_poc(4, "invalid Ppoc %1");
			}
		}

		if (num_COD == 0) {
			throw InvalidCodestream("no COD marker found");
		}
		if (num_COD > 1) {
			throw InvalidCodestream("more than one COD marker found");
		}
		if (num_QCD == 0) {
			throw InvalidCodestream("no QCD marker found");
		}
		if (num_QCD > 1) {
			throw InvalidCodestream("more than one QCD marker found");
		}
		if (num_POC_in_main != 0 && !fourk) {
			notes.push_back ({ VerificationNote::Type::BV21_ERROR, VerificationNote::Code::INCORRECT_JPEG2000_POC_MARKER_COUNT_FOR_2K, raw_convert<string>(num_POC_in_main) });
		}
		if (num_POC_in_main != 1 && fourk) {
			notes.push_back ({ VerificationNote::Type::BV21_ERROR, VerificationNote::Code::INCORRECT_JPEG2000_POC_MARKER_COUNT_FOR_4K, raw_convert<string>(num_POC_in_main) });
		}
		if (num_POC_after_main != 0) {
			notes.push_back ({ VerificationNote::Type::BV21_ERROR, VerificationNote::Code::INVALID_JPEG2000_POC_MARKER_LOCATION });
		}
		if (!tlm) {
			notes.push_back ({ VerificationNote::Type::BV21_ERROR, VerificationNote::Code::MISSING_JPEG200_TLM_MARKER });
		}
	}
	catch (InvalidCodestream const& e)
	{
		notes.push_back ({VerificationNote::Type::ERROR, VerificationNote::Code::INVALID_JPEG2000_CODESTREAM, string(e.what()) });
	}
}

