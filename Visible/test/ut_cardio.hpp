#ifndef __CARDIO_UT__
#define __CARDIO_UT__


#include <sstream>
#include <fstream>
#include <cassert>
#include <iomanip>
#include <string>
#include <complex>
#include <iostream>
#include "core/gtest_env_utils.hpp"
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
#include "boost_units_literals.h"
using namespace svl;
using namespace boost::units;
using namespace boost::units::si;
using namespace std;
using namespace cm;
namespace bi=boost::units;

namespace cardio_ut
{
    static void run (std::shared_ptr<test_utils::genv>& dev_env_ref)
    {
        EXPECT_TRUE(dev_env_ref != nullptr);
        EXPECT_TRUE(dev_env_ref-> executable_path_exists ());
        
        double epsilon = 1e-5;
        cardio_model cmm;
        cmm.length(0.01_cm);
        cmm.elongation(0.01_cm);
        cmm.width(0.003_cm);
        cmm.thickness(0.0002_cm);
        cmm.shear_velocity(200.0_cm_s);
        cmm.shear_control(1.0f);
        cmm.run();
        
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

        std::string jpath = dev_env_ref->executable_folder().c_str();
        jpath = jpath + "/file.json";
        {
            std::ofstream os(jpath);
            cereal::JSONOutputArchive oar(os);
            oar( CEREAL_NVP(cmm.result()) );
        }
    
        std::stringstream ss;
        {
            cereal::JSONOutputArchive ar(ss);
            cereal::JSONOutputArchive ar2(std::cout,
                                          cereal::JSONOutputArchive::Options(2, cereal::JSONOutputArchive::Options::IndentChar::space, 2));
            ar( CEREAL_NVP(cmm.result()) );
            ar2( CEREAL_NVP(cmm.result()) );
            std::cout << ss.str().length() << std::endl;
            std::cout << ss.str() << std::endl;
        }
        
        {
            std::ifstream is(jpath);
            std::string str((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
            std::cout << "---------------------" << std::endl << str << std::endl << "---------------------" << std::endl;
        }
        
     }
};

#endif
