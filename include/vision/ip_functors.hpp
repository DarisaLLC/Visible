#ifndef __FUNCTOR__
#define __FUNCTOR__

#include <stdio.h>
#include <stdlib.h>
#include "gtest/gtest.h"
#include "vision/roiWindow.h"
#include "vision/self_similarity.h"
#include "core/core.hpp"
#include "core/simple_timing.hpp"

#include <functional>

template<typename OUT>
class image_pairwise
{
public:
    virtual double operator()(const roiWindow<P8U>&,
                            const roiWindow<P8U>&, OUT&) = 0;
};


class svl_corr  : public image_pairwise<CorrelationParts>
{
public:
    double operator()(const roiWindow<P8U>& i,
                            const roiWindow<P8U>& m, CorrelationParts& cp)
    {
        Correlation::point(i, m, cp);
        return cp.r();
    }
};











#endif

