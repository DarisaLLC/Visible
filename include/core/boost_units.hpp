//
//  boost_units.hpp
//  Visible
//
//  Created by Arman Garakani on 11/1/18.
//

#ifndef boost_units_h
#define boost_units_h

#include <boost/units/systems/cgs/base.hpp>
#include <boost/units/base_units/metric/micron.hpp>

namespace boost {
    
    namespace units {
        
        namespace cgs {
            
            typedef unit<length_dimension,cgs::system>   length;
            
            BOOST_UNITS_STATIC_CONSTANT(micron,length);
            BOOST_UNITS_STATIC_CONSTANT(microns,length);
            
        } // namespace cgs
        
    } // namespace units
    
} // namespace boost

#endif /* boost_units_h */
