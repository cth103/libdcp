#include <cmath>
#include "gamma_lut.h"
#include "lut_cache.h"

using namespace libdcp;

LUTCache<GammaLUT> GammaLUT::cache;

GammaLUT::GammaLUT(int bits, float gamma)
	: LUT<float> (bits, gamma)
{
	int const bit_length = pow(2, bits);
	for (int i = 0; i < bit_length; ++i) {
		_lut[i] = pow(float(i) / (bit_length - 1), gamma);
	}
}
