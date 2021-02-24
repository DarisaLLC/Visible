//
//  cardiomyocyte_detail.hpp
//  Visible
//
//  Created by Arman Garakani on 11/6/18.
//

#ifndef cardiomyocyte_detail_h
#define cardiomyocyte_detail_h

#include "cardio_model/cardiomyocyte_model.hpp"


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



using namespace cm;

void clone_column (Eigen::ArrayXXd& F);
void applied_shearing_stress (const float N, const size_t nx, std::vector<double>& output);

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



cardio_model_params_template::cardio_model_params_template (){
    const auto no_length = 0.0_cm;
    const auto no_velocity = 0.0_cm_s;
    
    std::get<0>(m_template) = "shear_ctrl";
    std::get<1>(m_template) = 1.0f;
    std::get<2>(m_template) = "elongation";
    std::get<3>(m_template) = no_length;
    std::get<4>(m_template) = "length";
    std::get<5>(m_template) = no_length;
    std::get<6>(m_template) = "width";
    std::get<7>(m_template) = no_length;
    std::get<8>(m_template) = "thickness";
    std::get<9>(m_template) = no_length;
    std::get<10>(m_template) = "shesr";
    std::get<11>(m_template) = no_velocity;
}

const   cardio_model_params_template::params_t&  cardio_model_params_template::get_template ()  const { return m_template; }


SINGLETON_FCN(cardio_model_params_template,get_default)

cardio_model::cardio_model (){
    m_params = default_set();
}


//#define MULOCK   std::lock_guard<std::mutex> lock( m_mutex )

void cardio_model::shear_control (float sc) const {  std::get<1>(m_params) = sc; }
void cardio_model::elongation (quantity<boost::units::cgs::length> elo) const { std::get<3>(m_params) = elo; }
void cardio_model::length (quantity<boost::units::cgs::length> len) const { std::get<5>(m_params) = len; }
void cardio_model::width (quantity<boost::units::cgs::length> width) const { std::get<7>(m_params) = width; }
void cardio_model::thickness (quantity<boost::units::cgs::length> thick) const { std::get<9>(m_params) = thick; }
void cardio_model::shear_velocity(quantity<boost::units::cgs::velocity> sh_velo) const { std::get<11>(m_params) = sh_velo; }

const float cardio_model::shear_control () const {return std::get<1>(m_params); }
const quantity<boost::units::cgs::length> cardio_model::elongation () const {return std::get<3>(m_params); }
const quantity<boost::units::cgs::length> cardio_model::length () const {return std::get<5>(m_params); }
const quantity<boost::units::cgs::length> cardio_model::width () const {return std::get<7>(m_params); }
const quantity<boost::units::cgs::length> cardio_model::thickness () const {return std::get<9>(m_params); }
const quantity<boost::units::cgs::velocity> cardio_model::shear_velocity () const {return std::get<11>(m_params); }

//        cardio_model (float shear = 1.0f,//parameter controlling shear distribution
//                      float e_cm = 0.0100,// total elongation, cm
//                      float l_cm = 0.0100,// total length of cell [cm]
//                      float w_cm = 0.003, // width [cm]
//                      float t_cm = 0.0002,// thickness [cm]
//                      float shearv_cm_sec = 200.0) :
//
void cardio_model::run (){
    Cs_ = shear_velocity();
    dl_= elongation();
    l_ = length();
    t_ = thickness();
    w_ = width();
    N_ = shear_control();

//    std::unique_lock<std::mutex> lock(m_mutex);
    n_ = std::pair<size_t,size_t>(1000, 1000);
    F_ = Eigen::ArrayXXd (1000,1000);
    
    rho_ = ( 1.08 * ( bi::cgs::grams / bi::pow<3>(cgs::centimeters)) );
    dx_ = ( (l_.value() / static_cast<double>(n_.first ))* bi::cgs::centimeters);
    static double poisson_ratio (0.5);
    F_.setZero();
    half_.a = dx_ / 2.0;
    half_.b = w_ / 2.0;
    G_gel_ = cm::shearModulus(rho_, Cs_);
    A_cell_ = w_*l_;
    A_ = half_.area (half_);
    x_.resize (n_.first);
    std::iota(x_.begin(),x_.end(), cm::double_iota(dx_.value(), 0.0));
    Eigen::Map<Eigen::VectorXd> eX(x_.data(), x_.size());
    Eigen::VectorXd xn = eX;
    Eigen::VectorXd vecOfa (n_.first);
    vecOfa.setConstant(half_.a.value ());
    xn += vecOfa;
    
    
    factorOC_ = boost::math::constants::pi<double>() * G_gel_ * A_;
    static double zerod (0.0);
    double C = 1.0 / factorOC_.value();
    
    const double& a = half_.a.value();
    const double& b = half_.b.value();
    
    
    for (auto i = 0; i < x_.size(); i++)
    {
        cm::integral4_t I = cm::cerruti_rect::integrate(x_[i], zerod, a, b) * C;
        F_(i,0) = (1.0 - poisson_ratio)*I(0)+poisson_ratio*I(1);
    }
    clone_column(F_);
    
    applied_shearing_stress(N_, n_.first, Q_);
    Eigen::Map<Eigen::VectorXd> eQ(Q_.data(), Q_.size());
    
    Eigen::VectorXd U = F_.matrix() * eQ;
    double scale = (dl_.value() /2.0)/U(0);
    U *= scale;
    auto p = scale * eQ;
    quantity<cgs::force> total_exerted = p.segment (0, n_.first/2).sum() * bi::cgs::dynes;
    
    res.length = l_;
    res.width = w_;
    res.thickness = t_;
    res.elongation = dl_;
    res.total_reactive = total_exerted;
    res.moment_of_dipole = (- xn.dot(p));
    res.moment_arm_fraction = res.moment_of_dipole / res.total_reactive.value() / l_.value();
    
    res.average_contact_stress = total_exerted/((w_*l_)/2.0);
    res.total_simple_dipole = G_gel_ * l_ * dl_ * (boost::math::constants::pi<double>() / 4.0);
    res.average_contraction_strain = dl_/l_;
    
    
}

const struct cm::result& cardio_model::result () const
{
    return res;
}


cardio_model::params_t cardio_model::default_set () { return get_default().get_template(); }



void clone_column (Eigen::ArrayXXd& F)
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

void applied_shearing_stress (const float N, const size_t nx, std::vector<double>& output)
{
    output.resize (nx, 0);
    size_t n2 = nx / 2;
    std::vector<double>::iterator sItr =output.begin();
    std::vector<double>::iterator rItr =output.begin();
    std::advance (sItr, n2);
    std::advance (rItr, n2-1);
    
    // simple dipole
    if ( N == 0)
    {
        output[0] = 1; // pair ----> (left end)
        output[output.size()-1] = -1; // <------ ( right end )
    }
    else if (N > 0)
    {
        // polynomial distribution (linear, quadratic, whatever)
        
        for (double i = 0.5; i <= (n2-0.5); i+= 1.0)
        {
            double dd = (i*2)/nx;
            dd = std::pow(dd, N);
            *sItr++ = dd;
            *rItr-- = - dd;
        }
    }
    else
    {
        // Distribution as in Boussinesq-Cerruti plate problem
        for (double i = 1.0; i <= n2; i+= 1.0)
        {
            double dd = (2*(i - 0.5))/nx;
            dd = dd / std::sqrt(1.0 - dd*dd);
            *sItr++ = dd;
            *rItr-- = - dd;
        }
    }
}


#endif /* cardiomyocyte_detail_h */
