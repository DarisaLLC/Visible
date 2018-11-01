//
//  boost_units_literals.h
//  Visible
//
//  Created by Arman Garakani on 10/31/18.
//

#ifndef boost_units_literals_h
#define boost_units_literals_h


#include <boost/units/cmath.hpp>
#include <boost/units/io.hpp>
#include <boost/units/systems/cgs/length.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/cgs.hpp>
#include <boost/units/quantity.hpp>
#include <iostream>

using boost::units::quantity;
using boost::units::cgs::centimeter;
using namespace boost::units::si;

quantity<boost::units::cgs::velocity> operator"" _cm_s(long double x);
quantity<boost::units::cgs::length> operator"" _cm(long double x);
quantity<boost::units::cgs::time> operator"" _s(long double x);



#endif /* boost_units_literals_h */
