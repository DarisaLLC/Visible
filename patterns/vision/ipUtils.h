#ifndef __IPUTILS__
#define __IPUTILS__

#include "roiWindow.h"
#include "pair.hpp"
#include <vector>

template <typename T, typename P = T>
static tpair<P> GetRange(T * pels, uint32_t num)
{
    tpair<P> extremes;
    extremes.first = std::numeric_limits<T>::max();
    extremes.second = std::numeric_limits<T>::min();
    T * pel = pels;
    T * passed_end = pel + num;
    for (; pel < passed_end; pel++)
    {
        T pval = *pel;
        if (pval > extremes.second) extremes.second = pval;
        if (pval < extremes.first) extremes.first = pval;
    }
    return extremes;
}


//
//   |                      /----------------
//  O|                     / |
//  u|                    /  |
//  t|                   /   |
//  p|------------------/    |
//  u|                  |    |
//  t|<---------------->|<-->|
//       |  "offset"         "spread"
//   |
// --+---Input--------------------------------
//   |


roiWindow<P8U> convert(const uint16_t * pels, uint32_t width, uint32_t height);

std::pair<roiWindow<P8U>, tpair<uint16_t>> invariantStructureSegmentation(const uint16_t * src, int width, int height, int scan = 5);

void upBiValueMap(roiWindow<P8U> & src, tpair<uint16_t> & range, std::shared_ptr<uint16_t> &);


#endif
