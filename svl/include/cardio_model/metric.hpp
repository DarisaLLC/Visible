#ifndef _METRIC_HPP
#define _METRIC_HPP


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-private-field"

#include <iostream>

#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>

#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/complex.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>

#include <cereal/external/rapidjson/filestream.h>

#pragma GCC diagnostic pop

#include "boost/math/constants/constants.hpp"
#include <boost/units/physical_dimensions.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/systems/cgs.hpp>
#include <boost/units/io.hpp>
#include <boost/units/scale.hpp>
#include <boost/units/pow.hpp>
#include "boost/multi_array.hpp"
#include <Eigen/Dense>
#include <cassert>
#include <tuple>
#include <boost/units/detail/utility.hpp>
#include <boost/math/special_functions.hpp>
#include <iostream>
#include <strstream>

using namespace cereal;
using namespace std;
using namespace boost::math;
using Eigen::MatrixXd;
using namespace boost::units;
using namespace boost::units::si;
namespace bi=boost::units;


#include <boost/units/derived_dimension.hpp>
namespace boost {
    namespace units {
        /// derived dimension for moment : L^2 M T^-2
        typedef derived_dimension<length_base_dimension,2,
        mass_base_dimension,1,
        time_base_dimension,-2>::type moment_dimension;
    } // namespace units
    
} // namespace boost


#include <boost/units/physical_dimensions/stress.hpp>
#include <boost/units/make_system.hpp>
namespace boost {
    namespace units {
        namespace cgs {
            typedef unit<stress_dimension,cgs::system>     stress;
            typedef unit<moment_dimension,cgs::system>     moment;
        } // namespace si
    } // namespace units
} // namespace boost


// string output
template<class Y>
inline
std::string units_string (const quantity<Y>& val)
{
    using namespace boost::units;
    using boost::units::cgs::centimeter;
    using boost::units::cgs::gram;
    using boost::units::cgs::second;
    using boost::units::cgs::dyne;
    std::strstream ss;
   
    ss << symbol_format << val;
    std::string s_s = ss.str();
    return s_s;
}

template <class Archive, class Y, cereal::traits::EnableIf<cereal::traits::is_text_archive<Archive>::value> = cereal::traits::sfinae>
inline void serialize (Archive & ar, quantity<Y>& val)
{
    using boost::units::cgs::centimeter;
    using boost::units::cgs::gram;
    using boost::units::cgs::second;
    using boost::units::cgs::dyne;

    ar (
    CEREAL_NVP (val.value()),
        CEREAL_NVP (symbol_format));

}

//template<> void save<class Archive, cgs::length>(Archive & ar, quantity<cgs::length>& val);

namespace cm
{
    typedef Eigen::Matrix<double,4,1> integral4_t;
    
    struct half_space
    {
        quantity<cgs::length> a;
        quantity<cgs::length> b;
        quantity<cgs::area> area (struct half_space& hs)
        {
            return 4.0 * hs.a * hs.b;
        }
        
    };
    
    struct result
    {
        
        quantity<cgs::length>   length;
        quantity<cgs::length>  width;
        quantity<cgs::length>  thickness;
        quantity<cgs::length>  elongation;
        quantity<cgs::force> total_simple_dipole;
        quantity<cgs::force> total_reactive;
        quantity<cgs::stress> average_contact_stress;
        double moment_of_dipole;
        double moment_arm_fraction;
        double average_contraction_strain;
        
        
        
        
        friend inline ostream& operator<<(ostream& out, const result& m)
        {
            out << "{";
            out << m.length << std::endl;
            out << m.width << std::endl;
            out << m.thickness << std::endl;
            out << m.elongation << std::endl;
            out << m.total_simple_dipole;
            out << m.total_reactive;
            out << m.average_contact_stress;
            out << m.moment_of_dipole;
            out << m.moment_arm_fraction;
            out << m.average_contraction_strain;
            out << "}";
            return out;
        }
    };
    
    template <class Archive>
    void serialize(Archive & ar, result & t)
    {
        ar(
           cereal::make_nvp("length", units_string(t.length)),
           cereal::make_nvp("width", units_string(t.width)),
           cereal::make_nvp("thickness", units_string(t.thickness)),
           cereal::make_nvp("elongation", units_string(t.elongation)),
           cereal::make_nvp("total simple dipole", units_string(t.total_simple_dipole)),
           cereal::make_nvp("total reactive", units_string(t.total_reactive)),
           cereal::make_nvp("average contact stress ", units_string(t.average_contact_stress)),
           cereal::make_nvp("total dipole moment", t.moment_of_dipole),
           cereal::make_nvp("moment arm as fraction of cell length ", t.moment_arm_fraction),
           cereal::make_nvp("average contraction strain", t.average_contraction_strain));
    }
    
    
    // To create a new json template
    template <typename T>
    void output_json(const std::string & title, const T & tt)
    {
        cereal::JSONOutputArchive oar(std::cout);
        oar(cereal::make_nvp(title, tt));
    }
    
}


#endif
