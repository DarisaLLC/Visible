//
//  nr_support.hpp
//  Visible
//
//  Created by Arman Garakani on 3/14/21.
//

#ifndef nr_support_hpp
#define nr_support_hpp

#include "nr/nr.h"
using namespace NR;

void NR::avevar(Vec_I_DP &data, DP &ave, DP &var);

void NR::period(Vec_I_DP &x, Vec_I_DP &y, const DP ofac, const DP hifac,
				Vec_O_DP &px, Vec_O_DP &py, int &nout, int &jmax, DP &prob);



#endif /* nr_support_hpp */
