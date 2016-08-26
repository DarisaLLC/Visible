#ifndef __IPUTILS__
#define __IPUTILS__

#include "vision/roiWindow.h"
#include "core/pair.hpp"
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

std::pair<uint8_t,std::vector<std::pair<uint8_t, int> > > segmentationByInvaraintStructure(roiWindow<P8U>& src);

roiWindow<P8U> extractAtLevel (roiWindow<P8U>& src, uint8_t thr);



roiWindow<P8U> convert(const uint16_t * pels, uint32_t width, uint32_t height);

void upBiValueMap(roiWindow<P8U> & src, tpair<uint16_t> & range, std::shared_ptr<uint16_t> &);


#endif
