//
//  boost_literals.cpp
//  Visible
//
//  Created by Arman Garakani on 11/1/18.
//

#include "core/boost_units_literals.h"


quantity<boost::units::cgs::velocity> operator"" _cm_s(long double x)
{
    return double(x)*boost::units::cgs::centimeter_per_second;
}

quantity<boost::units::cgs::length> operator"" _cm(long double x)
{
    return quantity<boost::units::cgs::length>{double(x)*centimeter};
}

quantity<boost::units::cgs::time> operator"" _s(long double x)
{
    return double(x)*second;
}
