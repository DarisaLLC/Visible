#ifndef __BOOST_UNITS__
#define __BOOST_UNITS__

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
#include <Eigen/Dense>
#include <cassert>
#include "core/pair.hpp"

using namespace svl;
using Eigen::MatrixXd;
using namespace boost::units;
using namespace boost::units::si;
using namespace std;
namespace bi=boost::units;

quantity<energy>
work(const quantity<force>& F, const quantity<bi::si::length>& dx)
{
    return F * dx; // Defines the relation: work = force * distance.
}

namespace units_ut
{
    static void run ()
    {
        /// Test calculation of work.
        quantity<force>     F(2.0 * bi::si::newton); // Define a quantity of force.
        quantity<bi::si::length>    dx(2.0 * bi::si::meter); // and a distance,
        quantity<bi::si::energy>    E(work(F,dx));  // and calculate the work done.
        
        std::cout << "F  = " << F << std::endl
        << "dx = " << dx << std::endl
        << "E  = " << E << std::endl
        << std::endl;
        
        /// Test and check complex quantities.
        typedef std::complex<double> complex_type; // double real and imaginary parts.
        
        // Define some complex electrical quantities.
        quantity<electric_potential, complex_type> v = complex_type(12.5, 0.0) * volts;
        quantity<current, complex_type>            i = complex_type(3.0, 4.0) * amperes;
        quantity<resistance, complex_type>         z = complex_type(1.5, -2.0) * ohms;
        
        std::cout << "V   = " << v << std::endl
        << "I   = " << i << std::endl
        << "Z   = " << z << std::endl
        // Calculate from Ohm's law voltage = current * resistance.
        << "I * Z = " << i * z << std::endl
        // Check defined V is equal to calculated.
        << "I * Z == V? " << std::boolalpha << (i * z == v) << std::endl
        << std::endl;
    }
};

namespace eigen_ut
{
    static void clone_column (Eigen::ArrayXXd& F)
    {
        for (auto i = 0; i < F.rows(); i++)
            for (auto j = 0; j < F.cols() - i; j++)
            {
                F(i+j,j) = F(i,0);
                F(j,i+j) = F(i+j,j);
                
            }
        for (auto i = 0; i < F.rows(); i++)
            F(0,i) = F(i,0);
 
    }
    
    static void run ()
    {
        Eigen::ArrayXXd F_(8,8);
        
        F_.setZero ();
        
        for (auto i = 0; i < F_.rows(); i++)
            F_(i,0) = i+1;
        
        
        std::cout << F_ << std::endl;
        
        clone_column (F_);
        
        std::cout << F_ << std::endl;
        
    }
}
#endif
