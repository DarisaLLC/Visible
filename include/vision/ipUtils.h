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

namespace sliceSimilarity{
    
    typedef P8U pixel_t;
    typedef struct  { pixel_t::value_type val; float corr; float weight;} slice_result_t;
    typedef std::vector<slice_result_t> results_t;
    
    slice_result_t threshold (roiWindow<pixel_t>& src,results_t &, bool output = false);
}


roiWindow<P8U> extractAtLevel (roiWindow<P8U>& src, uint8_t thr);



roiWindow<P8U> convert(const uint16_t * pels, uint32_t width, uint32_t height);

void upBiValueMap(roiWindow<P8U> & src, tpair<uint16_t> & range, std::shared_ptr<uint16_t> &);

std::map<uint8_t, double> zscore (roiWindow<P8U>& src);

#endif
