

#ifndef _CARDIO_MODEL_HPP
#define _CARDIO_MODEL_HPP

# include <assert.h> // .h to support old libraries w/o <cassert> - effect is the same
#include "core/core.hpp"
#include "core/pair.hpp"
#include "cardio_model/metric.hpp"
#include "cardio_model/cerruti_rectangle_integral.hpp"
#include "core/boost_units_literals.h"
#include "core/stl_utils.hpp"
#include <cmath>
#include <map>
#include "core/static.hpp"

using namespace boost::units;
//using namespace boost::units::si;
//namespace bi=boost::units;

//**************************************************************************
//                 Mechanical model for a cardiomyocyte
// Arguments:
//   N = power index controlling the shape of the contact stresses p
//     > 0 -->  p = (x/a)^N           (e.g. if N=1 --> linear distribution)
//     < 0 -->  p = (x/a)/sqrt(1-(x/a)^2) (Boussinesq-Cerruti distribution)
//           (observe that it produces a nearly linear displacement pattern)
//       Note: whatever the value of N, p is antisymmetric w.r.t the center
//   dL = cell elongation
//    L = cell length
//    W = cell width
//    t = cell thickness
//   Cs = shear wave velocity of gel
// The input parameters are assumed to be given in the CGS system (cm-g-s)
//
// Assumptions:
//   The cell is a thin rectangular elastic plate of negligible stiffness
//   The cell is bonded onto the surface of a gel, which is idealized as an
//     elastic half-space with uniform properties
//   As the cell contracts, it exerts tangential shearing stresses onto
//     the gel which are symmetric with respect to the cell's midplane.
//   The shearing stresses are assumed to be distributed according to some
//     rule and scaled to produce the observed elongation. The rule
//     controlling the shape of the shearing forces is defined by the
//     parameter N as follows:
//     N >= 0  ==> shearing stresses distributed according to (x/a)^N
//        < 0  ==> distribution according to 1/sqrt(1-(x/a)^2)
//
//   Dynamic effects are neglected. Instead, a Cerruti-type solution
//   consisting of a set of narrow rectangular loads applied onto the
//   surface of an elastic half-space is used.
//
//
//           Written by Eduardo Kausel
//           MIT, Room 1-271, Cambridge, MA, kausel@mit.edu
//           V-2, March 6, 2013
//
//    C++ Implementation by Arman Garakani
//**************************************************************************



namespace cm  // protection from unintended ADL
{
    /// shear modulus in cgs units
    template<class Y>
    quantity<cgs::stress,Y>
    shearModulus(const quantity<cgs::mass_density,Y>& Rho,
                 const quantity<cgs::velocity,Y>& V)
    {
        using namespace boost::units::cgs;
        return (Rho * V * V);
    }
    // http://stackoverflow.com/a/39164765
    struct double_iota
    {
        double_iota(double inc, double init_value = 0.0) : _value(init_value), _inc(inc) {}
        
        operator double() const { return _value; }
        double_iota& operator++() { _value += _inc; return *this; }
        double _value;
        double _inc;
    };
    
    class cardio_model_params_template
    {
    public:
        typedef std::tuple< std::string,float,
        std::string, quantity<boost::units::cgs::length>,
        std::string, quantity<boost::units::cgs::length>,
        std::string, quantity<boost::units::cgs::length>,
        std::string, quantity<boost::units::cgs::length>,
        std::string, quantity<boost::units::cgs::velocity>> params_t;
        
        cardio_model_params_template ();
        
        
    const params_t& get_template () const;
    private:
        params_t m_template;
        
        
    };
    

    class cardio_model
    {
        
    public:
        typedef cardio_model_params_template::params_t params_t;
        
        cardio_model ();
  
        
        void shear_control (float sc) const ;
        void elongation (quantity<boost::units::cgs::length> elo) const;
        void length (quantity<boost::units::cgs::length> len) const;
        void width (quantity<boost::units::cgs::length> width) const;
        void thickness (quantity<boost::units::cgs::length> thick) const;
        void shear_velocity(quantity<boost::units::cgs::velocity> sh_velo) const;
        
        const float shear_control () const;
        const quantity<boost::units::cgs::length> elongation () const;
        const quantity<boost::units::cgs::length> length () const;
        const quantity<boost::units::cgs::length> width () const;
        const quantity<boost::units::cgs::length> thickness () const;
        const quantity<boost::units::cgs::velocity> shear_velocity () const;
        
//        cardio_model (float shear = 1.0f,//parameter controlling shear distribution
//                      float e_cm = 0.0100,// total elongation, cm
//                      float l_cm = 0.0100,// total length of cell [cm]
//                      float w_cm = 0.003, // width [cm]
//                      float t_cm = 0.0002,// thickness [cm]
//                      float shearv_cm_sec = 200.0) :
//
        void run ();
        
        const struct cm::result& result () const;
        
    private:
        static params_t default_set ();
        
        mutable params_t m_params;
        quantity<cgs::force> factorOC_;
        quantity<cgs::stress> G_gel_;
        quantity<cgs::velocity> Cs_;
        quantity<cgs::area> A_cell_;
        quantity<cgs::area> A_;
        half_space half_;
        quantity<cgs::length> a_;
        quantity<cgs::length> b_;
        quantity<cgs::mass_density> rho_;
        quantity<cgs::length> dx_;
        std::pair<size_t,size_t> n_;
        quantity<cgs::length> t_;
        quantity<cgs::length> w_;
        quantity<cgs::length> l_;
        quantity<cgs::length> dl_;
        std::vector<double> x_;
        float N_;
        cm::result res;
        
        Eigen::ArrayXXd F_;
        std::vector<double> Q_;
        
  
    };
}

#endif
