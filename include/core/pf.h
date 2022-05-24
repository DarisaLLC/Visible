//
//  pf.h
//
//
//  Created by Arman Garakani on 6/6/19.
//
#ifndef PF_H
#define PF_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

#define EPS 2.2204e-16

using namespace std;

namespace svl{
void findPeaks(vector<double> x0, vector<int>& peakInds);
}

#endif
