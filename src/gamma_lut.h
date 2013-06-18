#include "lut.h"
#include "lut_cache.h"

namespace libdcp {

class GammaLUT : public LUT<float>
{
public:
	GammaLUT (int bit_length, float gamma);
	static LUTCache<GammaLUT> cache;
};

}
