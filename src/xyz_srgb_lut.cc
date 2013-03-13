#include <iostream>
#include <cmath>
#include "xyz_srgb_lut.h"

using namespace libdcp;

LUTCache<XYZsRGBLUT> XYZsRGBLUT::cache;

XYZsRGBLUT::XYZsRGBLUT(int bits, float gamma)
	: LUT<int> (bits, gamma)
{
	int const bit_length = pow(2, bits);

	for (int i = 0; i < bit_length; ++i) {
		float v = float(i) / (bit_length - 1);
		if (v < (0.04045 / 12.92)) {
			v *= 12.92;
		} else {
			v = (1.055 * pow (v, (1 / gamma))) - 0.055;
		}

		_lut[i] = int(v * 255);
	}
}
