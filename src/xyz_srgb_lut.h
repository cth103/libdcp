#include "lut.h"
#include "lut_cache.h"

namespace libdcp {

class XYZsRGBLUT : public LUT<int>
{
public:
	XYZsRGBLUT(int colour_depth, float gamma);
	static LUTCache<XYZsRGBLUT> cache;
};

}
