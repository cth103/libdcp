#ifndef LIBDCP_LUT_CACHE_H
#define LIBDCP_LUT_CACHE_H

#include <list>
#include <boost/shared_ptr.hpp>

template<class T>
class LUTCache
{
public:
	boost::shared_ptr<T> get (int bit_depth, float gamma)
	{
		for (typename std::list<boost::shared_ptr<T> >::iterator i = _cache.begin(); i != _cache.end(); ++i) {
			if ((*i)->bit_depth() == bit_depth && (*i)->gamma() == gamma) {
				return *i;
			}
		}

		boost::shared_ptr<T> lut (new T (bit_depth, gamma));
		_cache.push_back (lut);
		return lut;
	}

private:
	std::list<boost::shared_ptr<T> > _cache;
};

#endif
