#ifndef __CARDIO_UT__
#define __CARDIO_UT__

#include <complex>
#include <iostream>

#include <boost/typeof/std/complex.hpp>

#include <boost/units/systems/si/energy.hpp>
#include <boost/units/systems/si/force.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/electric_potential.hpp>
#include <boost/units/systems/si/current.hpp>
#include <boost/units/systems/si/resistance.hpp>
#include <boost/units/systems/si/io.hpp>
#include <cassert>
#include "cardio_model/cardiomyocyte_model.hpp"

using namespace svl;
using namespace boost::units;
using namespace boost::units::si;
using namespace std;
using namespace cm;
namespace bi=boost::units;

namespace cardio_ut
{
    static void run ()
    {
        
        double epsilon = 1e-5;
        cardio_model cmm;
        EXPECT_NEAR(cmm.result().length.value(), 0.01, epsilon);
        EXPECT_NEAR(cmm.result().width.value(), 0.003, epsilon);
        EXPECT_NEAR(cmm.result().thickness.value(), 0.00020, epsilon);
        EXPECT_NEAR(cmm.result().elongation.value(), 0.01, epsilon);
        EXPECT_NEAR(cmm.result().total_simple_dipole.value(), 3.39292, epsilon);
        EXPECT_NEAR(cmm.result().total_reactive.value (), 2.57928, epsilon);
        EXPECT_NEAR(cmm.result().average_contact_stress.value ()*1e-6, 0.17195, epsilon);
        EXPECT_NEAR(cmm.result().average_contraction_strain, 1.0, epsilon);
        EXPECT_NEAR(cmm.result().moment_arm_fraction, 0.66667, epsilon);
        EXPECT_NEAR(cmm.result().moment_of_dipole, 0.01720, epsilon);
        
        std::stringstream ss;
        {
            cereal::JSONOutputArchive ar(ss);
            cardio_model cmm;
            ar( CEREAL_NVP(cmm.result()) );
        }
        std::cout << ss.str() << std::endl;

     }
};

#endif
