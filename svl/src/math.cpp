


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <algorithm>   //Needed for std::reverse.
#include <cmath>       //Needed for fabs, signbit, sqrt, etc...
#include <complex>
#include <exception>
#include <fstream>
#include <functional>  //Needed for passing kernel functions to integration schemes.
#include <iomanip>     //Needed for std::setw() for pretty-printing.
#include <iterator>
#include <limits>      //Needed for std::numeric_limits::max().
#include <list>
#include <map>
#include <numeric>
#include <stdexcept>
#include <string>      //Needed for stringification routines.
#include <tuple>       //Needed for Spearman's Rank Correlation Coeff, other statistical routines.
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/moreMath.h"
#include "core/gen_utils.hpp"
#include "core/core.hpp"

using namespace svl;



//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------- vec3: A three-dimensional vector -------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//Constructors.
template <class T>    vec3<T>::vec3(){   x=(T)(0);   y=(T)(0);   z=(T)(0);  }

//    template vec3<int>::vec3(void);
//    template vec3<long int>::vec3(void);
    template vec3<float>::vec3(void);
    template vec3<double>::vec3(void);


template <class T>    vec3<T>::vec3(T a, T b, T c) : x(a), y(b), z(c) { }

//    template vec3<int>::vec3(int, int, int);
//    template vec3<long int>::vec3(long int, long int, long int);
    template vec3<float>::vec3(float, float, float);
    template vec3<double>::vec3(double, double, double);

    
template <class T>    vec3<T>::vec3( const vec3<T> &in ) : x(in.x), y(in.y), z(in.z) { }

//    template vec3<int>::vec3( const vec3<int> & );
//    template vec3<long int>::vec3( const vec3<long int> & );
    template vec3<float>::vec3( const vec3<float> & );
    template vec3<double>::vec3( const vec3<double> & );

    
    
//More general: (but is it needed?)
//template<class Ch,class Tr,class T>     std::basic_ostream<Ch,Tr> & operator<<( std::basic_ostream<Ch,Tr> &&out, const vec3<T> &L ){
//    out << "(" << L.x << ", " << L.y << ", " << L.z << ")";
//    return out;
//}
template <class T>    std::ostream & operator<<( std::ostream &out, const vec3<T> &L ){
    //Note: This friend is templated (Y) within the templated class (T). We only
    // care about friend template when T==Y.
    //
    //There is significant whitespace here!
    out << "(" << L.x << ", " << L.y << ", " << L.z << ")";
    return out;
}

//    template std::ostream & operator<<(std::ostream &out, const vec3<int> &L );
//    template std::ostream & operator<<(std::ostream &out, const vec3<long int> &L );
    template std::ostream & operator<<(std::ostream &out, const vec3<float> &L );
    template std::ostream & operator<<(std::ostream &out, const vec3<double> &L );

    
    
template <class T> vec3<T> vec3<T>::Cross(const vec3<T> &in) const {
    const T thex = (*this).y * in.z - (*this).z * in.y;
    const T they = (*this).z * in.x - (*this).x * in.z;
    const T thez = (*this).x * in.y - (*this).y * in.x;
    return vec3<T>( thex, they, thez );
}

    template vec3<float>  vec3<float>::Cross(const vec3<float> &in) const ;
    template vec3<double> vec3<double>::Cross(const vec3<double> &in) const ;

   
template <class T> vec3<T> vec3<T>::Mask(const vec3<T> &in) const {
    const T thex = this->x * in.x;
    const T they = this->y * in.y;
    const T thez = this->z * in.z;
    return vec3<T>( thex, they, thez );
}

    template vec3<float>  vec3<float>::Mask(const vec3<float> &in) const ;
    template vec3<double> vec3<double>::Mask(const vec3<double> &in) const ;

    
template <class T> T vec3<T>::Dot(const vec3<T> &in) const {
    return (*this).x * in.x + (*this).y * in.y + (*this).z * in.z;
}

    template float vec3<float>::Dot(const vec3<float> &in) const;
    template double vec3<double>::Dot(const vec3<double> &in) const;

    
    
template <class T> vec3<T> vec3<T>::unit(void) const {
    const T tot = sqrt(x*x + y*y + z*z);
    return vec3<T>(x/tot, y/tot, z/tot);
} 

    template vec3<float> vec3<float>::unit(void) const;
    template vec3<double> vec3<double>::unit(void) const;

    
    
template <class T> T vec3<T>::length(void) const {
    const T tot = sqrt(x*x + y*y + z*z);
    return tot;
}

    template float vec3<float>::length(void) const;
    template double vec3<double>::length(void) const;

    
    
template <class T>  T vec3<T>::distance(const vec3<T> &rhs) const {
    const T dist = sqrt((x-rhs.x)*(x-rhs.x) + (y-rhs.y)*(y-rhs.y) + (z-rhs.z)*(z-rhs.z));
    return dist;
}

    template float vec3<float>::distance(const vec3<float> &rhs) const;
    template double vec3<double>::distance(const vec3<double> &rhs) const;

    
/*
template <class T>  T vec3<T>::distance(vec3 rhs){
    const T dist = sqrt((x-rhs.x)*(x-rhs.x) + (y-rhs.y)*(y-rhs.y) + (z-rhs.z)*(z-rhs.z));
    return dist;
}
    
template float vec3<float>::distance(vec3<float> rhs);
template double vec3<double>::distance(vec3<double> rhs);
*/
template <class T>  T vec3<T>::sq_dist(const vec3<T> &rhs) const {
    return ((x-rhs.x)*(x-rhs.x) + (y-rhs.y)*(y-rhs.y) + (z-rhs.z)*(z-rhs.z));
}

    template float vec3<float>::sq_dist(const vec3<float> &rhs) const;
    template double vec3<double>::sq_dist(const vec3<double> &rhs) const;



template <class T>  T vec3<T>::angle(const vec3<T> &rhs, bool *OK) const {
    const bool useOK = (OK != nullptr);
    if(useOK) *OK = false;

    const auto Alen = this->length();
    const auto Blen = rhs.length();

    if( !std::isfinite((T)(1)/(Alen))
    ||  !std::isfinite((T)(1)/(Blen))
    ||  !std::isfinite((T)(1)/(Alen*Blen)) ){
        if(useOK) return (T)(-1); //If the user is handling errors.
        FUNCERR("Not possible to compute angle - one of the vectors is too short");
    }

    const auto A = this->unit();
    const auto B = rhs.unit();
    const auto AdotB = A.Dot(B); //Are they pointing along the same direction?

    const auto C = A.Cross(B);
    const auto Clen = C.length(); // == sin(angle);
    const auto principleangle = asin(Clen); // is in range [-pi/2, pi/2].
    const auto absprinangle = svl::Abs(principleangle); // is in range [0, pi/2].

    T theangle;
    if(AdotB > (T)(0)){ //Pointing in same direction: angle < 90 degrees.
        theangle = absprinangle;
    }else{
        theangle = (T)(M_PI) - absprinangle;
    }
    if(useOK) *OK = true;
    return theangle;
}

    template float  vec3<float >::angle(const vec3<float > &rhs, bool *OK) const;
    template double vec3<double>::angle(const vec3<double> &rhs, bool *OK) const;


template <class T>
vec3<T>
vec3<T>::zero(void) const {
    return vec3<T>( static_cast<T>(0),
                    static_cast<T>(0),
                    static_cast<T>(0) );
}

    template vec3<float > vec3<float >::zero(void) const;
    template vec3<double> vec3<double>::zero(void) const;

    
template <class T>
vec3<T>
vec3<T>::rotate_around_x(T angle_rad) const {
    return vec3<T>( this->x,
                    this->y * std::cos(angle_rad) - this->z * std::sin(angle_rad),
                    this->y * std::sin(angle_rad) + this->z * std::cos(angle_rad) );
}

    template vec3<float > vec3<float >::rotate_around_x(float ) const;
    template vec3<double> vec3<double>::rotate_around_x(double) const;


template <class T>
vec3<T>
vec3<T>::rotate_around_y(T angle_rad) const {
    return vec3<T>( this->x * std::cos(angle_rad) + this->z * std::sin(angle_rad),
                    this->y,
                  - this->x * std::sin(angle_rad) + this->z * std::cos(angle_rad) );
}

    template vec3<float > vec3<float >::rotate_around_y(float ) const;
    template vec3<double> vec3<double>::rotate_around_y(double) const;


template <class T>
vec3<T>
vec3<T>::rotate_around_z(T angle_rad) const {
    return vec3<T>( this->x * std::cos(angle_rad) - this->y * std::sin(angle_rad),
                    this->x * std::sin(angle_rad) + this->y * std::cos(angle_rad),
                    this->z );
}

    template vec3<float > vec3<float >::rotate_around_z(float ) const;
    template vec3<double> vec3<double>::rotate_around_z(double) const;


template <class T>
bool 
vec3<T>::GramSchmidt_orthogonalize(vec3<T> &b, vec3<T> &c) const {
    //Using *this as seed, orthogonalize the inputs.
    
    //The first vector is *this.
    const auto UA = *this;
    const auto UB = b - UA * (UA.Dot(b) / UA.Dot(UA));
    const auto UC = c - UA * (UA.Dot(c) / UA.Dot(UA))
                      - UB * (UB.Dot(c) / UB.Dot(UB));
    if(this->isfinite() && b.isfinite() && c.isfinite()){
        b = UB;
        c = UC;
        return true;
    }
    return false;
}

    template bool vec3<float >::GramSchmidt_orthogonalize(vec3<float > &, vec3<float > &) const;
    template bool vec3<double>::GramSchmidt_orthogonalize(vec3<double> &, vec3<double> &) const;



template <class T>  std::string vec3<T>::to_string(void) const {
    std::stringstream out;
    out << *this;
    return out.str();
}

    template std::string vec3<float >::to_string(void) const;
    template std::string vec3<double>::to_string(void) const;



//Sets *this and returns a copy.
template <class T>  vec3<T> vec3<T>::from_string(const std::string &in){
    std::stringstream ss;
    ss << in;
    ss >> *this;
    return *this;
}

    template vec3<float > vec3<float >::from_string(const std::string &in);
    template vec3<double> vec3<double>::from_string(const std::string &in);

    

 
template <class T>    std::istream &operator>>(std::istream &in, vec3<T> &L){
    //Note: This friend is templated (Y) within the templated class (T). We only
    // care about friend template when T==Y.
    //
    //... << "("  << L.x << ", " << L.y << ", " <<  L.z  <<  ")";
    //We have at least TWO options here. We can use a method which is compatible
    // with the ( , , ) notation, or we can ask for straight-up numbers. 
    //We will discriminate here based on what 'in' is.
//    if(&in != &std::cin){                                            //Neat trick, but makes it hard to build on...
        char grbg;
        //... << "("  << L.x << ", " << L.y << ", " <<  L.z  <<  ")";
        in    >> grbg >> L.x >> grbg >> L.y >> grbg >>  L.z  >> grbg;
//    }else  in >> L.x >> L.y >> L.z;
    return in;
}

//    template std::istream & operator>>(std::istream &out, vec3<int> &L );
//    template std::istream & operator>>(std::istream &out, vec3<long int> &L );
    template std::istream & operator>>(std::istream &out, vec3<float> &L );
    template std::istream & operator>>(std::istream &out, vec3<double> &L );

     
    
template <class T>    vec3<T> & vec3<T>::operator=(const vec3<T> &rhs) {
    //Check if it is itself.
    if(this == &rhs) return *this; 
    (*this).x = rhs.x;    (*this).y = rhs.y;    (*this).z = rhs.z;
    return *this;
}

//    template vec3<int> & vec3<int>::operator=(const vec3<int> &rhs);
//    template vec3<long int> & vec3<long int>::operator=(const vec3<long int> &rhs);
    template vec3<float> & vec3<float>::operator=(const vec3<float> &rhs);
    template vec3<double> & vec3<double>::operator=(const vec3<double> &rhs);

    
    
template <class T>    vec3<T> vec3<T>::operator+(const vec3<T> &rhs) const {
    return vec3<T>( (*this).x + rhs.x, (*this).y + rhs.y, (*this).z + rhs.z);
}

//    template vec3<int> vec3<int>::operator+(const vec3<int> &rhs) const;
//    template vec3<long int> vec3<long int>::operator+(const vec3<long int> &rhs) const;
    template vec3<float> vec3<float>::operator+(const vec3<float> &rhs) const;
    template vec3<double> vec3<double>::operator+(const vec3<double> &rhs) const;


    
template <class T>    vec3<T> & vec3<T>::operator+=(const vec3<T> &rhs) {
    (*this).x += rhs.x;    (*this).y += rhs.y;    (*this).z += rhs.z;
    return *this;
}

//    template vec3<int> & vec3<int>::operator+=(const vec3<int> &rhs);
//    template vec3<long int> & vec3<long int>::operator+=(const vec3<long int> &rhs);
    template vec3<float> & vec3<float>::operator+=(const vec3<float> &rhs);
    template vec3<double> & vec3<double>::operator+=(const vec3<double> &rhs);

    
    
template <class T> vec3<T> vec3<T>::operator-(const vec3<T> &rhs) const {
    return vec3<T>( (*this).x - rhs.x, (*this).y - rhs.y, (*this).z - rhs.z);
}

//    template vec3<int> vec3<int>::operator-(const vec3<int> &rhs) const;
//    template vec3<long int> vec3<long int>::operator-(const vec3<long int> &rhs) const;
    template vec3<float> vec3<float>::operator-(const vec3<float> &rhs) const;
    template vec3<double> vec3<double>::operator-(const vec3<double> &rhs) const;


    
template <class T>    vec3<T> & vec3<T>::operator-=(const vec3<T> &rhs) {
    (*this).x -= rhs.x;    (*this).y -= rhs.y;    (*this).z -= rhs.z;
    return *this;
}

//    template vec3<int> & vec3<int>::operator-=(const vec3<int> &rhs);
//    template vec3<long int> & vec3<long int>::operator-=(const vec3<long int> &rhs);
    template vec3<float> & vec3<float>::operator-=(const vec3<float> &rhs);
    template vec3<double> & vec3<double>::operator-=(const vec3<double> &rhs);

    
//------------------------------ overloaded native-types -----------------------------

/*
template <class T>    vec3<T> vec3<T>::operator*(const T rhs) {
    return vec3<T>(x*rhs,y*rhs,z*rhs);
}
template <class T>    vec3<T> & vec3<T>::operator*=(const T rhs) {
    (*this).x *= rhs;    (*this).y *= rhs;    (*this).z *= rhs;
     return *this;
}
template vec3<int> & vec3<int>::operator*=(const int rhs);
template vec3<float> & vec3<float>::operator*=(const float rhs);
template vec3<double> & vec3<double>::operator*=(const double rhs);



template <class T>    vec3<T> vec3<T>::operator/(const T rhs) {
    return vec3<T>(x/rhs,y/rhs,z/rhs);
}
template <class T>    vec3<T> & vec3<T>::operator/=(const T rhs) {
    (*this).x /= rhs;    (*this).y /= rhs;    (*this).z /= rhs;
     return *this;
}
template vec3<int> & vec3<int>::operator/=(const int rhs);
template vec3<float> & vec3<float>::operator/=(const float rhs);
template vec3<double> & vec3<double>::operator/=(const double rhs);
*/

//--------

template <class T>    vec3<T> vec3<T>::operator*(const T &rhs) const {
    return vec3<T>(x*rhs,y*rhs,z*rhs);
}

//    template vec3<int> vec3<int>::operator*(const int &rhs) const;
//    template vec3<long int> vec3<long int>::operator*(const long int &rhs) const;
    template vec3<float> vec3<float>::operator*(const float &rhs) const;
    template vec3<double> vec3<double>::operator*(const double &rhs) const;

    
template <class T>    vec3<T> & vec3<T>::operator*=(const T &rhs) {
    (*this).x *= rhs;    (*this).y *= rhs;    (*this).z *= rhs;
    return *this;
}

//    template vec3<int> & vec3<int>::operator*=(const int &rhs);
//    template vec3<long int> & vec3<long int>::operator*=(const long int &rhs);
    template vec3<float> & vec3<float>::operator*=(const float &rhs);
    template vec3<double> & vec3<double>::operator*=(const double &rhs);

    
    
    
template <class T>    vec3<T> vec3<T>::operator/(const T &rhs) const {
    return vec3<T>(x/rhs,y/rhs,z/rhs);
}

//    template vec3<int> vec3<int>::operator/(const int &rhs) const;
//    template vec3<long int> vec3<long int>::operator/(const long int &rhs) const;
    template vec3<float> vec3<float>::operator/(const float &rhs) const;
    template vec3<double> vec3<double>::operator/(const double &rhs) const;

    
template <class T>    vec3<T> & vec3<T>::operator/=(const T &rhs) {
    (*this).x /= rhs;    (*this).y /= rhs;    (*this).z /= rhs;
    return *this;
}

//    template vec3<int> & vec3<int>::operator/=(const int &rhs);
//    template vec3<long int> & vec3<long int>::operator/=(const long int &rhs);
    template vec3<float> & vec3<float>::operator/=(const float &rhs);
    template vec3<double> & vec3<double>::operator/=(const double &rhs);

    
template <class T>    T & vec3<T>::operator[](size_t i) {
    if(false){
    }else if(i == 0){
        return this->x;
    }else if(i == 1){
        return this->y;
    }else if(i == 2){
        return this->z;
    }
    throw std::invalid_argument("Invalid element access. Cannot continue.");
    return this->x;
}

    template float  & vec3<float >::operator[](size_t);
    template double & vec3<double>::operator[](size_t);

    
    
template <class T>    bool vec3<T>::operator==(const vec3<T> &rhs) const {
    //There are, of course, varying degrees of equality for floating-point values.
    //
    //Typically the safest approach is an exact bit-wise equality using ==. This is
    // the least flexible but most reliable. 
    return ( (x == rhs.x) && (y == rhs.y) && (z == rhs.z) );


    //Define a maximum relative difference threshold. If above this, the numbers
    // are different. If below, they are 'equal' (to the threshold).
    //
    //NOTE: This will be unsuitable for some situations. If you've come here to
    // change this value: DON'T! Instead, define an auxiliary class or routine to
    // handle your special case!
    static const T MAX_REL_DIFF(1E-6);

    //Perfect, bit-wise match. Sometimes even copying will render this useless.
    if((x == rhs.x) && (y == rhs.y) && (z == rhs.z)) return true;
    
    //Relative difference match. Should handle zeros and low-near-zeros (I think). 
    // Macro RELATIVE_DIFF is currently defined in YgorMisc.h (Feb. 2013).
    if(svl::equal(x, rhs.x, MAX_REL_DIFF)
        && svl::equal(y, rhs.y, MAX_REL_DIFF)
            && svl::equal(z, rhs.z, MAX_REL_DIFF)) return true;

    return false;
}

//    template bool vec3<int>::operator==(const vec3<int> &rhs) const;
//    template bool vec3<long int>::operator==(const vec3<long int> &rhs) const;
    template bool vec3<float>::operator==(const vec3<float> &rhs) const;
    template bool vec3<double>::operator==(const vec3<double> &rhs) const;

   
template <class T>    bool vec3<T>::operator!=(const vec3<T> &rhs) const {
    return !( *this == rhs );
}

//    template bool vec3<int>::operator!=(const vec3<int> &rhs) const;
//    template bool vec3<long int>::operator!=(const vec3<long int> &rhs) const;
    template bool vec3<float>::operator!=(const vec3<float> &rhs) const;
    template bool vec3<double>::operator!=(const vec3<double> &rhs) const;

 
    
template <class T>    bool vec3<T>::operator<(const vec3<T> &rhs) const {
    //NOTE: Do *NOT* change this unless there is a weakness in this approach. If you need some
    //      special behaviour, implement for your particular needs elsewhere.
    //
    //NOTE: Do *NOT* rely on any particular implementation! This operator is generally only
    //      useful for in maps.
    //

    //Since we are using floating point numbers, we should check for equality before making a 
    // consensus of less-than.
    if(*this == rhs) return false;

    //Approach A: ordering based on the ordering of individual coordinates. We have no choice 
    // but to make preferred directions, which will not always satisfy the particular meaning of
    // the users' code. One issue is that there are multiple comparisons needed. Benefits include
    // naturally ordering into a logically space-filling order, recovering one-dimensional
    // ordering, no chance of overflow, no truncation errors, and no need for computing sqrt().
    if(this->z != rhs.z) return this->z < rhs.z;  
    if(this->y != rhs.y) return this->y < rhs.y;  
    return this->x < rhs.x;

    //Approach B: ordering based on the length of the vector. This is a means of generating a
    // single number that describes each vector. One issue is that unit vectors are all considered
    // equal, which might be considered illogical. Another is that a sqrt() is required and an
    // overflow can happen.
    //return this->length() < rhs.length();

    //NOTE: Although this is a fairly "unsatisfying" result, it appears to properly allow 
    // vec3's to be placed in std::maps, whereas more intuitive methods (x<rhs.x, etc..) do NOT. 
    // If an actual operator< is to be defined, please do NOT overwrite this one (so that we 
    // can continue to put vec3's into std::map and not have garbled output and weird bugs!) 
    //
    //return ( (y < rhs.y) ); //  <--- BAD! (See previous note above ^)
}

//    template bool vec3<int>::operator<(const vec3<int> &rhs) const;
//    template bool vec3<long int>::operator<(const vec3<long int> &rhs) const;
    template bool vec3<float>::operator<(const vec3<float> &rhs) const;
    template bool vec3<double>::operator<(const vec3<double> &rhs) const;

   
template <class T>    bool vec3<T>::isfinite(void) const {
    return std::isfinite(this->x) && std::isfinite(this->y) && std::isfinite(this->z);
}

    template bool vec3<float >::isfinite(void) const;
    template bool vec3<double>::isfinite(void) const;


#ifdef NOTYET
//This function evolves a pair of position and velocity (x(t=0),v(t=0)) to a pair (x(t=T),v(t=T)) using the
// classical expression for a time- and position-dependent force F(x;t). It is highly unstable, so the number
// of iterations must be specified. If this is going to be used for anything important, make sure that the
// number of iterations is chosen sufficiently high so as to produce negligible errors.
std::tuple<vec3<double>,vec3<double>> Evolve_x_v_over_T_via_F(const std::tuple<vec3<double>,vec3<double>> &x_and_v, 
                                                              std::function<vec3<double>(vec3<double> x, double T)> F,  
                                                              double T, long int steps){
    std::tuple<vec3<double>,vec3<double>> out(x_and_v), last(x_and_v);
    const double m = 1.0;

    if(steps <= 0) FUNCERR("Unable to evolve x and v - the number of steps specified is impossible");
    //if(T <= 0.0) ...   This is OK!
    if(!F) FUNCERR("Given function F is not valid. Unable to do anything");

    const double dt = T/static_cast<double>(steps);

    for(long int i=0; i<steps; ++i){
        const auto curr_t = static_cast<double>(i)*dt;
        const auto F_at_last_x_curr_t = F( std::get<0>(last), curr_t );

        //Update V.
        std::get<1>(out) = F_at_last_x_curr_t*(dt/m) + std::get<1>(last);
 
        //Use current V to update X.
        std::get<0>(out) = std::get<1>(out)*dt + std::get<0>(last);

        //Store the old values.
        std::get<0>(last) = std::get<0>(out);
        std::get<1>(last) = std::get<1>(out);
    }

    return std::move(out);
}

#endif


//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------- vec2: A three-dimensional vector -------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//Constructors.
template <class T>    vec2<T>::vec2(){   x=(T)(0);   y=(T)(0); }

//    template vec2<int>::vec2(void);
//    template vec2<long int>::vec2(void);
    template vec2<float>::vec2(void);
    template vec2<double>::vec2(void);


template <class T>    vec2<T>::vec2(T a, T b) : x(a), y(b) { }

//    template vec2<int>::vec2(int, int);
//    template vec2<long int>::vec2(long int, long int);
    template vec2<float>::vec2(float, float);
    template vec2<double>::vec2(double, double);

    
template <class T>    vec2<T>::vec2( const vec2<T> &in ) : x(in.x), y(in.y) { }

//    template vec2<int>::vec2( const vec2<int> & );
//    template vec2<long int>::vec2( const vec2<long int> & );
    template vec2<float>::vec2( const vec2<float> & );
    template vec2<double>::vec2( const vec2<double> & );

    
    
//More general: (but is it needed?)
//template<class Ch,class Tr,class T>     std::basic_ostream<Ch,Tr> & operator<<( std::basic_ostream<Ch,Tr> &&out, const vec2<T> &L ){
//    out << "(" << L.x << ", " << L.y << ", " << L.z << ")";
//    return out;
//}
template <class T>    std::ostream & operator<<( std::ostream &out, const vec2<T> &L ) {
    //Note: This friend is templated (Y) within the templated class (T). We only
    // care about friend template when T==Y.
    //
    //There is significant whitespace here!
    out << "(" << L.x << ", " << L.y << ")";
    return out;
}

//    template std::ostream & operator<<(std::ostream &out, const vec2<int> &L );
//    template std::ostream & operator<<(std::ostream &out, const vec2<long int> &L );
    template std::ostream & operator<<(std::ostream &out, const vec2<float> &L );
    template std::ostream & operator<<(std::ostream &out, const vec2<double> &L );

    
    
template <class T> T vec2<T>::Dot(const vec2<T> &in) const {
    return (*this).x * in.x + (*this).y * in.y;
}

    template float vec2<float>::Dot(const vec2<float> &in) const;
    template double vec2<double>::Dot(const vec2<double> &in) const;

   
template <class T> vec2<T> vec2<T>::Mask(const vec2<T> &in) const {
    const T thex = this->x * in.x; 
    const T they = this->y * in.y;
    return vec2<T>( thex, they );
}

    template vec2<float > vec2<float >::Mask(const vec2<float > &in) const ;
    template vec2<double> vec2<double>::Mask(const vec2<double> &in) const ;

 
    
template <class T> vec2<T> vec2<T>::unit(void) const {
    const T tot = sqrt(x*x + y*y);
    return vec2<T>(x/tot, y/tot);
} 

    template vec2<float> vec2<float>::unit(void) const;
    template vec2<double> vec2<double>::unit(void) const;

    
    
template <class T> T vec2<T>::length(void) const {
    const T tot = sqrt(x*x + y*y);
    return tot;
}

    template float vec2<float>::length(void) const;
    template double vec2<double>::length(void) const;

    
    
template <class T>  T vec2<T>::distance(const vec2<T> &rhs) const {
    const T dist = sqrt((x-rhs.x)*(x-rhs.x) + (y-rhs.y)*(y-rhs.y));
    return dist;
}

    template float vec2<float>::distance(const vec2<float> &rhs) const;
    template double vec2<double>::distance(const vec2<double> &rhs) const;

   
template <class T>  T vec2<T>::sq_dist(const vec2<T> &rhs) const {
    return ((x-rhs.x)*(x-rhs.x) + (y-rhs.y)*(y-rhs.y));
}

    template float  vec2<float >::sq_dist(const vec2<float > &rhs) const;
    template double vec2<double>::sq_dist(const vec2<double> &rhs) const;


template <class T>
vec2<T>
vec2<T>::zero(void) const {
    return vec2<T>( static_cast<T>(0),
                    static_cast<T>(0) );
}

    template vec2<float > vec2<float >::zero(void) const;
    template vec2<double> vec2<double>::zero(void) const;

    

template <class T>
vec2<T>
vec2<T>::rotate_around_z(T angle_rad) const {
    return vec2<T>( this->x * std::cos(angle_rad) - this->y * std::sin(angle_rad),
                    this->x * std::sin(angle_rad) + this->y * std::cos(angle_rad) );
}

    template vec2<float > vec2<float >::rotate_around_z(float ) const;
    template vec2<double> vec2<double>::rotate_around_z(double) const;


template <class T>  std::string vec2<T>::to_string(void) const {
    std::stringstream out;
    out << *this;
    return out.str();
}

    template std::string vec2<float >::to_string(void) const;
    template std::string vec2<double>::to_string(void) const;


//Sets *this and returns a copy.
template <class T>  vec2<T> vec2<T>::from_string(const std::string &in){
    std::stringstream ss;
    ss << in;
    ss >> *this;
    return *this;
}

    template vec2<float > vec2<float >::from_string(const std::string &in);
    template vec2<double> vec2<double>::from_string(const std::string &in); 


 
template <class T>    std::istream &operator>>( std::istream &in, vec2<T> &L ) {
    //Note: This friend is templated (Y) within the templated class (T). We only
    // care about friend template when T==Y.
    //
    //... << "("  << L.x << ", " << L.y << ", " <<  L.z  <<  ")";
    //We have at least TWO options here. We can use a method which is compatible
    // with the ( , , ) notation, or we can ask for straight-up numbers. 
    //We will discriminate here based on what 'in' is.
//    if(&in != &std::cin){                                   //Neat trick, but makes it hard to build on..
        char grbg;
        //... << "("  << L.x << ", " << L.y <<  ")";
        in    >> grbg >> L.x >> grbg >> L.y >> grbg;
//    }else  in >> L.x >> L.y;
    return in;
}

//    template std::istream & operator>>(std::istream &out, vec2<int> &L );
//    template std::istream & operator>>(std::istream &out, vec2<long int> &L );
    template std::istream & operator>>(std::istream &out, vec2<float> &L );
    template std::istream & operator>>(std::istream &out, vec2<double> &L );

    
    
template <class T>    vec2<T> & vec2<T>::operator=(const vec2<T> &rhs) {
    //Check if it is itself.
    if (this == &rhs) return *this; 
    (*this).x = rhs.x;    (*this).y = rhs.y;
    return *this;
}

//    template vec2<int> & vec2<int>::operator=(const vec2<int> &rhs);
//    template vec2<long int> & vec2<long int>::operator=(const vec2<long int> &rhs);
    template vec2<float> & vec2<float>::operator=(const vec2<float> &rhs);
    template vec2<double> & vec2<double>::operator=(const vec2<double> &rhs);

    
    
template <class T>    vec2<T> vec2<T>::operator+(const vec2<T> &rhs) const {
    return vec2<T>( (*this).x + rhs.x, (*this).y + rhs.y );
}

//    template vec2<int> vec2<int>::operator+(const vec2<int> &rhs) const;
//    template vec2<long int> vec2<long int>::operator+(const vec2<long int> &rhs) const;
    template vec2<float> vec2<float>::operator+(const vec2<float> &rhs) const;
    template vec2<double> vec2<double>::operator+(const vec2<double> &rhs) const;


    
template <class T>    vec2<T> & vec2<T>::operator+=(const vec2<T> &rhs) {
    (*this).x += rhs.x;    (*this).y += rhs.y;
    return *this;
}

//    template vec2<int> & vec2<int>::operator+=(const vec2<int> &rhs);
//    template vec2<long int> & vec2<long int>::operator+=(const vec2<long int> &rhs);
    template vec2<float> & vec2<float>::operator+=(const vec2<float> &rhs);
    template vec2<double> & vec2<double>::operator+=(const vec2<double> &rhs);

    
    
template <class T> vec2<T> vec2<T>::operator-(const vec2<T> &rhs) const {
    return vec2<T>( (*this).x - rhs.x, (*this).y - rhs.y);
}

//    template vec2<int> vec2<int>::operator-(const vec2<int> &rhs) const;
//    template vec2<long int> vec2<long int>::operator-(const vec2<long int> &rhs) const;
    template vec2<float> vec2<float>::operator-(const vec2<float> &rhs) const;
    template vec2<double> vec2<double>::operator-(const vec2<double> &rhs) const;


    
template <class T>    vec2<T> & vec2<T>::operator-=(const vec2<T> &rhs) {
    (*this).x -= rhs.x;    (*this).y -= rhs.y;
    return *this;
}

//    template vec2<int> & vec2<int>::operator-=(const vec2<int> &rhs);
//    template vec2<long int> & vec2<long int>::operator-=(const vec2<long int> &rhs);
    template vec2<float> & vec2<float>::operator-=(const vec2<float> &rhs);
    template vec2<double> & vec2<double>::operator-=(const vec2<double> &rhs);

    
//------------------------------ overloaded native-types -----------------------------


template <class T>    vec2<T> vec2<T>::operator*(const T &rhs) const {
    return vec2<T>(x*rhs,y*rhs);
}

//    template vec2<int> vec2<int>::operator*(const int &rhs) const;
//    template vec2<long int> vec2<long int>::operator*(const long int &rhs) const;
    template vec2<float> vec2<float>::operator*(const float &rhs) const;
    template vec2<double> vec2<double>::operator*(const double &rhs) const;

    
template <class T>    vec2<T> & vec2<T>::operator*=(const T &rhs) {
    (*this).x *= rhs;    (*this).y *= rhs;
    return *this;
}

//    template vec2<int> & vec2<int>::operator*=(const int &rhs);
//    template vec2<long int> & vec2<long int>::operator*=(const long int &rhs);
    template vec2<float> & vec2<float>::operator*=(const float &rhs);
    template vec2<double> & vec2<double>::operator*=(const double &rhs);

    
template <class T>    vec2<T> vec2<T>::operator/(const T &rhs) const {
    return vec2<T>(x/rhs,y/rhs);
}

//    template vec2<int> vec2<int>::operator/(const int &rhs) const;
//    template vec2<long int> vec2<long int>::operator/(const long int &rhs) const;
    template vec2<float> vec2<float>::operator/(const float &rhs) const;
    template vec2<double> vec2<double>::operator/(const double &rhs) const;

    
template <class T>    vec2<T> & vec2<T>::operator/=(const T &rhs) {
    (*this).x /= rhs;    (*this).y /= rhs;
    return *this;
}

//    template vec2<int> & vec2<int>::operator/=(const int &rhs);
//    template vec2<long int> & vec2<long int>::operator/=(const long int &rhs);
    template vec2<float> & vec2<float>::operator/=(const float &rhs);
    template vec2<double> & vec2<double>::operator/=(const double &rhs);

    
    
template <class T>    T & vec2<T>::operator[](size_t i) {
    if(false){
    }else if(i == 0){
        return this->x;
    }else if(i == 1){
        return this->y;
    }
    throw std::invalid_argument("Invalid element access. Cannot continue.");
    return this->x;
}

    template float  & vec2<float >::operator[](size_t);
    template double & vec2<double>::operator[](size_t);

    
    
template <class T>    bool vec2<T>::operator==(const vec2<T> &rhs) const {
    return ( (x == rhs.x) && (y == rhs.y) );
}

//    template bool vec2<int>::operator==(const vec2<int> &rhs) const;
//    template bool vec2<long int>::operator==(const vec2<long int> &rhs) const;
    template bool vec2<float>::operator==(const vec2<float> &rhs) const;
    template bool vec2<double>::operator==(const vec2<double> &rhs) const;

   
template <class T>    bool vec2<T>::operator!=(const vec2<T> &rhs) const {
    return !( *this == rhs );
}

//    template bool vec2<int>::operator!=(const vec2<int> &rhs) const;
//    template bool vec2<long int>::operator!=(const vec2<long int> &rhs) const;
    template bool vec2<float>::operator!=(const vec2<float> &rhs) const;
    template bool vec2<double>::operator!=(const vec2<double> &rhs) const;

 
    
template <class T>    bool vec2<T>::operator<(const vec2<T> &rhs) const {
    //NOTE: Do *NOT* change this unless there is a weakness in this approach. If you need some
    //      special behaviour, implement for your particular needs elsewhere.
    //
    //NOTE: Do *NOT* rely on any particular implementation! This operator is generally only
    //      useful for in maps.
    //

    //Since we are using floating point numbers, we should check for equality before making a 
    // consensus of less-than.
    if(*this == rhs) return false;
    
    //Approach A: ordering based on the ordering of individual coordinates. We have no choice 
    // but to make preferred directions, which will not always satisfy the particular meaning of
    // the users' code. One issue is that there are multiple comparisons needed. Benefits include
    // naturally ordering into a logically space-filling order, recovering one-dimensional
    // ordering, no chance of overflow, no truncation errors, and no need for computing sqrt().
    if(this->y != rhs.y) return this->y < rhs.y;
    return this->x < rhs.x;
    
    //Approach B: ordering based on the length of the vector. This is a means of generating a
    // single number that describes each vector. One issue is that unit vectors are all considered
    // equal, which might be considered illogical. Another is that a sqrt() is required and an
    // overflow can happen.
    //return this->length() < rhs.length();

    //NOTE: Although this is a fairly "unsatisfying" result, it appears to properly allow 
    // vec3's to be placed in std::maps, whereas more intuitive methods (x<rhs.x, etc..) do NOT. 
    // If an actual operator< is to be defined, please do NOT overwrite this one (so that we 
    // can continue to put vec3's into std::map and not have garbled output and weird bugs!) 
    //
    //return ( (y < rhs.y) ); //  <--- BAD! (See previous note above ^)
}

//    template bool vec2<int>::operator<(const vec2<int> &rhs) const;
//    template bool vec2<long int>::operator<(const vec2<long int> &rhs) const;
    template bool vec2<float>::operator<(const vec2<float> &rhs) const;
    template bool vec2<double>::operator<(const vec2<double> &rhs) const;


    
template <class T>    bool vec2<T>::isfinite(void) const {
    return std::isfinite(this->x) && std::isfinite(this->y);
}

    template bool vec2<float >::isfinite(void) const;
    template bool vec2<double>::isfinite(void) const;



//---------------------------------------------------------------------------------------------------------------------------
//----------------------------------------- line: (infinitely-long) lines in 3D space ---------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//Constructors.


//Member functions.

/*
//This function takes a point and a unit vector along each line and computes the point at which they intersect. If they diverge, the function returns false. Otherwise,
// it returns true and places the point of intersection in "output."
//
// If the function returns false, it does not imply that the lines diverge - it implies only that the solution computed with this method was unstable!
//
//This function accepts 3D vectors, but only uses the x and y parts. The z-component is entirely ignored.
template <class T>  bool line<T>::Intersects_With_Line_Once( const line<T> &in, vec3<T> &out) const {

    //------------
    // Speculation on how to extend this result to a fully-3D result:
    //   step 1 - find the plane which intersects with line 1 infinitely-many places and which only intersects in one place for line 2.
    //   step 2 - determine the point in this plane where line 2 intersects.
    //   step 3 - determine if line 1 and this point coincide.
    //                   If they coincide, then the lines coincide. If any difficulties arise or line 1 and the point do not coincide, then the lines do not coincide.
    //------------


    //If the pivot point R_0 of each line is identical, then we say an intersection occurs there.
    if( (*this).R_0 == in.R_0 ){
        if( !((*this).U_0 == in.U_0) ){
            out = in.R_0;
            return true;
        }
        //The lines overlap! We have an infinite number of solutions, so we pretend it is unsolveable.
        FUNCWARN("Attempting to determine intersection point of two identical lines. Pretending they do not intersect!");
        return false;
    }

    //If the two lines are not in the same z-plane, then the following routine is insufficient!
    if( ((*this).R_0.z == in.R_0.z) || ((*this).U_0.z != (T)(0)) || (in.U_0.z != (T)(0)) ){
        FUNCWARN("This function can not handle fully-3D lines. Lines which do not have a constant z-component are not handled. Continuing and indicating that we could not determine the point of intersection");
        return false;
    }

    //We parametrize each line like (R(t) = point + unit*t) and attempt to determine t for each line.
    // From Maxima:
    //   solve([u1x*t1 - u2x*t2 = Cx, u1y*t1 - u2y*t2 = Cy], [t1,t2]);
    //   --->  [[t1=(Cy*u2x-Cx*u2y)/(u1y*u2x-u1x*u2y) , t2=(Cy*u1x-Cx*u1y)/(u1y*u2x-u1x*u2y)]]
    const T denom = ((*this).U_0.y*in.U_0.x - (*this).U_0.x*in.U_0.y);
    if(fabs(denom) < (T)(1E-99)){
        FUNCWARN("Unable to compute the intersection of two lines. Either the lines do not converge, or the tolerances are set too high. Continuing and indicating that we could not determine the point of intersection");
        return false;
    }

    const T Cx       = (in.R_0.x - (*this).R_0.x);
    const T Cy       = (in.R_0.y - (*this).R_0.y);
    const T numer_t1 = (Cy*in.U_0.x      - Cx*in.U_0.y     );
    const T numer_t2 = (Cy*(*this).U_0.x - Cx*(*this).U_0.y);
    const T t1       = numer_t1 / denom;
    const T t2       = numer_t2 / denom;

    //Now we could (should) check if the two t's lead to consistent results. This is not done at the moment because I will surely have nicely-orthogonal lines that will definately intersect nicely.
    //out = vec3<double>( (*this).R_0.x + (*this).U_0.x*t1, (*this).R_0.y + (*this).U_0.y*t1, (*this).R_0.z  );
    out.x = (*this).R_0.x + (*this).U_0.x*t1;
    out.y = (*this).R_0.y + (*this).U_0.y*t1;
    out.z = (*this).R_0.z;
    return true;
}
*/



/*
//This function accepts any line embedded in 3D space.
template <class T>  bool line<T>::Intersects_With_Line_Once( const line<T> &in, vec3<T> &out) const {
    //First, we construct a plane which houses the unit vectors of the two lines.
    // This will give us two planes: $\vec{N} \cdot ( \vec{R} - \vec{R}_{a,0} )$ and $\vec{N} \cdot ( \vec{R} - \vec{R}_{b,0} )$
    // where $\vec{N} = \vec{U}_{a} \otimes \vec{U}_{b}.$ Since the planes are parallel, we just compute the distance between planes. 
    const vec3<T> N( this->U_0.Cross( in.U_0 ) );
    //FUNCINFO("The cross product of the unit vectors " << (*this).U_0 << " and " << in.U_0  << " of the lines is " << N);

FUNCWARN("This functions requires a code review!");

    if(N.length() < (T)(1E-9) ){
        //I might be wrong (very tired right now) but I think this means there are either infinite solutions or none. Either way, we cannot
        // compute them, so we just return a big, fat false.
        return false;
    }

    //The distance between planes can be computed as the distance from a single point on one plane to the other plane. (We know R_0 is on the plane.)
    const plane<T> plane_b( N, in.R_0 );
    const T separation = std::fabs( plane_b.Get_Signed_Distance_To_Point( (*this).R_0 ) ); 
    //FUNCINFO("The separation between planes is " << separation);

    // Explicitly, the signed distance is   dist = (u_a x u_b) dot (R_a - R_b), which removes the need to compute the plane...

    if( separation < (T)(1E-9) ){
        //Determine the point of intersection here.
        // First, we set the two line equations equal to each other in order to determine the parametrization ta and tb where they intersect.
        // Then we dot each (vector) equation with alternatively U_0,a and U_0,b to give us two equations for two unknowns (instead of three
        // equations and two unknowns with a ghost parameter.) The extra piece of information was used during the calculation of plane 
        // separation: ie. we have not lost any info by dotting both sides of the identity to reduce the dimensionality.
        const vec3<T> dR( (*this).R_0 - in.R_0 ); //dR = R_0_a - R_0_b
        const T udotu = (*this).U_0.Dot( in.U_0 );
        const T denom = (T)(1.0) - udotu*udotu;
        if(denom < (T)(1E-9)) return false; //Is this line required, given that we know ua x ub to be nearly zero ?

        //For a line, we only need to compute one of these. For a line segment, we'll need both so we can range-check.
        const T ta = -( ( (*this).U_0 - ( in.U_0 * (udotu) ) ).Dot( dR ) ) / denom;
        //const T tb =  ( ( in.U_0 - ( (*this).U_0 * (udotu) ) ).Dot( dR ) ) / denom; 

        out = (*this).R_0 + ( ( (*this).U_0 ) * ta );
        return true;
    }
    return false;
}
*/


//---------------------------------------------------------------------------------------------------------------------------
//--------------------- samples_1D: a convenient way to collect a sequentially-sampled array of data ------------------------
//---------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------ Constructors -------------------------------------------------------
template <class T> samples_1D<T>::samples_1D(){ }

    template samples_1D<float >::samples_1D(void);
    template samples_1D<double>::samples_1D(void);

//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T>::samples_1D(const samples_1D<T> &in) : samples(in.samples)  {
    this->uncertainties_known_to_be_independent_and_random = in.uncertainties_known_to_be_independent_and_random;
    this->stable_sort();
    this->metadata = in.metadata;
    return;
}

    template samples_1D<float >::samples_1D(const samples_1D<float > &in);
    template samples_1D<double>::samples_1D(const samples_1D<double> &in);

//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T>::samples_1D(const std::list<vec2<T>> &in_samps){ 
    //Providing [x_i, f_i] data. Assumes sigma_x_i and sigma_f_i uncertainties are (T)(0).
    for(auto elem : in_samps) this->push_back(elem.x, elem.y);
    this->stable_sort();
    return;
}

    template samples_1D<float >::samples_1D(const std::list< vec2<float >> &in_points);
    template samples_1D<double>::samples_1D(const std::list< vec2<double>> &in_points);

//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T>::samples_1D(std::vector<std::array<T,4>> in_samps) : samples(std::move(in_samps)) { 
    this->stable_sort();
    return;
}

    template samples_1D<float >::samples_1D(std::vector<std::array<float ,4>> in_samps);
    template samples_1D<double>::samples_1D(std::vector<std::array<double,4>> in_samps);



//---------------------------------------------------- Member Functions -----------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::operator=(const samples_1D<T> &rhs){
    if(this == &rhs) return *this;
    this->uncertainties_known_to_be_independent_and_random = rhs.uncertainties_known_to_be_independent_and_random;
    this->samples = rhs.samples;
    this->metadata = rhs.metadata;
    return *this;
}

    template samples_1D<float > samples_1D<float >::operator=(const samples_1D<float > &rhs);
    template samples_1D<double> samples_1D<double>::operator=(const samples_1D<double> &rhs);

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  void samples_1D<T>::push_back(T x_i, T sigma_x_i, T f_i, T sigma_f_i, bool inhibit_sort){
    this->samples.push_back( {x_i, sigma_x_i, f_i, sigma_f_i} );
    if(!inhibit_sort) this->stable_sort();
    return;
}

    template void samples_1D<float >::push_back(float  x_i, float  sigma_x_i, float  f_i, float  sigma_f_i, bool inhibit_sort);
    template void samples_1D<double>::push_back(double x_i, double sigma_x_i, double f_i, double sigma_f_i, bool inhibit_sort);

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  void samples_1D<T>::push_back(const std::array<T,2> &x_dx, const std::array<T,2> &y_dy, bool inhibit_sort){
    this->samples.push_back( { std::get<0>(x_dx), std::get<1>(x_dx), std::get<0>(y_dy), std::get<1>(y_dy) } );
    if(!inhibit_sort) this->stable_sort();
    return;
}

    template void samples_1D<float >::push_back(const std::array<float ,2> &x_dx, const std::array<float ,2> &y_dy, bool inhibit_sort);
    template void samples_1D<double>::push_back(const std::array<double,2> &x_dx, const std::array<double,2> &y_dy, bool inhibit_sort);

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  void samples_1D<T>::push_back(const std::array<T,4> &samp, bool inhibit_sort){
    this->samples.push_back( samp );
    if(!inhibit_sort) this->stable_sort();
    return;
}

    template void samples_1D<float >::push_back(const std::array<float ,4> &samp, bool inhibit_sort);
    template void samples_1D<double>::push_back(const std::array<double,4> &samp, bool inhibit_sort);

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  void samples_1D<T>::push_back(T x_i, T f_i, bool inhibit_sort){
    //We assume sigma_x_i and sigma_f_i uncertainties are (T)(0).        
    this->samples.push_back( {x_i, (T)(0), f_i, (T)(0)} );
    if(!inhibit_sort) this->stable_sort();
    return;
}

    template void samples_1D<float >::push_back(float  x_i, float  f_i, bool inhibit_sort);
    template void samples_1D<double>::push_back(double x_i, double f_i, bool inhibit_sort);

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  void samples_1D<T>::push_back(const vec2<T> &x_i_and_f_i, bool inhibit_sort){
    //We assume sigma_x_i and sigma_f_i uncertainties are (T)(0).        
    this->samples.push_back( {x_i_and_f_i.x, (T)(0), x_i_and_f_i.y, (T)(0)} );
    if(!inhibit_sort) this->stable_sort();
    return;
}

    template void samples_1D<float >::push_back(const vec2<float > &x_i_and_f_i, bool inhibit_sort);
    template void samples_1D<double>::push_back(const vec2<double> &x_i_and_f_i, bool inhibit_sort);

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  void samples_1D<T>::push_back(T x_i, T f_i, T sigma_f_i, bool inhibit_sort){
    //We assume sigma_x_i is (T)(0).
    this->samples.push_back( {x_i, (T)(0), f_i, sigma_f_i} );
    if(!inhibit_sort) this->stable_sort();
    return;
}

    template void samples_1D<float >::push_back(float  x_i, float  f_i, float  sigma_f_i, bool inhibit_sort);
    template void samples_1D<double>::push_back(double x_i, double f_i, double sigma_f_i, bool inhibit_sort);

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  bool samples_1D<T>::empty() const {
    return this->samples.empty();
}

    template bool samples_1D<float >::empty() const;
    template bool samples_1D<double>::empty() const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  size_t samples_1D<T>::size() const {
    return this->samples.size();
}

    template size_t samples_1D<float >::size() const;
    template size_t samples_1D<double>::size() const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  void samples_1D<T>::stable_sort() {
    //Sorts on x-axis. Lowest-first.
    //
    //This routine is called automatically after this->push_back(...) unless it is surpressed by the user. It can also be 
    // called explicitly by the user, but this is not needed unless they are directly editing this->samples for some reason.
    const auto sort_on_x_i = [](const std::array<T,4> &L, const std::array<T,4> &R) -> bool {
        return L[0] < R[0];
    };
    std::stable_sort(this->samples.begin(), this->samples.end(), sort_on_x_i);
    return;
}

    template void samples_1D<float >::stable_sort();
    template void samples_1D<double>::stable_sort();

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  std::pair<std::array<T,4>,std::array<T,4>> samples_1D<T>::Get_Extreme_Datum_x(void) const {
    //Get the datum with the minimum and maximum x_i. If duplicates are found, there is no rule specifying which.
    if(this->empty()){
        const auto nan = std::numeric_limits<T>::quiet_NaN();
        return std::make_pair<std::array<T,4>,std::array<T,4>>({nan,nan,nan,nan},{nan,nan,nan,nan});
    }

    samples_1D<T> working;
    working = *this;
    working.stable_sort();
    return std::pair<std::array<T,4>,std::array<T,4>>(working.samples.front(),working.samples.back()); 
}

    template std::pair<std::array<float ,4>,std::array<float ,4>> samples_1D<float >::Get_Extreme_Datum_x(void) const;
    template std::pair<std::array<double,4>,std::array<double,4>> samples_1D<double>::Get_Extreme_Datum_x(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  std::pair<std::array<T,4>,std::array<T,4>> samples_1D<T>::Get_Extreme_Datum_y(void) const {
    //Get the datum with the minimum and maximum f_i. If duplicates are found, there is no rule specifying which.
    if(this->empty()){
        const auto nan = std::numeric_limits<T>::quiet_NaN();
        return std::make_pair<std::array<T,4>,std::array<T,4>>({nan,nan,nan,nan},{nan,nan,nan,nan});
    }

    samples_1D<T> working;
    working = this->Swap_x_and_y();
    working.stable_sort();
    const auto flippedF = working.samples.front();
    const auto flippedB = working.samples.back();
    return std::pair<std::array<T,4>,std::array<T,4>>({flippedF[2],flippedF[3],flippedF[0],flippedF[1]},
                          {flippedB[2],flippedB[3],flippedB[0],flippedB[1]});
}

    template std::pair<std::array<float ,4>,std::array<float ,4>> samples_1D<float >::Get_Extreme_Datum_y(void) const;
    template std::pair<std::array<double,4>,std::array<double,4>> samples_1D<double>::Get_Extreme_Datum_y(void) const;



//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Strip_Uncertainties_in_x(void) const {
    //Sets uncertainties to zero. Useful in certain situations, such as computing aggregate std dev's.
    samples_1D<T> out(*this);
    for(auto &P : out.samples) P[1] = (T)(0);
    return std::move(out);
}

    template samples_1D<float > samples_1D<float >::Strip_Uncertainties_in_x(void) const;
    template samples_1D<double> samples_1D<double>::Strip_Uncertainties_in_x(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Strip_Uncertainties_in_y(void) const {
    //Sets uncertainties to zero. Useful in certain situations, such as computing aggregate std dev's.
    samples_1D<T> out(*this);
    for(auto &P : out.samples) P[3] = (T)(0);
    return std::move(out);
}

    template samples_1D<float > samples_1D<float >::Strip_Uncertainties_in_y(void) const;
    template samples_1D<double> samples_1D<double>::Strip_Uncertainties_in_y(void) const;

//---------------------------------------------------------------------------------------------------------------------------
//Ensure there is a single datum with the given x_i, averaging coincident data if necessary.
//
// Note: This routine sorts data along x_i, lowest-first.
//
// Note: Parameter eps need not be small. All parameters must be within eps of every other member of the
//       group in order to be grouped together and averaged. In other words, this routine requires <eps 
//       from the furthest-left datum, NOT <eps from the nearest-left datum. The latter would allow long
//       chains with a coincidence length potentially infinitely long.
//
// Note: Because coincident x_i are averaged (because eps need not be small), under certain circumstances
//       the output may have points separated by <eps. This should only happen when significant information
//       is lost during averaging, so is likely to happen for denormals, very large and very small inputs,
//       and any other numerically unstable situations. It is best in any case not to rely on the output
//       being separated by >=eps. (If you want such control, explicitly resampling/interpolating would be
//       a better idea.)
//
// Note: For uncertainty propagation purposes, datum with the same x_i (within eps) are considered to be
//       estimates of the same quantity. If you do not want this behaviour and you have uncertainties
//       that are known to be independent and random, consider 'faking' non-independence or non-randomness
//       for this call. That way uncertainties will be a simple average of the incoming d_x_i. 
//
template <class T> void samples_1D<T>::Average_Coincident_Data(T eps) {
    this->stable_sort();
    if(this->samples.size() < 2) return;
    auto itA = std::begin(this->samples);
    while(itA != std::end(this->samples)){
        //Advance a second iterator past all coincident samples.
        auto itB = std::next(itA);
        while(itB != std::end(this->samples)){
            if(((*itB)[0]-(*itA)[0]) < eps){
                ++itB;
            }else{
                break;
            }
        }

        //If there were not coincident data, move on to the next datum.
        const auto num = std::distance(itA,itB);
        if(num == 1){
            ++itA;
            continue;
        }

        //Average the elements [itA,itB).
        std::array<T,4> averaged;
        averaged.fill(static_cast<T>(0));
        for(auto it = itA; it != itB; ++it){
            averaged[2] += (*it)[2];
            averaged[0] += (*it)[0]; //Though these are all within eps of A, eps may not be small. 
                                     // Averaging avoids bias at the cost of some loss of precision.

            if(this->uncertainties_known_to_be_independent_and_random){
                averaged[1] += std::pow((*it)[1],static_cast<T>(2.0));
                averaged[3] += std::pow((*it)[3],static_cast<T>(2.0));
            }else{
                averaged[1] += std::abs((*it)[1]);
                averaged[3] += std::abs((*it)[3]);
            }
        }
        if(this->uncertainties_known_to_be_independent_and_random){
            averaged[1] = std::sqrt(averaged[1]);
            averaged[3] = std::sqrt(averaged[3]);
        }
        averaged[0] /= static_cast<T>(num);
        averaged[1] /= static_cast<T>(num);
        averaged[2] /= static_cast<T>(num);
        averaged[3] /= static_cast<T>(num);

        std::swap(*itA,averaged);
        itA = this->samples.erase(std::next(itA),itB);
    } 
    return;
}

    template void samples_1D<float >::Average_Coincident_Data(float  eps);
    template void samples_1D<double>::Average_Coincident_Data(double eps);

//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Rank_x(void) const {
    //Replaces x-values with (~integer) rank. {dup,N}-plicates get an averaged (maybe non-integer) rank. The y-values (f_i 
    // and sigma_f_i) are not affected. The order of the elements will not be altered.
    //
    //NOTE: sigma_x_i uncertainties are all set to zero herein. Having an uncertainty and a rank is non-sensical.
    //
    //NOTE: Rank is 0-based!
    //
    //Note: All neighbouring points which share a common x ("N-plicates") get an averaged rank. This is important for
    //      statistical purposes and could be filtered out afterward if desired.
    samples_1D<T> out(*this);

    //Step 0 - special cases.
    if(out.empty()) return std::move(out);
    if(out.size() == 1){
        out.samples.front()[0] = (T)(0);
        out.samples.front()[1] = (T)(0);
        return std::move(out);
    }

    //Step 1 - cycle through the sorted samples.
    for(auto p_it = out.samples.begin(); p_it != out.samples.end(); ){
        const T X = (*p_it)[0]; //The x-value of interest.
        auto p2_it = p_it;      //Point where N-plicates stop.
        size_t num(1);          //Number of N-plicates.
       
        //Iterate just past the N-plicates. 
        while(((++p2_it) != out.samples.end()) && ((*p2_it)[0] == X)) ++num;
        
        //This rank gives the average of N sequential natural numbers.
        auto rank = static_cast<T>(std::distance(out.samples.begin(),p_it));
        rank += (T)(0.5) * static_cast<T>(num - (size_t)(1)); 
        
        //Set the rank and iterate past all the N-plicates.
        for(auto pp_it = p_it; pp_it != p2_it; ++pp_it){
            (*p_it)[0] = rank;
            (*p_it)[1] = (T)(0);
            ++p_it;
        }
    }
    return std::move(out);
}

    template samples_1D<float > samples_1D<float >::Rank_x(void) const;
    template samples_1D<double> samples_1D<double>::Rank_x(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Rank_y(void) const {
    //Replaces y-values with (~integer) rank. {dup,N}-plicates get an averaged (maybe non-integer) rank. The x-values (x_i 
    // and sigma_x_i) are not affected. The order of the elements *might* be altered due to the use of a computational trick
    // here requiring two stable_sorts. If this is a problem, go ahead and implement a more stable version! (Why was this not
    // done in the first place? This routine was originally needed for a double-ranking scheme which didn't care about
    // stability of the order.)
    //
    //NOTE: sigma_x_i uncertainties are all set to zero herein. Having an uncertainty and a rank is non-sensical.
    //
    //NOTE: Rank is 0-based!
    //
    //Note: All neighbouring points which share a common y ("N-plicates") get an averaged rank. This is important for
    //      statistical purposes and could be filtered out afterward if desired.
    samples_1D<T> out(this->Swap_x_and_y().Rank_x().Swap_x_and_y());
    return std::move(out);
}

    template samples_1D<float > samples_1D<float >::Rank_y(void) const;
    template samples_1D<double> samples_1D<double>::Rank_y(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Swap_x_and_y(void) const {
    //Swaps x_i with f_i and sigma_x_i with sigma_f_i.
    //
    //NOTE: This routine is often used to avoid writing two routines. Just write it, say, for operating on the x-values and
    //      then for the y-values version do:  Swap_x_and_y().<operation on x-values here>.Swap_x_and_y();. It is more costly
    //      in terms of computation, creating extra copies all around, but it is quite convenient for maintenance and 
    //      testing purposes.
    samples_1D<T> out(*this);
    for(auto p_it = out.samples.begin(); p_it != out.samples.end(); ++p_it){
        const auto F = (*p_it);
        (*p_it)[0] = F[2];
        (*p_it)[1] = F[3];
        (*p_it)[2] = F[0];
        (*p_it)[3] = F[1];
    }
    out.stable_sort();
    return std::move(out);
}

    template samples_1D<float > samples_1D<float >::Swap_x_and_y(void) const;
    template samples_1D<double> samples_1D<double>::Swap_x_and_y(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Sum_x(void) const {
    //Computes the sum with proper uncertainty propagation.
    // 
    // NOTE: If there is no data, a sum of zero with infinite uncertainty will be returned.
    //
    if(this->empty()){
        return { (T)(0), std::numeric_limits<T>::infinity() };
    }else if(this->size() == 1){
        return { this->samples.front()[0], this->samples.front()[1] };
    }

    T sum((T)(0));
    T sigma((T)(0));
    for(const auto &P : this->samples){
        sum += P[0];
        if(this->uncertainties_known_to_be_independent_and_random){
            sigma += P[1]*P[1]; 
        }else{
            sigma += std::abs(P[1]);
        }
    }
    if(this->uncertainties_known_to_be_independent_and_random) sigma = std::sqrt(sigma);
    return {sum, sigma};
}

    template std::array<float ,2> samples_1D<float >::Sum_x(void) const;
    template std::array<double,2> samples_1D<double>::Sum_x(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Sum_y(void) const {
    //Computes the sum with proper uncertainty propagation.
    // 
    // NOTE: If there is no data, a sum of zero with infinite uncertainty will be returned.
    //
    if(this->empty()){
        return { (T)(0), std::numeric_limits<T>::infinity() };
    }else if(this->size() == 1){
        return { this->samples.front()[2], this->samples.front()[3] };
    }
    
    T sum((T)(0));
    T sigma((T)(0));
    for(const auto &P : this->samples){
        sum += P[2];
        if(this->uncertainties_known_to_be_independent_and_random){
            sigma += P[3]*P[3]; 
        }else{
            sigma += std::abs(P[3]);
        }
    }   
    if(this->uncertainties_known_to_be_independent_and_random) sigma = std::sqrt(sigma);
    return {sum, sigma};
}

    template std::array<float ,2> samples_1D<float >::Sum_y(void) const;
    template std::array<double,2> samples_1D<double>::Sum_y(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Average_x(void) const {
    //Computes the average and std dev of the *data*. This std dev is not a measure of uncertainty in the mean. Please keep 
    // reading if you are confused.
    // 
    // The "std dev of the data" characterizes the width of a distribution. An infinite number of datum will *not* 
    // necessarily result in a zero std dev of the data. In this case you are essentially wanting an AVERAGE or want to 
    // characterize a distribution of numbers. Each datum could be taken to have uncertainty equal to the std dev of the 
    // data. (See John R. Taylor's "An Introduction to Error Analysis", 2nd Ed. for justification).
    // 
    // The "std dev of the mean" characterizes how certain we are of the mean, as computed from a collection of datum which
    // we *believe* to be independent measurements of the same quantity. In this case you are essentially wanting a MEAN or
    // want to produce a best estimate of some quantity -- but you don't want to characterize the distribution the mean came
    // from. An infinite number of measurements will result in a zero std dev of the mean.
    //
    // Consider two illustrative scenarios:
    // 
    // 1. You measure the height of all the students in a class. You want to know how tall the average student is, so you 
    //    take all the measurements and compute the AVERAGE. The std dev (of the data) describes how much the height varies,
    //    and says something about the distribution the AVERAGE was pulled from. ---> Use this function!
    //
    // 2. You want to precisely measure a single student's height, so you get all the other students to measure his height.
    //    To get the best estimate, you compute the MEAN. The std dev (of the mean) describes how confident you are in the 
    //    MEAN height. More measurements (maybe a million) will give you a precise estimate, so the std dev (of the mean) 
    //    will tend to zero as more data is collected. ---> Use the MEAN function!
    //
    // NOTE: This routine ignores uncertainties. You are looking for a 'weighted-average' if you need to take into account
    //       existing uncertainties in the data.
    //
    // NOTE: This routine uses an unbiased estimate of the std dev (by dividing by N-1 instead of N) and is thus suitable 
    //       for incomplete data sets. Exactly-complete sets (aka population) should use N instead of N-1, although the 
    //       difference is often negligible when ~ N > 10.
    //
    // NOTE: If there is no data, an average of zero with infinite uncertainty will be returned.
    //
    if(this->empty()){
        //FUNCWARN("Cannot compute the average of an empty set");
        return { (T)(0), std::numeric_limits<T>::infinity() };
    }else if(this->size() == 1){
        //FUNCWARN("Computing the average of a single element. The std dev will be infinite");
        return { this->samples.front()[0], std::numeric_limits<T>::infinity() };
    } 
    const T N = static_cast<T>(this->samples.size());
    const T invN = (T)(1)/N;
    const T invNminusone = (T)(1)/(N - (T)(1));

    //First pass: compute the average.
    T x_avg((T)(0));
    for(auto p_it = this->samples.begin(); p_it != this->samples.end(); ++p_it){
        const T x_i = (*p_it)[0];
        x_avg += x_i*invN;
    }

    //Second pass: compute the std dev.
    T stddevdata((T)(0));
    for(auto p_it = this->samples.begin(); p_it != this->samples.end(); ++p_it){
        const T x_i = (*p_it)[0];
        stddevdata += (x_i - x_avg)*(x_i - x_avg)*invNminusone;
    }
    stddevdata = std::sqrt(stddevdata);

    return {x_avg, stddevdata};
}

    template std::array<float ,2> samples_1D<float >::Average_x(void) const;
    template std::array<double,2> samples_1D<double>::Average_x(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Average_y(void) const {
    //See this->Average_x() for info.
    if(this->empty()){ 
        //FUNCWARN("Cannot compute the average of an empty set");
        return { (T)(0), std::numeric_limits<T>::infinity() };
    }else if(this->size() == 1){
        //FUNCWARN("Computing the average of a single element. The std dev will be infinite");
        return { this->samples.front()[2], std::numeric_limits<T>::infinity() };
    }
    const T N = static_cast<T>(this->samples.size());
    const T invN = (T)(1)/N;
    const T invNminusone = (T)(1)/(N - (T)(1));
    
    //First pass: compute the average.
    T y_avg((T)(0));
    for(auto p_it = this->samples.begin(); p_it != this->samples.end(); ++p_it){
        const T y_i = (*p_it)[2];
        y_avg += y_i*invN;
    }

    //Second pass: compute the std dev.
    T stddevdata((T)(0));
    for(auto p_it = this->samples.begin(); p_it != this->samples.end(); ++p_it){
        const T y_i = (*p_it)[2];
        stddevdata += (y_i - y_avg)*(y_i - y_avg)*invNminusone;
    }
    stddevdata = std::sqrt(stddevdata);

    return {y_avg, stddevdata};
}

    template std::array<float ,2> samples_1D<float >::Average_y(void) const;
    template std::array<double,2> samples_1D<double>::Average_y(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Mean_x(void) const {
    //Computes the statistical mean and std dev of the *mean*. This std dev measures the uncertainty of the mean. For the 
    // std dev of the mean to be sensical, the samples must be measurements of the *same quantity*. Please keep reading if 
    // you are confused.
    // 
    // The "std dev of the data" characterizes the width of a distribution. An infinite number of datum will *not* 
    // necessarily result in a zero std dev of the data. In this case you are essentially wanting an AVERAGE or want to 
    // characterize a distribution of numbers. Each datum could be taken to have uncertainty equal to the std dev of the 
    // data. (See John R. Taylor's "An Introduction to Error Analysis", 2nd Ed. for justification).
    // 
    // The "std dev of the mean" characterizes how certain we are of the mean, as computed from a collection of datum which
    // we *believe* to be independent measurements of the same quantity. In this case you are essentially wanting a MEAN or
    // want to produce a best estimate of some quantity -- but you don't want to characterize the distribution the mean came
    // from. An infinite number of measurements will result in a zero std dev of the mean.
    //
    // Consider two illustrative scenarios:
    // 
    // 1. You measure the height of all the students in a class. You want to know how tall the average student is, so you 
    //    take all the measurements and compute the AVERAGE. The std dev (of the data) describes how much the height varies,
    //    and says something about the distribution the AVERAGE was pulled from. ---> Use the AVERAGE function!
    //
    // 2. You want to precisely measure a single student's height, so you get all the other students to measure his height.
    //    To get the best estimate, you compute the MEAN. The std dev (of the mean) describes how confident you are in the 
    //    MEAN height. More measurements (maybe a million) will give you a precise estimate, so the std dev (of the mean) 
    //    will tend to zero as more data is collected. ---> Use this function!
    //
    // NOTE: This routine ignores uncertainties. You are looking for a 'weighted-mean' if you need to take into account
    //       existing uncertainties in the data. Be aware the weighted mean is a slightly different beast!
    //
    // NOTE: This routine uses an unbiased estimate of the std dev (by dividing by N-1 instead of N) and is thus suitable 
    //       for incomplete data sets. Exactly-complete sets (aka population) should use N instead of N-1, although the 
    //       difference is often negligible when ~ N > 10.
    //
    // NOTE: If there is no data, an average of zero with infinite uncertainty will be returned.
    //
    // NOTE: Do you want the MEAN or the MEDIAN? From ("Weighted Median Filters: A Tutorial" by Lin Yin, 1996):
    //                   "[...] the median is the maximum likelihood estimate of the signal level in
    //                    the presence of uncorrelated additive biexponentially distributed noise;
    //                    while, the arithmetic mean is that for Gaussian distributed noise."
    // 
    if(this->empty()) return { (T)(0), std::numeric_limits<T>::infinity() }; 
    const T N = static_cast<T>(this->samples.size());
    const auto avg = this->Average_x();
    const T mean = avg[0];
    const T stddevdata = avg[1];
    const T stddevmean = ( std::isfinite(stddevdata) ? stddevdata/std::sqrt(N) : std::numeric_limits<T>::infinity() ); 
    return { mean, stddevmean };
}

    template std::array<float ,2> samples_1D<float >::Mean_x(void) const;
    template std::array<double,2> samples_1D<double>::Mean_x(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Mean_y(void) const {
    //See this->Mean_x() for info.
    if(this->empty()) return { (T)(0), std::numeric_limits<T>::infinity() };
    const T N = static_cast<T>(this->samples.size());
    const auto avg = this->Average_y();
    const T mean = avg[0];
    const T stddevdata = avg[1];
    const T stddevmean = ( std::isfinite(stddevdata) ? stddevdata/std::sqrt(N) : std::numeric_limits<T>::infinity() );
    return { mean, stddevmean };
}

    template std::array<float ,2> samples_1D<float >::Mean_y(void) const;
    template std::array<double,2> samples_1D<double>::Mean_y(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Weighted_Mean_x(void) const {
    //Computes the weighted mean and std dev of the *weighted data*. For the differences between what the AVERAGE and MEAN
    // routines return, see either the Mean_x()/Mean_y() or Average_x()/Average_y() routines. Hint: this is a true mean,
    // not an average. (The uncertainty relation can be derived using the standard error propagation relation.)
    //
    // The weighting factors used in the calculation are the sigma_x_i uncertainties: w_i for x_i is 1/(sigma_x_i^2).
    //
    // When is this routine appropriate? Let's say you have ten papers that report slightly different estimates of the
    // same physical constant. A weighted mean is a good way to figure out the aggregate estimate in which high-certainty
    // estimates are given more weight.
    //
    // NOTE: This function ought to handle ANY sigma_x_i uncertainties passed in, including zero. Keep in mind that if 
    //       there is data with zero sigma_x_i and there is also data with non-zero sigma_x_i, the non-zero sigma_x_i data
    //       will not contribute to the weighted average. This is because the zero sigma_x_i data are infinitely strongly
    //       weighted. Be aware that zero uncertainties going in will (correctly) yield zero uncertainties going out!
    //
    // NOTE: All infinitely-strongly weighted data are treated as being identically weighted. Be aware that this might not
    //       reflect reality, especially since the weights go like 1/(sigma_x_i^2) and will thus blow up quickly with small
    //       uncertainties!
    // 
    // NOTE: A weighted mean (and std dev of the weighted mean) reduces to the statistical mean (and std dev of the mean) 
    //       iff the weights are unanimously (T)(1). If the weights are all equal but not (T)(1) then the weighted mean will
    //       reduce to the statistical mean, but the std dev of the weighted mean will not reduce to the std dev of the 
    //       mean. It will be skewed.
    //
    // NOTE: If there is no data, an average of zero with infinite uncertainty will be returned.
    //
    // NOTE: This routine is *not* immune to overflow. Keep the numbers nice, or compress them if you must.
    //
    // NOTE: In almost all situations you will want to use this routine, you should declare (and ensure) that the datum
    //       are normally-distributed. If the datum are strongly covariant then it is OK to make no assumptions, but the
    //       idea of binning such data loses much of its sense if the data is not normally distributed. Try avoid such a
    //       situation by carefully evaluating the normality of the bins before relying on this routine.
    //
    if(this->empty()){
        //FUNCWARN("Cannot compute the weighted mean of an empty set");
        return { std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::infinity() };
    }else if(this->size() == 1){
        //FUNCWARN("Computing the weighted mean of a single element. The std dev will be infinite");
        return { this->samples.front()[0], std::numeric_limits<T>::infinity() };
    }

    //Characterize the data. If there are any very low uncertainty datum (~zero) then can later ignore the others.
    // We will also proactively compute some things so that only a single loop is required.
    long int Nfinite = 0; //The number of datum with finite weights.
    long int Ninfinite = 0; //The number of datum with infinite weights.
    T sum_w_fin = (T)(0); //Only finite-weights summed.
    T sum_w_x_fin = (T)(0); //Only finite-weighted datum: sum of w_i*x_i.
    T sum_x_inf = (T)(0); //The sum of x_i for all infinite-weighted datum.
    T sum_sqrt_w_i_fin = (T)(0); //Only finite-weighted datum: sum of sqrt(w_i) for non-normal data.
    for(const auto &P : this->samples){
        const T x_i = P[0];
        const T sigma_x_i = P[1];
        if(!std::isfinite(sigma_x_i)) continue; //Won't count toward mean.

        const T w_i = (T)(1)/(sigma_x_i*sigma_x_i);
        if(!std::isfinite(w_i)){
            ++Ninfinite;
            sum_x_inf += x_i;
        }else{
            ++Nfinite;
            sum_w_fin += w_i;
            sum_w_x_fin += w_i*x_i; 
            sum_sqrt_w_i_fin += (T)(1)/std::abs(sigma_x_i);
        }
    }

    //Verify that at least some data was suitable.
    if((Nfinite + Ninfinite) == 0){
        //FUNCWARN("No data was suitable for weighted mean. All datum had inf uncertainty");
        return { std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::infinity() };
    }

    //If there were infinite-weighted datum, it is easy to proceed. There is legitimately no uncertainty due to the
    // infinitly-strong weighting. And the weights are all equal, so the mean is a regular mean.
    if(Ninfinite != 0){
        const T mean = sum_x_inf / static_cast<T>(Ninfinite);
        return { mean, (T)(0) };
    }

    //Otherwise, we need to calculate uncertainty.
    const T mean = sum_w_x_fin / sum_w_fin;
    T sigma = (T)(0);
    if(this->uncertainties_known_to_be_independent_and_random){
        sigma = (T)(1)/std::sqrt(sum_w_fin);
    }else{
        sigma = sum_sqrt_w_i_fin / sum_w_fin;
    }
    return { mean, sigma };
}

    template std::array<float ,2> samples_1D<float >::Weighted_Mean_x(void) const;
    template std::array<double,2> samples_1D<double>::Weighted_Mean_x(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Weighted_Mean_y(void) const {
    //See this->Weighted_Mean_x() for info.
    const auto copy(this->Swap_x_and_y());
    return copy.Weighted_Mean_x();
}

    template std::array<float ,2> samples_1D<float >::Weighted_Mean_y(void) const;
    template std::array<double,2> samples_1D<double>::Weighted_Mean_y(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Median_y(void) const {
    //Finds or computes the median datum with linear interpolation halfway between datum (i.e., un-weighted mean) if N is odd.
    std::array<T,2> out = { std::numeric_limits<T>::quiet_NaN(),
                            std::numeric_limits<T>::quiet_NaN() };

    if(this->samples.empty()) return out;
    if(this->samples.size() == 1){
        out[0] = this->samples.front()[2];
        out[1] = this->samples.front()[3];
        return out;
    }

    //Sort the y-data.
    samples_1D<T> working(this->Swap_x_and_y());
    working.stable_sort();

    size_t N = working.samples.size();
    size_t M = N/2;

    //Advance to just shy of the halfway datum, or to the left of the central data pair.
    auto it = std::next(working.samples.begin(),M-1);
    if(2*M == N){ //If there is a pair, take the mean.
        const auto L = *it;
        ++it;
        const auto R = *it;

        //Grab the left and right f_i and sigma_f_i. (Remember: we treat them as if they were
        // x_i and sigma_x_i becasue we swapped x and y earlier.)
        const auto L_f_i = L[0];
        const auto L_sigma_f_i = L[1];
        const auto R_f_i = R[0];
        const auto R_sigma_f_i = R[1];

        out[0] = (T)(0.5)*L_f_i + (T)(0.5)*R_f_i;

        //Simple half-way point (i.e., un-weighted mean) uncertainty propagation.
        if(this->uncertainties_known_to_be_independent_and_random){
            out[1] = std::sqrt( std::pow((T)(0.5)*L_sigma_f_i, 2) + std::pow((T)(0.5)*R_sigma_f_i, 2) );
        }else{
            out[1] = (T)(0.5)*std::abs(L_sigma_f_i) + (T)(0.5)*std::abs(R_sigma_f_i);
        }
        return out;
    }
    ++it;
    out[0] = (*it)[0];
    out[1] = (*it)[1];
    return out;
}

    template std::array<float ,2> samples_1D<float >::Median_y(void) const;
    template std::array<double,2> samples_1D<double>::Median_y(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Median_x(void) const {
    //See this->Median_y() for info.
    const auto copy(this->Swap_x_and_y());
    return copy.Median_y();
}

    template std::array<float ,2> samples_1D<float >::Median_x(void) const;
    template std::array<double,2> samples_1D<double>::Median_x(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Select_Those_Within_Inc(T L, T H) const {
    //Selects those with a key within [L,H] (inclusive), leaving the order intact.
    //
    //We just scan from lowest to highest, copying the elements which are inclusively in the range. This obviates the
    // need to sort afterward.
    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
    out.metadata = this->metadata;
    const bool inhibit_sort = true;
    for(const auto &elem : this->samples){
        const T x_i = elem[0];
        if(isininc(L, x_i, H)) out.push_back(elem, inhibit_sort);
    }
    return std::move(out);
}

    template samples_1D<float > samples_1D<float >::Select_Those_Within_Inc(float  L, float  H) const;
    template samples_1D<double> samples_1D<double>::Select_Those_Within_Inc(double L, double H) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  std::array<T,4> samples_1D<T>::Interpolate_Linearly(const T &at_x) const {
    //Interpolation assuming the datum are connected with line segments. Standard error propagation formulae are used to 
    // propagate uncertainties (i.e., the uncertainties are *not* themselves linearly interpolated).
    //
    // Returns [at_x, sigma_at_x (== 0), f, sigma_f] interpolated at 'at_x'. If at_x is not within the range of the samples, 
    // expect {at_x, (T)(0), (T)(0),(T)(0)}.
    //
    // If at_x exactly matches one of the samples, no interpolation is performed. Datum are scanned from lowest to highest, 
    // so interpolation at high values will be more costly than low values.
    //
    // NOTE: The returned sigma_at_x is always (T)(0). Due to propagation of uncertainties, under the assumption that datum
    //       are connected by line segment, the nearby sigma_x_i and sigma_f_i will be converted into an equivalent total
    //       sigma_f. So the sigma_at_x is truly zero. It is returned for consistency.
    //
    // NOTE: While uncertainties in x_i are honoured for uncertainty propagation purposes, they are disregarded when
    //       determining which two neighbouring datum the provided x lies between. If the uncertainties in x_i are larger
    //       than the separation of x_i, then a more sophisticated treatment will probably be necessary. For example, taking
    //       into account the excessive uncertainty of neighbouring x_i might be worthwhile. This condition is NOT inspected
    //       herein, so you should always be aware of how large your uncertainties are.
    //
    //NOTE: If at_x lies exactly on a datum, then we end up over-estimating the uncertainty due to the interpolation
    //      formula simplifying. We do not correct this because exactly on the datum it is difficult to assess what
    //      happens to the sigma_x_i. (Is it somehow converted to sigma_f_i? Is it reported as-is, even though all 
    //      other interpolated points have no sigma_x_i? etc..)
    //

    //Step 0 - Sanity checks.
    if(this->samples.size() < 2) FUNCERR("Unable to interpolate - there are less than 2 samples");

    const std::array<T,4> at_x_as_F { at_x, (T)(0), (T)(0), (T)(0) }; //For easier integration with std::algorithms.

    //Step 1 - Check if the point is within the endpoints. 
    const T lowest_x  = this->samples.front()[0];
    const T highest_x = this->samples.back()[0];
    if(!isininc(lowest_x, at_x, highest_x)){
        //Here will we 'interpolate' to zero. I *think* this is makes the most mathematical sense: the distribution is not 
        // defined outside its range, so it should either be zero or +-infinity. There is only one (mathematical) zero and
        // two infinities. Zero is the unique, sane choice.
        //
        // Note that this causes two problems. It possibly makes the distribution discontinuous, and also possibly makes the
        // uncertainty discontinuous. Be aware of this if continuity is important for your problem.
        return at_x_as_F;
    }

    //Step 2 - Check if the point is bounded by two samples (or if it is sitting exactly on a sample).
    //
    //The following are initialized to 0 to silence warnings.
    T  x0((T)(0)),  x1((T)(0)),  y0((T)(0)),  y1((T)(0));
    T sx0((T)(0)), sx1((T)(0)), sy0((T)(0)), sy1((T)(0)); 
    {
        //We use a binary search routine to find the bounds. First, we look for the right bound and then assume the point
        // just to the left is the left bound. (So this routine should honour the ordering of points with identical x_i.)
        const auto lt_xval = [](const std::array<T,4> &L, const std::array<T,4> &R) -> bool {
            return L[0] < R[0];
        };
        auto itR = std::upper_bound(this->samples.begin(), this->samples.end(), at_x_as_F, lt_xval);
        //Check if at_x is exactly on the highest_x point. If it is, we simplify the rest of the routine by including this
        // point in the preceeding range. Since there is one extra point that is not naturally captured when using ranges
        // like [x_i, x_{i+1}) or (x_i, x_{i+1}] this is a necessary step.
        if(itR == this->samples.end()) itR = std::prev(this->samples.end());
        const auto itL = std::prev(itR);
        x0  = (*itL)[0];  x1  = (*itR)[0];  //x_i to the left of at_x.
        sx0 = (*itL)[1];  sx1 = (*itR)[1];  //uncertainty in x_i to the left of at_x.
        y0  = (*itL)[2];  y1  = (*itR)[2];  //x_i to the right of at_x.
        sy0 = (*itL)[3];  sy1 = (*itR)[3];  //uncertainty in x_i to the right of at_x.
    }

    //Step 2.5 - Check if the points are (effectively, numerically) on top of one another. Bail, if so.
    const T dx = x1 - x0;
    const T invdx = (T)(1)/dx;
    if(!std::isnormal(dx) || !std::isnormal(invdx)){ //Catches dx=0, 0=1/dx, inf=1/dx cases.
        FUNCERR("Cannot interpolate between two (computationally-identical) points. Try removing them by "
                "averaging f_i and adding sigma_x_i and sigma_f_i in quadrature. Cannot continue");
        //In this case, trying to assume that (at_x-x0) is half that of (x1-x0) and other schemes to re-use the linear eqn
        // will fail. Why? We ultimately need a derivative, and the derivative is zero here (unless f_i are the same too).
        // The only reasonable solution is to deal with these problems PRIOR to calling this function; we cannot do anything
        // because this is a const member!
    }

    //Step 3 - Given a (distinct) lower and an upper point, we perform the linear interpolation.
    const T f_at_x = y0 + (y1 - y0)*(at_x - x0)*invdx;

    //Step 4 - Work out the uncertainty. These require partial derivatives of f_at_x (== F) thus we have dF/dy0, etc..
    const T dFdy0 = (x1 - at_x)*invdx;
    const T dFdy1 = (at_x - x0)*invdx;
    const T dFdx0 = ((at_x - x1)*(y1 - y0))*invdx*invdx;
    const T dFdx1 = ((x0 - at_x)*(y1 - y0))*invdx*invdx;

    T sigma_f_a_x = (T)(0);
    if(this->uncertainties_known_to_be_independent_and_random){
        sigma_f_a_x = std::sqrt( (dFdy0 * sy0)*(dFdy0 * sy0) + (dFdy1 * sy1)*(dFdy1 * sy1)
                               + (dFdx0 * sx0)*(dFdx0 * sx0) + (dFdx1 * sx1)*(dFdx1 * sx1) );
    }else{
        sigma_f_a_x = ( std::abs(dFdy0 * sy0) + std::abs(dFdy1 * sy1)
                      + std::abs(dFdx0 * sx0) + std::abs(dFdx1 * sx1) );
    }
    return { at_x, (T)(0), f_at_x, sigma_f_a_x };
}

    template std::array<float ,4> samples_1D<float >::Interpolate_Linearly(const float  &at_x) const;
    template std::array<double,4> samples_1D<double>::Interpolate_Linearly(const double &at_x) const;



//---------------------------------------------------------------------------------------------------------------------------
#if 0
template <class T>
samples_1D<T> 
samples_1D<T>::Peaks(void) const {
    //Returns the locations linearly-interpolated peaks.
    //
    // Note: This routine requires input to sorted.
    //
    // Note: The crossings are returned with an estimate of the uncertainty and f_at_x (i.e., the height of the peak).
    //       
    const auto N = this->samples.size();
    if(N < 3) throw std::invalid_argument("Unable to identify peaks with < 3 datum");

    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
    const bool InhibitSort = true;

    //Compute the first and second derivatives.
    auto deriv_1st = this->Derivative_Centered_Finite_Differences();
    auto deriv_2nd = deriv_1st.Derivative_Centered_Finite_Differences();

    //Locate zero-crossings of the first derivative.
    auto zero_crossings = deriv_1st.Crossings(static_cast<T>(0));

    //For each, evaluate the inflection. If negative, add the peak x and f_at_x to the outgoing collection.
    for(const auto &p : zero_crossings.samples){
        const auto x = p[0];
        const auto fpp = deriv_2nd.Interpolate_Linearly(x);
        if(fpp[2] <= static_cast<T>(0)){
            out.push_back( this->Interpolate_Linearly(x), InhibitSort );
        }
    }
    return out;
}

    template samples_1D<float > samples_1D<float >::Peaks(void) const;
    template samples_1D<double> samples_1D<double>::Peaks(void) const;
#endif

//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Derivative_Centered_Finite_Differences(void) const {
    //Calculates the discrete derivative using centered finite differences. (The endpoints use forward and backward
    // finite differences to handle boundaries.) Data should be reasonably smooth -- no interpolation is used.
    //
    // NOTE: The data needs to be relatively smooth for this routine to be useful. No interpolation or smoothing
    //       is performed herein. If such is needed, you should do it before calling this routine.
    //
    // NOTE: If the spacing between datum is not too large, the centered finite difference will probably be a
    //       better estimate of the derivative because it is not forward or backward biased.
    //
    // NOTE: At the moment, this routine cannot handle when adjacents points' dx is not finite. In the future,
    //       these points might be averaged -- though the intolerance behaviour is not unreasonable. Do not
    //       count on either behaviour. (At the time of writing, the derivative at such point will simply be
    //       infinite or NaN, depending on the implementation.)
    //
    // NOTE: AT THE MOMENT, UNCERTAINTIES sigma_x_i and sigma_f_i ARE IGNORED! This should be rectified!
    //       Ideally, they should propagate standard operations errors AND the inherent finite-difference
    //       errors (of order O(dx)) PLUS any estimated local sampling noise (if possible).
    
#pragma message "Warning - finite difference routine are lacking uncertainty-handling!"
    
    const auto samps_size = static_cast<long int>(this->samples.size());
    if(samps_size < 2) FUNCERR("Cannot compute derivative with so few points. Cannot continue");
    const bool InhibitSort = true;
    samples_1D<T> out = *this;
    
    for(long int i = 0; i < samps_size; ++i){
        //Handle left boundary: use forward finite differences.
        if(i == 0){
            //const auto L = this->samples[i-1];
            const auto C = this->samples[i];
            const auto R = this->samples[i+1];
            const auto new_x_i = C[0];
            const auto new_sigma_x_i = C[1]; // <---- need something better here!
            const auto new_f_i = (R[2] - C[2])/(R[0] - C[0]);
            const auto new_sigma_f_i = C[3]; // <---- need something better here!
            
            out.samples[i] = { new_x_i, new_sigma_x_i, new_f_i, new_sigma_f_i };
            
            //Handle right boundary: use backward finite differences.
        }else if(i == (samps_size - 1)){
            const auto L = this->samples[i-1];
            const auto C = this->samples[i];
            //const auto R = this->samples[i+1];
            const auto new_x_i = C[0];
            const auto new_sigma_x_i = C[1]; // <---- need something better here!
            const auto new_f_i = (C[2] - L[2])/(C[0] - L[0]);
            const auto new_sigma_f_i = C[3]; // <---- need something better here!
            
            out.samples[i] = { new_x_i, new_sigma_x_i, new_f_i, new_sigma_f_i };
            
            //Else, use centered finite differences.
        }else{
            const auto L = this->samples[i-1];
            const auto C = this->samples[i];
            const auto R = this->samples[i+1];
            const auto new_x_i = C[0];
            const auto new_sigma_x_i = C[1]; // <---- need something better here!
            const auto new_f_i = (R[2] - L[2])/(R[0] - L[0]);
            const auto new_sigma_f_i = C[3]; // <---- need something better here!
            
            out.samples[i] = { new_x_i, new_sigma_x_i, new_f_i, new_sigma_f_i };
            
        }
    }
    return std::move(out);
};

template samples_1D<float > samples_1D<float >::Derivative_Centered_Finite_Differences(void) const;
template samples_1D<double> samples_1D<double>::Derivative_Centered_Finite_Differences(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> 
samples_1D<T> 
samples_1D<T>::Resample_Equal_Spacing(size_t N) const {
    //Resamples the data into approximately equally-spaced samples using linear interpolation.
    if(N < 2) throw std::invalid_argument("Unable to resample < 2 datum with equal spacing");

    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
    const bool InhibitSort = true;

    //Get the endpoints.
    auto extrema = this->Get_Extreme_Datum_x();
    const T xmin = extrema.first[0];
    const T xmax = extrema.second[0];
    const T dx = (xmax - xmin) / static_cast<T>(N-1);
    for(size_t i = 0; i < N; ++i){
        out.push_back( this->Interpolate_Linearly( xmin + dx * static_cast<T>(i) ), InhibitSort );
    }
    return out;
}

    template samples_1D<float > samples_1D<float >::Resample_Equal_Spacing(size_t) const;
    template samples_1D<double> samples_1D<double>::Resample_Equal_Spacing(size_t) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> 
T 
samples_1D<T>::Find_Otsu_Binarization_Threshold(void) const {
    //Binarize (threshold) the samples along x using Otsu's criteria.
    //
    // Otsu thresholding works best with well-defined bimodal histograms.
    // It works by finding the threshold that partitions the voxel intensity histogram into two parts,
    // essentially so that the sum of each partition's variance is minimal.
    //
    // Note that *this is treated as a histogram. All uncertainties are ignored.
    //
    // Note that this routine will not actually binarize anything. It only produces a threshold for the user
    // to use for binarization. In general, this routine will be used to analyze a histogram produced from some
    // non-1D data source (e.g., image pixel intensities). So binarization should be applied in the originating
    // domain.
    if(this->samples.size() < 2) throw std::runtime_error("Unable to binarize with <2 datum");

    const auto zero = static_cast<T>(0);
    const auto one  = static_cast<T>(1);

    // Sum the total bin magnitude. Note that for later consistencty we use a potentially lossy summation.
    // This is intentional, since worse numerical woes may result later if we use inconsistent approaches.
    // This situation could be rectified by using rolling estimators for moments, means, and variances. TODO.
    const auto total_bin_magnitude = [&](void) -> T {
        T out = zero;
        for(const auto &s : this->samples) out += s[2];
        return out;
    }();

    const T total_moment = [&](void) -> T {
        T moment = zero;
        T i = zero;
        for(const auto &s : this->samples){
            moment += (s[2] * i);
            i += one;
        }
        return moment;
    }();

    T running_low_moment = zero;
    T running_magnitude_low = zero;

    T max_variance = -one;
    size_t threshold_index = 0;

    size_t i = 0;
    for(const auto &s : this->samples){
        running_magnitude_low += s[2];

        // If no bins with any height have yet been seen, skip them.
        if(running_magnitude_low == zero){
            ++i;
            continue;
        }

        const auto running_magnitude_high = total_bin_magnitude - running_magnitude_low;

        // If we've reached the end, or numerical losses will cause issues, then bail.
        if(running_magnitude_high <= zero) break;

        running_low_moment += (s[2] * static_cast<T>(i));
        const auto mean_low = running_low_moment / running_magnitude_low;
        const auto mean_high = (total_moment - running_low_moment) / running_magnitude_high;

        // If numerical issues cause negative or non-finite means (which should always be >= 0), then bail.
        if( !std::isfinite(mean_low)
        ||  !std::isfinite(mean_high)
        ||  (mean_low < zero)
        ||  (mean_high < zero) ){
            break;
        }

        // Test if the current threshold's variance is maximal.
        const auto current_variance = running_magnitude_low
                                    * running_magnitude_high
                                    * std::pow(mean_high - mean_low, 2.0);
        if(max_variance < current_variance){
            max_variance = current_variance;
            threshold_index = i;
        }
        ++i;
    }

    if(max_variance < zero){
        throw std::logic_error("Unable to perform Otsu thresholding; no suitable thresholds were identified");
    }

    const auto f_threshold = this->samples.at(threshold_index)[0];
    return f_threshold;
}

    template float  samples_1D<float >::Find_Otsu_Binarization_Threshold(void) const;
    template double samples_1D<double>::Find_Otsu_Binarization_Threshold(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Multiply_With(T factor) const {
    //Multiply all sample f_i's by a given factor. Uncertainties are appropriately scaled too.
    samples_1D<T> out;
    out = *this;
    const T abs_factor = std::abs(factor);
    for(auto &elem : out.samples){
        elem[2] *= factor;
        elem[3] *= abs_factor; //Same for either type of uncertainty.
    }
    return std::move(out);
}

    template samples_1D<float > samples_1D<float >::Multiply_With(float  factor) const;
    template samples_1D<double> samples_1D<double>::Multiply_With(double factor) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Sum_With(T factor) const {
    //Add the given factor to all sample f_is. Uncertainties are unchanged (since the given factor
    // implicitly has no uncertainty).
    samples_1D<T> out;
    out = *this;
    for(auto &elem : out.samples){
        elem[2] += factor;
    }
    return std::move(out);
}

    template samples_1D<float > samples_1D<float >::Sum_With(float  factor) const;
    template samples_1D<double> samples_1D<double>::Sum_With(double factor) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Apply_Abs(void) const {
    //Apply an absolute value functor to all f_i.
    samples_1D<T> out;
    out = *this;
    for(auto &elem : out.samples) elem[2] = std::abs(elem[2]);
    return std::move(out);
}

    template samples_1D<float > samples_1D<float >::Apply_Abs(void) const;
    template samples_1D<double> samples_1D<double>::Apply_Abs(void) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Sum_x_With(T dx) const {
    //Shift all x_i's by a factor. No change to uncertainties.
    samples_1D<T> out;
    out = *this;
    for(auto &sample : out.samples) sample[0] += dx;
    return std::move(out);
}

    template samples_1D<float > samples_1D<float >::Sum_x_With(float  dx) const;
    template samples_1D<double> samples_1D<double>::Sum_x_With(double dx) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Multiply_x_With(T x) const {
    //Multiply all x_i's by a factor. No change to uncertainties.
    samples_1D<T> out;
    out = *this;
    for(auto &sample : out.samples) sample[0] *= x;
    return std::move(out);
}

    template samples_1D<float > samples_1D<float >::Multiply_x_With(float  x) const;
    template samples_1D<double> samples_1D<double>::Multiply_x_With(double x) const;


///---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Sum_With(const samples_1D<T> &g) const {
    //This routine sums two samples_1D by assuming they are connected by line segments. Because linear interpolation is used,
    // the error propagation may deal with sigma_x_i uncertainty in an odd way; don't be surprised if the vertical 
    // uncertainties grow larger and the horizontal uncertainties grow smaller, or if both grow larger!
    //
    // NOTE: If the samples share exactly the same x_i in the same order, this routine should only require a single pass
    //       through the datum of each. If one has a subset of the other's x_i with no holes, it should also only require
    //       a single pass. I think all other combinations will require linear interpolation which could involve sweeping
    //       over all datum in either samples_1D (unless some sort of index has been built in after writing). Be aware of
    //       potentially high computation cost of combining large or numerous distinct-x_i samples_1D. 
    //
    // NOTE: Metadata from both inputs are injected into the new contour, but items from *this have priority in case there
    //       is a collision.
    //
    // NOTE: Uncertainty bars may abruptly grow large or shrink at the range perhiphery of one samples_1D. For example, if 
    //       summing two samples_1D with similar f_i and sigmas, the uncertainty will naturally jump at the edges.
    //
    //   F:                              |               |
    //                                   |               |
    //                               |   X               X  |                        
    //                               |   |               |  |                                                    
    //   ----------------------------X---|---------------|--X--------------------------------------------------> x
    //                               |                      |                                                    
    //                               |                      |                                                    
    //                           
    //                                                                  |                                       
    //   G:                     |            |           |              |            |                           
    //          |      |        |            |           |              X            |        |                   
    //      |   |      |        X            X           X              |            X        |       |          
    //      |   X      X        |            |           |              |            |        X       |           
    //   ---X---|------|--------|------------|-----------|---------------------------|--------|-------X--------> x
    //      |   |      |                                                                      |       |
    //      |                                                                                         |          
    //                           
    //                                 (region of reduced              _ ____
    //                       ___ _       uncertainties)            __-- |    ----____                           
    //  F+G:            __---   | -_    _ ___ ____   ____       _--     |            |                           
    //          |      |        |   -  / |   |    ---    |\  _--        X            |        |                   
    //      |   |      |        X    |/  X   X           X \|           |            X        |       |          
    //      |   X      X        |    |   |   |           |  |           |            |        X       |           
    //   ---X---|------|--------|----X----------------------X------------------------|--------|-------X--------> x
    //      |   |      |             |                      |                                 |       |
    //      |                                                                                         |          
    //                           
    //                             /|______________________/|___ Jump in uncertainties here.
    // 
    //   Because F's domain is much shorter than G's domain, the uncertainties will jump discontinuously at the boundary
    //   of F. This is natural, and is due to the discontinuity of F's uncertaintites over the domain of G. 
    //
    //   One way around this would be to SMOOTHLY TAPER the edges of F so the discontinuity is not so abrupt. How best to
    //   smooth requires interpretation of the data, but something as simple as a linear line should work. Use f_i = 0 and 
    //   sigma_f_i = 0 just to the right and just to the left of the domain.
    //

    //Step 0 - Special cases. If either *this or g are empty (i.e., zero everywhere), simply return the other.
    if(g.empty()) return *this;
    if(this->empty()) return g;

    //We can assume that the resultant samples_1D is normal, random, and independent only if both the input are too.
    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = ( this->uncertainties_known_to_be_independent_and_random
                                                          && g.uncertainties_known_to_be_independent_and_random ); 
    out.metadata = this->metadata;
    out.metadata.insert(g.metadata.begin(), g.metadata.end()); //Will not overwrite existing elements from this->metadata.

    //These are used to try avoid (costly) interpolations.
    const T f_lowest_x  = this->samples.front()[0];
    const T f_highest_x = this->samples.back()[0];
    const T g_lowest_x  = g.samples.front()[0];
    const T g_highest_x = g.samples.back()[0];

    //Get the part on the left where only one set of samples lies.
    auto f_it = this->samples.begin(); 
    auto g_it = g.samples.begin();
    const bool inhibit_sort = true; // We scan from lowest to highest and thus never need to sort.

    while( true ){
        const bool fvalid = (f_it != this->samples.end());  
        const bool gvalid = (g_it != g.samples.end());

        if(fvalid && !gvalid){
            out.push_back(*f_it, inhibit_sort);
            ++f_it;
            continue;
        }else if(gvalid && !fvalid){
            out.push_back(*g_it, inhibit_sort);
            ++g_it;
            continue;
        }else if(fvalid && gvalid){
            const T F_x_i       = (*f_it)[0];
            const T F_sigma_x_i = (*f_it)[1];
            const T F_f_i       = (*f_it)[2];
            const T F_sigma_f_i = (*f_it)[3];
            const T G_x_i       = (*g_it)[0];
            const T G_sigma_x_i = (*g_it)[1];
            const T G_f_i       = (*g_it)[2];
            const T G_sigma_f_i = (*g_it)[3];

            //The lowest x_i gets pushed into the summation.
            if( F_x_i == G_x_i ){
                const T x_i = F_x_i; // == G_x_i == 0.5*(F_x_i + G_x_i).
                const T f_i = F_f_i + G_f_i;
                T sigma_x_i, sigma_f_i;

                if(out.uncertainties_known_to_be_independent_and_random){
                    //I originally treated this as an average, but this doesn't make sense. If G_sigma_x_i is zero,
                    // then we end up with 0.5*F_sigma_i for the new point. Rather, I *believe* we should end up
                    // with 1.0*F_sigma_i. This seems more intuitively correct and is in any case safer, so I 
                    // decided to just drop the 0.5* factor.
                    //
                    // I'm not so sure that I even can treat it as an average. The x_i line up, but they are not
                    // considered measurements of the same thing. They just happen to have the same expected mean...
                    //
                    //sigma_x_i = 0.5*std::sqrt( F_sigma_x_i*F_sigma_x_i + G_sigma_x_i*G_sigma_x_i );
                    sigma_x_i = std::sqrt( F_sigma_x_i*F_sigma_x_i + G_sigma_x_i*G_sigma_x_i );
                    sigma_f_i = std::sqrt( F_sigma_f_i*F_sigma_f_i + G_sigma_f_i*G_sigma_f_i );
                }else{
                    //sigma_x_i = 0.5*(std::abs(F_sigma_x_i) + std::abs(G_sigma_x_i));
                    sigma_x_i = (std::abs(F_sigma_x_i) + std::abs(G_sigma_x_i));
                    sigma_f_i = std::abs(F_sigma_f_i) + std::abs(G_sigma_f_i);
                }
                out.push_back(x_i, sigma_x_i, f_i, sigma_f_i, inhibit_sort);
                ++f_it;
                ++g_it;
                continue;
            }else if( F_x_i < G_x_i ){
                //Check if we can avoid a costly interpolation.
                if(!isininc(g_lowest_x, F_x_i, g_highest_x)){
                    out.push_back(*f_it, inhibit_sort);
                }else{
                    const auto GG = g.Interpolate_Linearly(F_x_i);
                    const T GG_x_i       = GG[0];
                    //const T GG_sigma_x_i = GG[1]; //Should always be zero.
                    const T GG_f_i       = GG[2];
                    const T GG_sigma_f_i = GG[3];
                    
                    const T x_i = F_x_i;
                    const T f_i = F_f_i + GG_f_i;
                    const T sigma_x_i = F_sigma_x_i;
                    T sigma_f_i;

                    if(out.uncertainties_known_to_be_independent_and_random){
                        sigma_f_i = std::sqrt(F_sigma_f_i*F_sigma_f_i + GG_sigma_f_i*GG_sigma_f_i);
                    }else{
                        sigma_f_i = std::abs(F_sigma_f_i) + std::abs(GG_sigma_f_i);
                    }
                    out.push_back(x_i, sigma_x_i, f_i, sigma_f_i, inhibit_sort);
                }
                ++f_it;
                continue;
            }else{
                //Check if we can avoid a costly interpolation.
                if(!isininc(f_lowest_x, G_x_i, f_highest_x)){
                    out.push_back(*g_it, inhibit_sort);
                }else{
                    const auto FF = this->Interpolate_Linearly(G_x_i);
                    const T FF_x_i       = FF[0];
                    //const T FF_sigma_x_i = FF[1]; //Should always be zero.
                    const T FF_f_i       = FF[2];
                    const T FF_sigma_f_i = FF[3];
                    
                    const T x_i = G_x_i;
                    const T f_i = G_f_i + FF_f_i;
                    const T sigma_x_i = G_sigma_x_i;
                    T sigma_f_i;
                    
                    if(out.uncertainties_known_to_be_independent_and_random){
                        sigma_f_i = std::sqrt(G_sigma_f_i*G_sigma_f_i + FF_sigma_f_i*FF_sigma_f_i);
                    }else{
                        sigma_f_i = std::abs(G_sigma_f_i) + std::abs(FF_sigma_f_i);
                    }
                    out.push_back(x_i, sigma_x_i, f_i, sigma_f_i, inhibit_sort);
                }
                ++g_it;
                continue;
            }
        }else if(!fvalid && !gvalid){
            break; //Done.
        }
    }
    return out;
}

    template samples_1D<float > samples_1D<float >::Sum_With(const samples_1D<float > &in) const;
    template samples_1D<double> samples_1D<double>::Sum_With(const samples_1D<double> &in) const;


//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Subtract(const samples_1D<T> &B) const {
    return this->Sum_With( B.Multiply_With( (T)(-1) ) );
}

    template samples_1D<float > samples_1D<float >::Subtract(const samples_1D<float > &in) const;
    template samples_1D<double> samples_1D<double>::Subtract(const samples_1D<double> &in) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Purge_Nonfinite_Samples(void) const {
    //Using the "erase and remove" idiom.
    auto out = *this;
    out.samples.erase( std::remove_if( out.samples.begin(), out.samples.end(), 
                                       [](const std::array<T,4> &s) -> bool { return !std::isfinite(s[2]); } ), 
                       out.samples.end() );
    return out;
}

    template samples_1D<float > samples_1D<float >::Purge_Nonfinite_Samples(void) const;
    template samples_1D<double> samples_1D<double>::Purge_Nonfinite_Samples(void) const;



//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Aggregate_Equal_Sized_Bins_Weighted_Mean(long int N, bool explicitbins) const {
    //This routine groups a samples_1D into equal sized bins, finding the WEIGHTED MEAN of their f_i to produce a single 
    // datum with: x_i at the bin's mid-x-point, sigma_x_i=0, f_i= WEIGHTED MEAN of bin data, and sigma_f_i= std dev of
    // the weighted mean. This routine might be useful as a preparatory step for linear regressing noisy data, but this is
    // perhaps not statistically sound (the sigma_x_i are completely ignored). This routine is suitable for eliminating
    // datum which share an x_i.
    //
    // The datum in the output are WEIGHTED MEANS of the original data f_i and sigma_f_i destined for that bin. All data in
    // a bin are weighted the same along the x-coordinate, regardless of sigma_x_i or x_i. (Why? Because the bins have a 
    // definite x-position and width, so there is no way to reasonably propagate sigma_x_i or x_i).
    //
    // To recap: f_i and sigma_f_i are used to compute a WEIGHTED MEAN, but x_i and sigma_x_i are essentially IGNORED.
    //
    // NOTE: If you do not want to take into account sigma_f_i uncertainties, either strip them or set them all equal.
    //       (Say, to (T)(1) so that they have the same weight). If some datum have zero sigma_f_i uncertainty and others
    //       have non-zero sigma_f_i, the latter will be disregarded due to the infinitely-strong weighting of zero-
    //       sigma_f_i uncertainty.
    //
    // NOTE: If a bin contains no data, the bin f_i and sigma_f_i will be { (T)(0), (T)(0) }.
    //
    // NOTE: Take care to ensure your data is appropriate for binning. Generally: the smaller the bin, the more statistical
    //       sense binning makes.
    //
    // NOTE: In almost all situations you will want to use this routine, you should declare (and ensure) that the datum
    //       are normally-distributed. If the datum are strongly covariant then it is OK to make no assumptions, but the
    //       idea of binning such data loses much of its sense if the data is not normally distributed. Try avoid such a
    //       situation by carefully evaluating the normality of the bins before relying on this routine.
    //
    // NOTE: To ensure the bins are always sorted in the proper order (which is tough to guarantee, even with a stable
    //       sort, because of floating point errors in determining the bin widths), explicit bins are not exactly the 
    //       true bin width. They are slightly narrower at the base (98% of the bin width) and taper further at the top 
    //       (96% of the bin width). This is cosmetic, and is common for histograms. So if you are computing, say, overlap
    //       between histograms (which you shouldn't be doing with explicit bins...) expect a small discrepancy.
    //       
 
    //Step 0 - Sanity checks.
    if(!isininc((long int)(1),N,(long int)(this->size()))){
         FUNCERR("Asking to aggregate data into a nonsensical number of bins");
    }

    //Get the min/max x-value and bin spacing.
    const auto xmin  = this->samples.front()[0];
    const auto xmax  = this->samples.back()[0];
    const auto spanx = xmax - xmin; //Span of lowest to highest.
    const auto dx    = spanx/static_cast<T>(N);
    if(!std::isnormal(dx)){
        FUNCERR("Aggregating could not proceed: encountered problem with bin width");
    }

    //Cycle through the data, collecting the data for each bin.
    std::vector<std::vector<std::array<T,4>>> binnedraw(N);
    for(const auto &P : this->samples){
        //Determine which bin this datum should go in. We scale the datum's x-coordinate to the span of all coordinates.
        // Then, we multiply by the bin number to get the 'block' of x-coords it belongs to. Then we floor to an int and
        // handle the special upper point.
        //
        // The upper bin is the only both-endpoint-inclusive bin. (This is not a problem, because one of the bins *must*
        // be doubly-inclusive. Why? Because for N bins there are N+1 bin delimiters, and N+1 isn't divisible by N for
        // a general N.)
        const T x_i = P[0];
        const T distfl = std::abs(x_i - xmin); //Distance from lowest.
        const T sdistfl = distfl/spanx; //Scaled from [0,1].
        const T TbinN = std::floor(static_cast<T>(N)*sdistfl); //Bin number as type T.
        const size_t binN = static_cast<size_t>(TbinN);
        const size_t truebinN = (binN >= (size_t)(N)) ? (size_t)(N)-(size_t)(1) : binN;
        binnedraw[truebinN].push_back(P);
    }

    //Now the raw data is ready for aggregating. Cycle through each bin, performing the reduction.
    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
    out.metadata = this->metadata;
    const bool inhibit_sort = true;
    for(long int i = 0; i < N; ++i){
        const T binhw  = dx*(T)(0.48); //Bin half-width -- used to keep bins ordered upon sorting.
        const T basedx = dx*(T)(0.01); //dx from true bin L and R -- used to keep bins ordered upon sorting.
        const T xmid   = xmin + dx*(static_cast<T>(i) + (T)(0.5)); //Middle of the bin.
        const T xbtm   = xmid - binhw;
        const T xtop   = xmid + binhw;

        //Compute the weighted mean of the data. (This routine will handle any sigma_f_i uncertainties properly.)
        // If no data are present, just report the Weight_Mean_y() fallback (probably a mean of {(T)(0),inf}.)
        samples_1D<T> abin(binnedraw[i]);
        abin.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
        abin.metadata = this->metadata; //Needed? Most likely not, but better to be safe here.
        const std::array<T,2> binwmy = abin.Weighted_Mean_y();      

        if(!explicitbins){
            //If the user wants a pure resample, omit the bins.
            out.push_back({xmid, (T)(0), binwmy[0], binwmy[1] }, inhibit_sort);

        }else{ 
            //Otherwise, if displaying on screen (or similar) we explicitly show the bin edges.
            const T xbtm_base = xbtm - basedx;  //Just a tad wider than the top of the bin.
            const T xtop_base = xtop + basedx;
            out.push_back({xbtm_base, (T)(0),    (T)(0),    (T)(0)}, inhibit_sort);
            out.push_back({xbtm,      (T)(0), binwmy[0],    (T)(0)}, inhibit_sort);
            out.push_back({xmid,      (T)(0), binwmy[0], binwmy[1]}, inhibit_sort); //Center of bin, w/ sigma_f_i intact.
            out.push_back({xtop,      (T)(0), binwmy[0],    (T)(0)}, inhibit_sort);
            out.push_back({xtop_base, (T)(0),    (T)(0),    (T)(0)}, inhibit_sort);
        }
    }
    return std::move(out);
}

    template samples_1D<float > samples_1D<float >::Aggregate_Equal_Sized_Bins_Weighted_Mean(long int N, bool showbins) const;
    template samples_1D<double> samples_1D<double>::Aggregate_Equal_Sized_Bins_Weighted_Mean(long int N, bool showbins) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Aggregate_Equal_Datum_Bins_Weighted_Mean(long int N) const {
    //This routine groups a samples_1D into bins of an equal number (N) of datum. The bin x_i and f_i are generated from 
    // the WEIGHTED MEANS of the bin data. This routine propagates sigma_x_i and sigma_f_i. They are propagated separately,
    // meaning that sigma_x_i and sigma_f_i have no influence on one another (just as x_i and f_i have no influence on each
    // other for the purposes of computing the WEIGHTED MEAN).
    //
    // I believe this routine to be more statistically sound than the ...Equal_Sized_Bins() variant because the x_i and
    // sigma_x_i are taken into account herein. Prefer to use this routine, especially on data with equal x_i separation
    // where this routine essentially has no downside compared to the ...Equal_Sized_Bins() variant.
    //
    // To recap: f_i and sigma_f_i are used to compute a WEIGHTED MEAN, and x_i and sigma_x_i are used to compute a
    // WEIGHTED MEAN, but the two have no influence on one another.
    //
    // NOTE: If you do not want to take into account sigma_f_i or sigma_x_i uncertainties, either strip them or set them
    //       all equal. (Say, to (T)(1) so that they have the same weight). If some datum have zero uncertainty while 
    //       others have non-zero uncertainty, the latter will be disregarded due to the infinitely-strong weighting of
    //       zero-uncertainty datum.
    //
    // NOTE: Take care to ensure your data is appropriate for binning. Generally: the smaller the bin, the more statistical
    //       sense binning makes.
    //
    // NOTE: All bins will contain at least a single point, but the bins obviously cannot always all be composed of the
    //       same number of datum. Therefore the bin with the largest x may have less datum than the rest. If you are 
    //       worried about this, try find a divisor before calling this routine.
    //
    // NOTE: If your data has lots of datum with identical x_i (i.e., at least 2*N of them) then it is possible that two
    //       outgoing bins will also exactly overlap. Be aware of this. This routine will run fine, but it can cause issues
    //       in other routines. The solution is to remove all datum with identical x_i prior to calling this routine.
    //
    // NOTE: In almost all situations you will want to use this routine, you should declare (and ensure) that the datum
    //       are normally-distributed. If the datum are strongly covariant then it is OK to make no assumptions, but the
    //       idea of binning such data loses much of its sense if the data is not normally distributed. Try avoid such a
    //       situation by carefully evaluating the normality of the bins before relying on this routine.
    //
 
    //Step 0 - Sanity checks.
    if(N < 1){
         FUNCERR("Asking to aggregate data with a nonsensical number of datum in each bin");
    }

    //Cycle through the data, collecting the data for each bin.
    std::vector<std::vector<std::array<T,4>>> binnedraw;
    std::vector<std::array<T,4>> shtl;
    for(const auto &P : this->samples){
        //Add element to the shuttle.
        shtl.push_back(P);

        //Check if the requisite number of datum has been collected for a complete bin.
        if(N == (long int)(shtl.size())){
            //If so, then a bin has been harvested. Dump the shuttle into the raw data bins and prep for next iteration.
            binnedraw.push_back(std::move(shtl));
            shtl = std::vector<std::array<T,4>>();
        }
    }
    if(!shtl.empty()) binnedraw.push_back(std::move(shtl)); //Final, possibly incomplete bin.

    //Now the raw data is ready for aggregating. Cycle through each bin, performing the reduction.
    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
    out.metadata = this->metadata;
    const bool inhibit_sort = true;
    for(auto const &rawbin : binnedraw){
        samples_1D<T> abin(rawbin);
        abin.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
        abin.metadata = this->metadata; //Probably not needed, but better to be safe here.
 
        //Compute the weighted mean of the data. (This routine will handle any uncertainties properly.)
        // If no data are present, just report the Weight_Mean_y() fallback (probably a mean of {(T)(0),inf}.)
        const std::array<T,2> binwmx = abin.Weighted_Mean_x();      
        const std::array<T,2> binwmy = abin.Weighted_Mean_y();

        //If the user wants a pure resample, omit the bins.
        out.push_back({ binwmx[0], binwmx[1], binwmy[0], binwmy[1] }, inhibit_sort);
    }
    out.stable_sort(); //In case the weighted mean somehow screws up the order.
    return std::move(out);
}

    template samples_1D<float > samples_1D<float >::Aggregate_Equal_Datum_Bins_Weighted_Mean(long int N) const;
    template samples_1D<double> samples_1D<double>::Aggregate_Equal_Datum_Bins_Weighted_Mean(long int N) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Histogram_Equal_Sized_Bins(long int N, bool explicitbins) const {
    //This routine groups a samples_1D into equal sized bins, finding the SUM of their f_i to produce a single datum with
    // x_i at the bin's mid-x-point, sigma_x_i=0, f_i= SUM of bin data, and sigma_f_i= proper propagated uncertainty for 
    // the data sigma_f_i. This routine is useful for plotting a distribution of numbers, possibly with a sigma_f_i
    // weighting. 
    //
    // Often the sigma_f_i will be set to (T)(1) and then the height of all bins will be divided by the total number of
    // datum. This will generate a 'proper' histogram; each bin's height will be the fraction (# of bin datum)/(total # of
    // datum). Such a histogram can be used, for example, to inspect the normality of a distribution of numbers. (In this 
    // case the bin sigma_f_i have no meaning. But if each datum had a proper uncertainty then the bins would correctly
    // propagate the uncertainty due to summation over sigma_f_i in each bin.)
    //
    // NOTE: If a bin contains no data, the bin f_i and sigma_f_i will be { (T)(0), (T)(0) }.
    //
    // NOTE: To ensure the bins are always sorted in the proper order (which is tough to guarantee, even with a stable
    //       sort, because of floating point errors in determining the bin widths), explicit bins are not exactly the 
    //       true bin width. They are slightly narrower at the base (98% of the bin width) and taper further at the top 
    //       (96% of the bin width). This is cosmetic, and is common for histograms. So if you are computing, say, overlap
    //       between histograms (which you shouldn't be doing with explicit bins...) expect a small discrepancy.
    //       
 
    //Step 0 - Sanity checks.
    if(!isininc((long int)(1),N,(long int)(this->size()))){
         FUNCERR("Asking to histogram data into a nonsensical number of bins");
    }

    //Get the min/max x-value and bin spacing.
    const auto xmin  = this->samples.front()[0];
    const auto xmax  = this->samples.back()[0];
    const auto spanx = xmax - xmin; //Span of lowest to highest.
    const auto dx    = spanx/static_cast<T>(N);
    if(!std::isnormal(dx)){
        FUNCERR("Histogram generation could not proceed: encountered problem with bin width");
    }

    //Step 1 - Cycle through the data, collecting the data for each bin.
    std::vector<std::vector<std::array<T,4>>> binnedraw(N);
    for(const auto &P : this->samples){
        //Determine which bin this datum should go in. We scale the datum's x-coordinate to the span of all coordinates.
        // Then, we multiply by the bin number to get the 'block' of x-coords it belongs to. Then we floor to an int and
        // handle the special upper point.
        //
        // The upper bin is the only both-endpoint-inclusive bin. (This is not a problem, because one of the bins *must*
        // be doubly-inclusive. Why? Because for N bins there are N+1 bin delimiters, and N+1 isn't divisible by N for
        // a general N.)
        const T x_i = P[0];
        const T distfl = std::abs(x_i - xmin); //Distance from lowest.
        const T sdistfl = distfl/spanx; //Scaled from [0,1].
        const T TbinN = std::floor(static_cast<T>(N)*sdistfl); //Bin number as type T.
        const size_t binN = static_cast<size_t>(TbinN);
        const size_t truebinN = (binN >= (size_t)(N)) ? (size_t)(N)-(size_t)(1) : binN;
        binnedraw[truebinN].push_back(P);
    }

    //Step 2 - Now the raw data is ready to histogram. Cycle through each bin, performing the reduction.
    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
    out.metadata = this->metadata;
    const bool inhibit_sort = true;
    for(long int i = 0; i < N; ++i){
        const T binhw  = dx*(T)(0.48); //Bin half-width -- used to keep bins ordered upon sorting.
        const T basedx = dx*(T)(0.01); //dx from true bin L and R -- used to keep bins ordered upon sorting.
        const T xmid   = xmin + dx*(static_cast<T>(i) + (T)(0.5)); //Middle of the bin.
        const T xbtm   = xmid - binhw;
        const T xtop   = xmid + binhw;

        //Compute the sum of the data. (This routine will handle any sigma_f_i uncertainties properly.)
        // If no data are present, just report the Sum_y() fallback (probably a mean of {(T)(0),inf}.)
        samples_1D<T> abin(binnedraw[i]);
        abin.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
        abin.metadata = this->metadata; //Probably not needed, but better to be safe here.
        const std::array<T,2> binsumy = abin.Sum_y();

        if(!explicitbins){
            //If the user wants a pure resample, omit the bins.
            out.push_back({xmid, (T)(0), binsumy[0],  binsumy[1] }, inhibit_sort);

        }else{ 
            //Otherwise, if displaying on screen (or similar) we explicitly show the bin edges.
            const T xbtm_base = xbtm - basedx;  //Just a tad wider than the top of the bin.
            const T xtop_base = xtop + basedx;
            out.push_back({xbtm_base, (T)(0),     (T)(0),     (T)(0)}, inhibit_sort);
            out.push_back({xbtm,      (T)(0), binsumy[0],     (T)(0)}, inhibit_sort);
            out.push_back({xmid,      (T)(0), binsumy[0], binsumy[1]}, inhibit_sort); //Center of bin, w/ sigma_f_i intact.
            out.push_back({xtop,      (T)(0), binsumy[0],     (T)(0)}, inhibit_sort);
            out.push_back({xtop_base, (T)(0),     (T)(0),     (T)(0)}, inhibit_sort);
        }
    }
    return std::move(out);
}

    template samples_1D<float > samples_1D<float >::Histogram_Equal_Sized_Bins(long int N, bool showbins) const;
    template samples_1D<double> samples_1D<double>::Histogram_Equal_Sized_Bins(long int N, bool showbins) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,4> samples_1D<T>::Spearmans_Rank_Correlation_Coefficient(bool *OK) const {
    //Computes a correlation coefficient suitable for judging correlation (without any underlying model!) when the data is 
    // monotonically increasing or decreasing. See:
    //   http://en.wikipedia.org/wiki/Spearman's_rank_correlation_coefficient#Definition_and_calculation
    // for more info. This routine (and Spearman's Rank Correlation Coefficient in general) IGNORES all uncertainties.
    //
    // Indeed, it would be difficult to ascribe an uncertainty to the result because ranking is used. I suspect the most
    // reliable approach would use Monte Carlo for a bootstrap-like estimation.
    //
    // Four numbers are returned:
    //     1. The coefficient itself (rho),
    //     2. The number of samples used (as a type T, not an integer),
    //     3. The z-value,
    //     4. The t-value.
    //
    // NOTE: To compute the P-value from the t-value, use DOF = N-2 where N is given as the second return value.
    //
    // NOTE: Don't rely on the z- or t-values computed here if N < 10. 
    //       From http://vassarstats.net/corr_rank.html:
    //       "Note that t is not a good approximation of the sampling distribution of rho when n is less than 10.
    //       For values of n less than 10, you should use the following table of critical values of rs, which shows 
    //       the values of + or —rs required for significance at the .05 level, for both a directional and a 
    //       non-directional test."
    //
    if(OK != nullptr) *OK = false;
    const auto ret_on_err = std::array<T,4>({(T)(-1),(T)(-1),(T)(-1),(T)(-1)}); 

    const samples_1D<T> ranked(this->Rank_x().Rank_y());
    const auto avgx = ranked.Mean_x()[0]; //Don't care about the uncertainty.
    const auto avgy = ranked.Mean_y()[0];

    T numer((T)(0)), denomA((T)(0)), denomB((T)(0));
    for(const auto &P : ranked.samples){
        numer  += (P[0] - avgx)*(P[2] - avgy);
        denomA += (P[0] - avgx)*(P[0] - avgx);
        denomB += (P[2] - avgy)*(P[2] - avgy);
    }
    const auto rho = numer/std::sqrt(denomA*denomB);
    if(!std::isfinite(rho) || (svl::Abs(rho) > (T)(1))){
        if(OK == nullptr) FUNCERR("Found coefficient which was impossible or nan. Bailing");
        FUNCWARN("Found coefficient which was impossible or nan. Bailing");
        return ret_on_err;
    }
    const auto num = static_cast<T>(ranked.samples.size());

    if(ranked.size() < 3){
        if(OK == nullptr) FUNCERR("Unable to compute z-value - too little data");
        FUNCWARN("Unable to compute z-value - too little data");
        return ret_on_err;
    }
    const T zval = std::sqrt((num - 3.0)/1.060)*std::atanh(rho);

    if(ranked.size() < 2){
        if(OK == nullptr) FUNCERR("Unable to compute t-value - too little data");
        FUNCWARN("Unable to compute t-value - too little data");
        return ret_on_err;
    }
    const T tval_n = rho*std::sqrt(num - (T)(2));
    const T tval_d = std::sqrt((T)(1) - rho*rho);
    const T tval   = std::isfinite((T)(1)/tval_d) ? tval_n/tval_d : std::numeric_limits<T>::infinity();

    if(OK != nullptr) *OK = true;
    return {rho, num, zval, tval};
}

    template std::array<float ,4> samples_1D<float >::Spearmans_Rank_Correlation_Coefficient(bool *OK) const;
    template std::array<double,4> samples_1D<double>::Spearmans_Rank_Correlation_Coefficient(bool *OK) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,3> samples_1D<T>::Compute_Sxy_Sxx_Syy(bool *OK) const {
    //Computes {Sxy, Sxx, Syy} which are used for linear regression and other procedures.
    if(OK != nullptr) *OK = false;
    const auto ret_on_err = std::array<T,3>({(T)(-1),(T)(-1),(T)(-1)});

    //Ensure the data is suitable.
    if(this->size() < 1){
        if(OK == nullptr) FUNCERR("Unable to calculate Sxy,Sxx,Syy with no data");
        FUNCWARN("Unable to calculate Sxy,Sxx,Syy with no data. Bailing");
        return ret_on_err;
    }

    const auto mean_x = this->Mean_x()[0]; //Disregard uncertainties.
    const auto mean_y = this->Mean_y()[0]; 
    T Sxx((T)(0)), Sxy((T)(0)), Syy((T)(0));
    for(const auto &P : this->samples){
        Sxy += (P[0] - mean_x)*(P[2] - mean_y);
        Sxx += (P[0] - mean_x)*(P[0] - mean_x);
        Syy += (P[2] - mean_y)*(P[2] - mean_y);
    }
    if(OK != nullptr) *OK = true;
    return {Sxy,Sxx,Syy};
}

    template std::array<float ,3> samples_1D<float >::Compute_Sxy_Sxx_Syy(bool *OK) const;
    template std::array<double,3> samples_1D<double>::Compute_Sxy_Sxx_Syy(bool *OK) const;



#ifdef NOTYET

//---------------------------------------------------------------------------------------------------------------------------
template <class T> lin_reg_results<T> samples_1D<T>::Linear_Least_Squares_Regression(bool *OK, bool SkipExtras) const {
    //Performs a standard linear (y=m*x+b) regression. This routine ignores all provided uncertainties, and instead reports
    // the calculated (perceived?) uncertainties from the data. Thus, this routine ASSUMES all sigma_y_i are the same, and
    // ASSUMES all sigma_x_i are ZERO. Obviously these are not robust assumptions, so it is preferrable to use WEIGHTED
    // REGRESSION if uncertainties are known. 
    //
    // An example where this routine would NOT be appropriate: you take some measurements and then 'linearize' your data by
    // taking the logarithm of all y_i. Unless your data all have very similar y_i, you will very certainly break the 
    // assumption that all sigma_f_i are equal. (Weighted regression *is* suitable for such a situation.)
    //
    // Returns: {[0]slope, [1]sigma_slope, [2]intercept, [3]sigma_intercept, [4]the # of datum used, [5]std dev of 
    // the data ("sigma_f"), [6]sum-of-squared residuals, [7]covariance, [8]linear corr. coeff. (r, aka "Pearson's),
    // [9]t-value for r, [10]two-tailed P-value for r}. 
    //
    // If you only want the slope and intercept, pass in SkipExtras = true. This should speed up the computation. But
    // conversely, do not rely on ANYTHING being present other than slope and intercept if you set SkipExtras = true.
    //
    // Some important comments on the output parameters:
    //
    // - The slope and intercept are defined as [y = slope*x + intercept]. 
    //
    // - The std dev of the data is reported -- this is the sigma_f_i for all datum under the assumption that all 
    //   sigma_f_i are equal. 
    //
    // - The covariance is useful for computing uncertainties while performing interpolation using the resultant parameters.
    //   Neglecting the covariance can lead to substantial under- or over-prediction of uncertainty! (If in doubt, check 
    //   the standard uncertainty propagation formulae: it includes a covariance term I otherwise neglect. Performing the 
    //   uncertainty propagation is straightforward so you have no excuse to forgo it.)
    //
    // - The sum-of-squared residuals is the metric for fitting. It should be at a minimum for the optimal fit and is often
    //   reported for non-linear fits. It can be used to compare various types of fits (e.g., linear vs. nonlinear).
    //
    // - The linear correlation coefficient (aka "Pearson's", aka "r") describes how well the data fits a linear functional
    //   form. Numerical Recipes suggests using Spearman's Rank Correlation Coefficient (Rs) or Kendell's Tau coefficient 
    //   instead, as they take into account characteristics of the x_i and f_i distribution that Pearson's r does not.
    //
    // - The t-value and number of datum used can be converted to a probability (P-value) as either a one- or two-tailed 
    //   distribution. The Degrees of Freedom (DOF) to use is [# of datum used - 2] to account for the number of free 
    //   parameters. (It might be better to simply use the returned P-value, I don't know at the moment.)
    //
    // - The two-tailed P-value describes the probability that the data (N measurements of two uncorrelated variables; the 
    //   ordinate and abscissa) would give a *larger* |r| than what we reported. So if N=20 and |r|=0.5 then P=0.025 and the
    //   linear correlation is significant (assuming your threshold is P=0.05). Thus this value can be used to inspect 
    //   whether the fit line is a 'good fit'. Numerical recipes advocates a Student's t-test while John R. Taylor gives an
    //   (more likely to be) exact integral for computing p-values which will necessitate a numerical integration or 
    //   evaluation of a hypergeometric function. (See John R. Taylor's "An Introduction to Error Analysis", 2nd Ed., 
    //   Appendix C.) We have gone the Student's t-test route for speed and simplicity, but a routine to compute Taylor's
    //   way is provided in YgorStats. A cursory comparison showed strong agreement in reasonable circumstances. It would
    //   be prudent to ensure the computations agree if relying on the p-values.
    //
    // NOTE: This routine is often used to get first-order guess of fit parameters for more sophisticated routines (i.e.,
    //       weighted linear regression). This is an established (possibly even statistically legitimate) strategy.
    //
    // NOTE: We could go on to compute lots of additional things with what this routine spits out:
    //       - P-values corresponding to the t-value and DOF=#datum_used-2 (either one- or two-tailed),
    //       - Coefficient of determination = r*r = r^2,
    //       - Standard error of r = sqrt((1-r*r)/DOF), (<--- Note this is not the same as the std dev!)
    //       - (I think) a z-value,
    //       - If all sigma_x_i are equal, you can transform it into a sigma_f_i uncertainty by rolling it into the
    //         reported std dev of the data ("sigma_f"). This gives you an equivalent total sigma_f using either:
    //         1. [std::hypot(sigma_f, slope*sigma_x);] (if normal, random, independent uncertainties), or
    //         2. [std::abs(sigma_f) + std::abs(slope*sigma_x);] otherwise.
    //       - The uncertainty in sigma_y as the estimate of the true width of the distribution. Double check this, 
    //         but I believe it is merely 1/sqrt(2*(N-2)). (Please double check! Search "fractional uncertainty in sigma".)
    //       And on and on. 
    //

    if(OK != nullptr) *OK = false;
    const lin_reg_results<T> ret_on_err;
    lin_reg_results<T> res;

    //Ensure the data is suitable.
    if(this->size() < 3){
        //If we have two points, there are as many parameters as datum. We cannot even compute the stats.
        // While it is possible to do it for 2 data points, the closed form solution in that case is 
        // easy enough to do in a couple lines, and probably should just be implemented as needed.
        if(OK == nullptr) FUNCERR("Unable to perform meaningful linear regression with so few points");
        FUNCWARN("Unable to perform meaningful linear regression with so few points. Bailing");
        return ret_on_err;
    }
    res.N = static_cast<T>(this->size());
    res.dof = res.N - (T)(2);

    //Cycle through the data, accumulating the basic ingredients for later.
/*
    res.sum_x  = (T)(0);
    res.sum_f  = (T)(0);
    res.sum_xx = (T)(0);
    res.sum_xf = (T)(0);
    for(const auto &P : this->samples){
        res.sum_x  += P[0];
        res.sum_xx += P[0]*P[0];
        res.sum_f  += P[2];
        res.sum_xf += P[0]*P[2];
    }
*/
    {   //Accumulate the data before summing, so we can be more careful about summing the numbers.
        std::vector<T> data_x, data_f, data_xx, data_xf;
        for(const auto &P : this->samples){
            data_x.push_back(  P[0]      );
            data_f.push_back(  P[2]      );
            data_xx.push_back( P[0]*P[0] );
            data_xf.push_back( P[0]*P[2] );
        }
        res.sum_x  = svl::Sum(data_x);
        res.sum_f  = svl::Sum(data_f);
        res.sum_xx = svl::Sum(data_xx);
        res.sum_xf = svl::Sum(data_xf);
    }


    //Compute the fit parameters.
    const T common_denom = std::abs(res.N*res.sum_xx - res.sum_x*res.sum_x);
    if(!std::isnormal(common_denom)){
        //This cannot be zero, inf, or nan. Proceeding with a sub-normal is also a bad idea.
        if(OK == nullptr) FUNCERR("Encountered difficulties with data. Is the data pathological?");
        FUNCWARN("Encountered difficulties with data. Is the data pathological? Bailing");
        return ret_on_err;
    }

    res.slope = (res.N*res.sum_xf - res.sum_x*res.sum_f)/common_denom;
    res.intercept = (res.sum_xx*res.sum_f - res.sum_x*res.sum_xf)/common_denom;
    if(!std::isfinite(res.slope) || !std::isfinite(res.intercept)){
        //While we *could* proceed, something must be off. Maybe an overflow (and inf or nan) during accumulation?
        if(OK == nullptr) FUNCERR("Encountered difficulties computing m and b. Is the data pathological?");
        FUNCWARN("Encountered difficulties computing m and b. Is the data pathological? Bailing");
        return ret_on_err;
    }

    if(SkipExtras){
        if(OK != nullptr) *OK = true;
        return std::move(res);
    }

    //Now compute the statistical stuff.
    res.mean_x = res.sum_x/res.N;
    res.mean_f = res.sum_f/res.N;
    res.sum_sq_res = (T)(0);
    res.Sxf = (T)(0);
    res.Sxx = (T)(0);
    res.Sff = (T)(0);
    for(const auto &P : this->samples){
        res.sum_sq_res += std::pow(P[2] - (res.intercept + res.slope*P[0]), 2);
        res.Sxf += (P[0] - res.mean_x)*(P[2] - res.mean_f);
        res.Sxx += std::pow(P[0] - res.mean_x, 2.0);
        res.Sff += std::pow(P[2] - res.mean_f, 2.0);
    }
    res.sigma_f = std::sqrt(res.sum_sq_res/res.dof);
    res.sigma_slope = res.sigma_f*std::sqrt(res.N/common_denom);
    res.sigma_intercept = res.sigma_f*std::sqrt(res.sum_x/common_denom);
    res.lin_corr = res.Sxf/std::sqrt(res.Sxx*res.Sff);
    res.covariance = res.Sxf/res.N;
    res.tvalue = res.lin_corr*std::sqrt(res.dof/((T)(1)-std::pow(res.lin_corr,2.0)));

    //Reminder: this p-value is based on t-values and is thus only approximately correct!
    res.pvalue = svl::P_From_StudT_2Tail(res.tvalue, res.dof);

    if(OK != nullptr) *OK = true;
    return std::move(res);
}       

    template lin_reg_results<float > samples_1D<float >::Linear_Least_Squares_Regression(bool *OK, bool SkipExtras) const;
    template lin_reg_results<double> samples_1D<double>::Linear_Least_Squares_Regression(bool *OK, bool SkipExtras) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> lin_reg_results<T> samples_1D<T>::Weighted_Linear_Least_Squares_Regression(bool *OK, T *slope_guess) const {
    // This routine is not an exact drop-in replacement for the un-weighted case!
    //
    // It is a more sophisticated linear regression. Takes into account both sigma_x_i and sigma_f_i uncertainties to 
    // compute a weighted linear regression using likelihood-maximizing weighting. The sigma_x_i uncertainties are 
    // converted into equivalent sigma_f_i uncertainties based on a trial (ordinary, non-weighted) linear regression. So 
    // the underlying assumption is that the uncertainties are of sufficient uniformity that they will not substantially 
    // affect the regression. This technique is covered in chapter 8 of John R. Taylor's "An Introduction to Error 
    // Analysis" 2nd Ed.. See eqn. 8.21, its footnote, and problem 8.9 for detail. A better solution would be to 
    // iterate through the regression with successively-computed slopes, only stopping when (if?) no change is detected
    // between iterations. (How to achieve this is discussed below.) Some quantities computed were inspired by Numerical
    // Recipes.
    //
    // From what I can ascertain, this routine IS suitable for properly fitting 'linearized' data, so long as the 
    // uncertainties are transformed according to the standard error propagation formula.
    //
    // Returns: {[0]slope, [1]sigma_slope, [2]intercept, [3]sigma_intercept, [4]the # of datum used, [5]std dev of 
    // the data ("sigma_f"), [6]Chi-square, [7]covariance, [8]linear corr. coeff. (aka "Pearson's), [9]Q-value of fit,
    // [10]covariance of the slope and intercept, [11]corr. coeff. for sigma_slope and sigma_intercept}.
    //
    // Many of these parameters are also reported in the un-weighted linear regression case. The differences:
    //
    // - There is no t-value because a Chi-square statistic is now used. There is nothing like the t-value for the Chi-
    //   square, so the Chi-square itself is reported. (This is essentially a weighted sum-of-square residuals.)
    //
    // - There is no p-value. Instead a q-value is reported. This is the Chi-square equivalent to a p-value, but is less
    //   precise. (It does not involve the linear correlation coefficient like the un-weighted case.) 
    //
    // - The correlation between uncertainties ("r_{a,b}" in Numerical Recipes) is reported (c.f., linear correlation 
    //   coefficient -- aka Pearson's -- which computes the correlation between abscissa and ordinate). If r_{a,b} is 
    //   positive, errors in the slope and intercept are likely to have the same sign. If negative, they'll be likely to
    //   have opposite signs. Do not confuse r_{a,b} and Pearson's r. They are totaly different quantities!
    //
    //   Also note that if all the datum have ~zero uncertainty, and are thus infinitely weighted, r_{a,b} is not useful
    //   for anything because sigma_slope and sigma_intercept are zero.
    //
    // - The 'covariance of the slope and intercept' ["cov(a,b)"] is different from the 'covariance of the data.' 
    //   It is introduced in Numerical Recipes, but not discussed in any depth.
    //
    // NOTE: You can determine the best slope_guess to use by starting with a guess and iterating the regression.
    //       Simple tests indicate it seems stable and can be jump-started with just about anything reasonable.
    //       For example, using data:     { { 1.00, 0.10, 2.0000, 134.221 },
    //                                    { 1.05, 0.20, 3.0500, 134.221 },
    //                                    { 1.10, 0.20, 10.600, 134.221 },
    //                                    { 1.40, 0.90, 1000.0, 134.221 } }
    //       with the first and last points to guess the slope initially gives successive slopes: 
    //                                1523, 1675, 1643, 1649, 1648, 1648 ... 
    //       with a corresponding shrink of sigma_slope. It is recommended to do this if your uncertainties are highly
    //       variable or if a small number of points appear to be determining the fit.
    //
    //       Leaving slope_guess as nullptr will default to an un-weighted regression slope.
    //
    // NOTE: This routine can be made to report nearly identical parameters as standard (unweighted) linear regression by
    //       making all sigma_x_i zero and making all sigma_f_i equal to the std dev of the data ("sigma_y") reported by 
    //       standard linear regression. Alternatively, increasing sigma_x_i while decreasing sigma_f_i can have the
    //       same effect (the exact amount depends on the assumptions made about uncertainties).
    //
    // *****************************************************************************************************************
    // ****** Therefore, setting the uncertainties to (T)(1) WILL NOT give you standard linear regression! If you ******
    // ****** don't have uncertainties, I strongly recommend you to use standard non-weighted linear regression!  ******
    // *****************************************************************************************************************
    //
    // NOTE: The deferral of determining the slope with non-weight linear regression is often used to get a first-order 
    //       guess of fit parameters for more sophisticated routines (i.e., weighted linear regression). This is an 
    //       established (possibly even statistically legitimate) strategy. Still, it could be more robust. If in doubt,
    //       use a Monte Carlo or bootstrap approach instead.
    //
    // NOTE: From what I can tell, the conversion of sigma_x_i to an equivalent sigma_f_i based on the slope should always
    //       combine uncertainties in quadrature -- even if the data are not assumed to be normal, random, and independent
    //       themselves. This seems to be what John R. Taylor advocates in his book, anyways. I'm not thrilled about this
    //       and so have decided to play it safe and use the assumption-less propagation formula if no assumptions are 
    //       specified. This may result in an over-estimate of the uncertainty.
    //
    // NOTE: Like the weighted mean, if there are any datum with effectively zero sigma_f_i mixed with non-zero sigma_f_i
    //       datum, the latter will simply not contribute. And because we cannot differentiate various strengths of inf
    //       the strong certainty datum will all be treated as if they have identical weighting. This will be surprising
    //       if you're not prepared, but is the best we can do short of total failure. (And it still makes sense 
    //       logically, so it seems OK to do it this way.)
    //

    bool l_OK(false);
    if(OK != nullptr) *OK = false;
    const lin_reg_results<T> ret_on_err;
    lin_reg_results<T> res;

    //Approximating slope, used for converting sigma_x_i to equiv. sigma_f_i.
    T approx_slope;
    if(slope_guess != nullptr){
        approx_slope = *slope_guess;
    }else{
        //Attempt to compute the non-weighted linear regression slope. 
        const auto nwlr = this->Linear_Least_Squares_Regression(&l_OK);
        if(!l_OK){
            if(OK == nullptr) FUNCERR("Standard linear regression failed. Cannot properly propagate unertainties");
            FUNCWARN("Standard linear regression failed. Cannot properly propagate unertainties. Bailing");
            return ret_on_err;
        }
        approx_slope = nwlr.slope;
    }

    //Ensure the data is suitable.
    if(this->size() < 3){
        //If we have two points, there are as many parameters as datum. We cannot even compute the stats...
        if(OK == nullptr) FUNCERR("Unable to perform meaningful linear regression with so few points");
        FUNCWARN("Unable to perform meaningful linear regression with so few points. Bailing");
        return ret_on_err;
    }
    res.N = static_cast<T>(this->size());
    res.dof = res.N - (T)(2);

    //Segregate datum into two groups based on whether the weighting is finite.
    samples_1D<T> fin_data, inf_data;

    //Pass over the datum, preparing them and computing some simple quantities.
    res.sum_x  = (T)(0);
    res.sum_f  = (T)(0);
    res.sum_xx = (T)(0);
    res.sum_xf = (T)(0);

    res.sum_w    = (T)(0);
    res.sum_wx   = (T)(0);
    res.sum_wf   = (T)(0);
    res.sum_wxx  = (T)(0);
    res.sum_wxf  = (T)(0);

    const bool inhibit_sort = true;
    for(const auto &P : this->samples){
        auto CP = P; //"CP" == "copy of p".

        //Using the standard linear regression slope, transform sigma_x_i into equivalent sigma_f_i.
        if(this->uncertainties_known_to_be_independent_and_random){
            CP[3] = std::hypot(CP[3], approx_slope*CP[1]);
        }else{
            CP[3] = std::abs(CP[3]) + std::abs(approx_slope*CP[1]);
        }
        CP[1] = (T)(0);

        //Take care of the un-weighted quantities.
        res.sum_x  += CP[0];
        res.sum_xx += CP[0]*CP[0];
        res.sum_f  += CP[2];
        res.sum_xf += CP[0]*CP[2];

        //Will be infinite if P[3] is zero. If any are finite, all these sums will also be finite.
        const T w_i = (std::isfinite(CP[3])) ? (T)(1)/(CP[3]*CP[3]) : (T)(0);
        if(std::isfinite(w_i)){
            fin_data.push_back(CP,inhibit_sort);
        }else{
            inf_data.push_back(CP,inhibit_sort);
            res.sum_w     = std::numeric_limits<T>::infinity();
            res.sum_wx    = std::numeric_limits<T>::infinity();
            res.sum_wf    = std::numeric_limits<T>::infinity();
            res.sum_wxx   = std::numeric_limits<T>::infinity();
            res.sum_wxf   = std::numeric_limits<T>::infinity();
            continue;
        }
        res.sum_w    += w_i;
        res.sum_wx   += w_i*CP[0];
        res.sum_wxx  += w_i*CP[0]*CP[0];
        res.sum_wf   += w_i*CP[2];
        res.sum_wxf  += w_i*CP[0]*CP[2];
    }
    const bool tainted_by_inf = !inf_data.empty();
    res.mean_x = res.sum_x/res.N;
    res.mean_f = res.sum_f/res.N;
    if(tainted_by_inf){
        res.mean_wx = std::numeric_limits<T>::infinity();
        res.mean_wf = std::numeric_limits<T>::infinity();
    }else{
        res.mean_wx = res.sum_wx/res.N;
        res.mean_wf = res.sum_wf/res.N;
    }


    //Case 1 - all datum have finite weight, and all datum will have been pushed into fin_data.
    if(!tainted_by_inf){
        const T common_denom = res.sum_w*res.sum_wxx - res.sum_wx*res.sum_wx;
        if(!std::isnormal(common_denom)){
            //This cannot be zero, inf, or nan. Proceeding with a sub-normal is also a bad idea.
            if(OK == nullptr) FUNCERR("Encountered difficulties with data. Is the data pathological?");
            FUNCWARN("Encountered difficulties with data. Is the data pathological? Bailing");
            return ret_on_err;
        }

        res.slope     = (res.sum_w*res.sum_wxf - res.sum_wx*res.sum_wf)/common_denom;
        res.intercept = (res.sum_wxx*res.sum_wf - res.sum_wx*res.sum_wxf)/common_denom;
        if(!std::isfinite(res.slope) || !std::isfinite(res.intercept)){
            //While we *could* proceed, something must be off. Maybe an overflow (and inf or nan) during accumulation?
            if(OK == nullptr) FUNCERR("Encountered difficulties computing m and b. Is the data pathological?");
            FUNCWARN("Encountered difficulties computing m and b. Is the data pathological? Bailing");
            return ret_on_err;
        }

        //Now compute the statistical stuff.
        res.chi_square = (T)(0);
        res.Sxf        = (T)(0);
        res.Sxx        = (T)(0);
        res.Sff        = (T)(0);
        for(const auto &P : fin_data.samples){
            res.chi_square += std::pow((P[2] - (res.intercept + res.slope*P[0]))/P[3], 2.0);
            res.Sxf  += (P[0] - res.mean_x)*(P[2] - res.mean_f);
            res.Sxx  += (P[0] - res.mean_x)*(P[0] - res.mean_x);
            res.Sff  += (P[2] - res.mean_f)*(P[2] - res.mean_f);
        }
        res.sigma_slope     = std::sqrt(res.sum_w/common_denom);
        res.sigma_intercept = std::sqrt(res.sum_wxx/common_denom);
        res.covariance      = res.Sxf/res.N;
        res.lin_corr        = res.Sxf/std::sqrt(res.Sxx*res.Sff); //Pearson's linear correlation coefficient.

//TODO : check if this is the correct sigma_f or if I should be using corr_params or cov_params here.

        res.sigma_f         = std::sqrt(res.chi_square/(((T)(1)-std::pow(res.lin_corr,2.0))*res.Sff)); //(Of interest? sigma_f_i were provided.)
        res.qvalue          = svl::Q_From_ChiSq_Fit(res.chi_square, res.dof);
        res.cov_params      = -res.sum_wx/common_denom;
        res.corr_params     = -res.sum_wx/std::sqrt(res.sum_w*res.sum_wxx);

        if(OK != nullptr) *OK = true;
        return std::move(res);

    //Case 2 - some data had infinite weighting.
    }else{

        if(!fin_data.empty()){
            //Handling this case will require juggling infs and non-infs for all calculations. I have not done this.
            // To get around this, just give very small uncertainties to the problematic data. If this functionality
            // is needed (even though it is a truly stange situation), try doing something like an epsilon argument
            // where a portion of the data has small uncertainty epsilon and the rest has much larger values. Expand
            // all expressions for small epsilon with something like a Taylor expansion, and see if there is a finite
            // expansion.
            if(OK == nullptr) FUNCERR("Data has a mix of zero (or denormal) and non-zero uncertainties. Not (yet?) supported");
            FUNCWARN("Data has a mix of zero (or denormal) and non-zero uncertainties. Not (yet?) supported");
            return ret_on_err;
        }

        //Note: For the rest of the routine, we assume the data all have the same (infinite) weighting, so sums will all
        //      cancel nicely. I'm not sure we could possibly do anything else.

        const T common_denom = res.N*res.sum_xx - res.sum_x*res.sum_x; //Omitting factor [1/sigma_f_i^2].
        if(!std::isnormal(common_denom)){
            //This cannot be zero, inf, or nan. Proceeding with a sub-normal is also a bad idea.
            if(OK == nullptr) FUNCERR("Encountered difficulties with data. Is the data pathological?");
            FUNCWARN("Encountered difficulties with data. Is the data pathological? Bailing");
            return ret_on_err;
        }
        res.slope     = (res.N*res.sum_xf - res.sum_x*res.sum_f)/common_denom;
        res.intercept = (res.sum_xx*res.sum_f - res.sum_x*res.sum_xf)/common_denom;

        res.sum_sq_res = (T)(0);
        res.Sxf = (T)(0);
        res.Sxx = (T)(0);
        res.Sff = (T)(0);
        for(const auto &P : inf_data.samples){

// TODO : check if I need to use WEIGHTED means for these quantities.
            res.sum_sq_res += std::pow(P[2] - (res.intercept + res.slope*P[0]), 2.0);
            res.Sxf += (P[0] - res.mean_x)*(P[2] - res.mean_f);
            res.Sxx += (P[0] - res.mean_x)*(P[0] - res.mean_x);
            res.Sff += (P[2] - res.mean_f)*(P[2] - res.mean_f);
        }
        res.sigma_f         = std::sqrt(res.sum_sq_res/res.dof);
        res.sigma_slope     = (T)(0); //std::sqrt(infSumW/common_denom);
        res.sigma_intercept = (T)(0); //std::sqrt(res.sum_xx/common_denom);
        res.covariance      = res.Sxf/res.N;
        res.lin_corr        = res.Sxf/std::sqrt(res.Sxx*res.Sff); //Pearson's linear correlation coefficient.
        res.chi_square      = std::numeric_limits<T>::infinity(); //Due to sigma_x_i being ~zero.
        res.qvalue          = (T)(0); // Limit as ChiSq ---> inf.

        res.cov_params      = (T)(0); //Infinitely weighted down to zero.
        res.corr_params     = -res.sum_x/std::sqrt(res.N*res.sum_xx);

        if(OK != nullptr) *OK = true;
        return std::move(res);
    }
  
    FUNCERR("Programming error. Should not have got to this point"); 
    return ret_on_err;
}       

    template lin_reg_results<float > samples_1D<float >::Weighted_Linear_Least_Squares_Regression(bool *OK, float  *slope_guess) const;
    template lin_reg_results<double> samples_1D<double>::Weighted_Linear_Least_Squares_Regression(bool *OK, double *slope_guess) const;



//---------------------------------------------------------------------------------------------------------------------------
template <class T>   bool samples_1D<T>::Write_To_File(const std::string &filename) const {
    //This routine writes the numerical data to file in a 4-column format. It can be directly plotted or otherwise 
    // manipulated by, say, Gnuplot.
    //
    //NOTE: This routine will not overwrite or append an existing file. It will return 'false' on any error or if 
    //      encountering an existing file.
    if(Does_File_Exist_And_Can_Be_Read(filename)) return false;
    return WriteStringToFile(this->Write_To_String(), filename);
}

    template bool samples_1D<float >::Write_To_File(const std::string &filename) const;
    template bool samples_1D<double>::Write_To_File(const std::string &filename) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T>   bool samples_1D<T>::Read_From_File(const std::string &filename){
    //This routine reads numerical data from a file in 4-column format. No metadata is retained or recovered. 
    //
    // NOTE: This routine will return 'false' on any error.
    //
    // NOTE: This routine shouldn't overwrite existing data if a failure is encountered.
    //

    std::ifstream FI(filename, std::ifstream::in);
    if(!FI.good()) return false;

    const bool InhibitSort = false;
    samples_1D<T> indata;

    //Read in the numbers until EOF is encountered.
    std::string line;
    while(FI.good()){
        line.clear();
        std::getline(FI, line);
        if(line.empty()) continue;

//    while(!FI.eof()){
//        std::getline(FI, line);
//        if(FI.eof()) break;
        
        std::size_t nonspace = line.find_first_not_of(" \t");
        if(nonspace == std::string::npos) continue; //Only whitespace.
        if(line[nonspace] == '#') continue; //Comment.

        std::stringstream ss(line);
        T a, b, c, d;
        ss >> a >> b;
        const auto ab_ok = (!ss.fail());
        ss >> c >> d;
        const auto cd_ok = (!ss.fail());
        if(ab_ok && !cd_ok){
            indata.push_back(a, static_cast<T>(0),
                             b, static_cast<T>(0), InhibitSort);
        }else if(ab_ok && cd_ok){
            indata.push_back(a, b, c, d, InhibitSort);
        }
    }

    *this = indata;
    this->metadata.clear();
    return true;
}

    template bool samples_1D<float >::Read_From_File(const std::string &filename);
    template bool samples_1D<double>::Read_From_File(const std::string &filename);

//---------------------------------------------------------------------------------------------------------------------------
template <class T>   std::string samples_1D<T>::Write_To_String() const {
    std::stringstream out;
    for(const auto &P : this->samples) out << P[0] << " " << P[1] << " " << P[2] << " " << P[3] << std::endl;
    return out.str();
}

    template std::string samples_1D<float >::Write_To_String() const;
    template std::string samples_1D<double>::Write_To_String() const;


//---------------------------------------------------------------------------------------------------------------------------
template <class T> void samples_1D<T>::Plot(const std::string &Title) const {
    //This routine produces a very simple, default plot of the data.
    //
    Plotter2 plot_coll;
    if(!Title.empty()) plot_coll.Set_Global_Title(Title);
    plot_coll.Insert_samples_1D(*this);//,"", const std::string &linetype = "");
    plot_coll.Plot();
    return;
}

//    template void samples_1D<float >::Plot(const std::string &) const;
    template void samples_1D<double>::Plot(const std::string &) const;

//---------------------------------------------------------------------------------------------------------------------------
template <class T> void samples_1D<T>::Plot_as_PDF(const std::string &Title, const std::string &Filename_In) const {
    //This routine produces a very simple, default plot of the data.
    Plotter2 plot_coll;
    if(!Title.empty()) plot_coll.Set_Global_Title(Title);
    plot_coll.Insert_samples_1D(*this);//,"", const std::string &linetype = "");
    plot_coll.Plot_as_PDF(Filename_In);
    return;
}

//    template void samples_1D<float >::Plot_as_PDF(const std::string &, const std::string &) const;
    template void samples_1D<double>::Plot_as_PDF(const std::string &, const std::string &) const;
#endif

//---------------------------------------------------------------------------------------------------------------------------
template <class T>    std::ostream & operator<<( std::ostream &out, const samples_1D<T> &L ) {
    //Note: This friend is templated (Y) within the templated class (T). We only
    // care about friend template when T==Y.
    out << "(samples_1D.";
    out << " normalityassumed= " << (L.uncertainties_known_to_be_independent_and_random ? 1 : 0);
    out << " num_samples= " << L.size() << " ";
    for(auto s_it = L.samples.begin(); s_it != L.samples.end(); ++s_it){
        out << (*s_it)[0] << " " << (*s_it)[1] << " " << (*s_it)[2] << " " << (*s_it)[3] << " ";
    }
    out << ")";
    return out;
}

    template std::ostream & operator<<(std::ostream &out, const samples_1D<float > &L );
    template std::ostream & operator<<(std::ostream &out, const samples_1D<double> &L );

//---------------------------------------------------------------------------------------------------------------------------
template <class T>    std::istream &operator>>( std::istream &in, samples_1D<T> &L ) {
    //Note: This friend is templated (Y) within the templated class (T). We only
    //      care about friend template when T==Y.
    //
    //NOTE: Best to use like: ` std::stringstream("...text...") >> samps; `.
    //
    //NOTE: If you're getting weird crashes and not sure why, try using a fresh stringstream.
    //      Naiively reusing a stringstream will probably cause issues. 
    const bool skip_sort = true;
    L.samples.clear(); 
    std::string grbg;
    long int N;
    long int U;
    in >> grbg; //'(samples_1D.'
    in >> grbg; //'normalityassumed='
    in >> U;    //'1' or '0'
    in >> grbg; //'num_samples='
    in >> N;    //'13'   ...or something...
    //FUNCINFO("N is " << N);
    for(long int i=0; i<N; ++i){
        T x_i, sigma_x_i, f_i, sigma_f_i;
        in >> x_i >> sigma_x_i >> f_i >> sigma_f_i;
        L.push_back({x_i, sigma_x_i, f_i, sigma_f_i}, skip_sort);
    }
    in >> grbg; //')'
    if(U == 1){     L.uncertainties_known_to_be_independent_and_random = true;
    }else{          L.uncertainties_known_to_be_independent_and_random = false;
    }
    L.stable_sort();
    return in;
}

    template std::istream & operator>>(std::istream &out, samples_1D<float > &L );
    template std::istream & operator>>(std::istream &out, samples_1D<double> &L );


//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

template <class C> 
samples_1D<typename C::value_type> 
Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(const C &nums, long int N, bool explicitbins){
    //This function takes a list of (possibly unordered) numbers and returns a histogram with N bars of equal width
    // suitable for plotting or further computation. The histogram will be normalized such that each bar will be 
    // (number_of_points_represented_by_bar/total_number_of_points). In other words, the occurence rate.
    //
    using T = typename C::value_type; // float or double.

    const T NDasT = static_cast<T>(nums.size());

    //Put the data into a samples_1D with equal sigma_f_i and no uncertainty.
    const bool inhibit_sort = true; //Temporarily inhibit the sort. We just defer it until all the data is ready.
    samples_1D<T> shtl; //Used to shuttle the data to the histogramming routine.
    for(const auto &P : nums){
        shtl.push_back( { P, (T)(0), (T)(1), (T)(0) }, inhibit_sort );
    }
    shtl.stable_sort();

    //Get a non-normalized histogram out of the data. 
    samples_1D<T> out = shtl.Histogram_Equal_Sized_Bins(N, explicitbins);

    //Normalize the bins with the total number of datum. Ensure the uncertainty is zero, because we have ignored it thusfar.
    for(auto &P : out.samples){
        P[2] /= NDasT;
        P[3] = (T)(0);
    }
    return std::move(out);
}

    template samples_1D<float > Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(const std::list<float > &nums, long int N, bool explicitbins);
    template samples_1D<double> Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(const std::list<double> &nums, long int N, bool explicitbins);

    template samples_1D<float > Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(const std::vector<float > &nums, long int N, bool explicitbins);
    template samples_1D<double> Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(const std::vector<double> &nums, long int N, bool explicitbins);




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////              SANDBOX STUFF              ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
    I didn't need this at the moment, but I feel it is something which might be useful for many things.
    Cannot get it to work so I abandoned it for a special case which happened to be applicable (planar_image).

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //This is a generic 3D bounding box routine. It is written in such a way as to 
    // hopefully move it into a more general spot somewhere in YgorMath.cc.

    // Diagram:
    //              _______------D_                               
    //            A __             --__                      
    //           /    --__             ---_                  
    //           |        --__             -_                
    //          /             --__     ___-- C                   
    //          |                  B --     /                
    //       r1  --__             /         |                    
    //         /     --__         |        /            
    //         |         --__    /    ____ |            
    //        /              --_ | ---      r3                   
    //        |                   r2      /             
    //       E __               /         |                  
    //           --__           |        /                  
    //               --__      /       __G           z         
    //                   --__  |  ___--              |  y         
    //                        F --                   | /    
    //                                               |/____x
    // Requirements:
    //   1) The points (A,B,C,D) must be ordered (in either orientation). 
    //   2) The points (E,F,G,H) must be ordered in the SAME orientation as (A,B,C,D).
    //   3) Point A must be next to all of (B,D,E).
    //   4) Point B must be next to all of (A,C,F).
    //   5) Point C must be next to all of (C,D,G).
    //   6) Point D must be next to all of (A,C,H).  (etc.)
    //
    // Notes:
    //   1) This routine does *not* require any of the points to lie on any planes, just
    //      that they are ordered uniformly.
    //   2) We may be given (r1,r2,r3,r4) and a thickness (A + E)*0.5 instead. This limits
    //      the geometry, but is equivalent (ie. we can reconstruct (A,B,C,D,E,F,G,H)).
    //

    const auto r1 = this->position(           0,              0);  // = (A + E)*0.5;
    const auto r2 = this->position(this->rows-1,              0);  // = (B + F)*0.5;
    const auto r3 = this->position(this->rows-1,this->columns-1);  // = (C + G)*0.5;  
    const auto r4 = this->position(           0,this->columns-1);  // = (D + H)*0.5;

    const auto dt = this->pxl_dz; // = |A-E| == |B-F| == etc..

    const auto A  = r1 + ((r2-r1).Cross(r4-r1)).unit()*(dt*0.5);
    const auto B  = A  + (r2-r1);
    const auto C  = A  + (r3-r1);
    const auto D  = A  + (r4-r1);

    const auto E  = r1 - ((r2-r1).Cross(r4-r1)).unit()*(dt*0.5);
    const auto F  = A  + (r2-r1);
    const auto G  = A  + (r3-r1);
    const auto H  = A  + (r4-r1);

    //Now create planes describing the edges. (Arbitraily) choose outward as the 
    // positive orientation. Once we choose the orientation, we must stick with it.
    std::list<plane<double>> planes;
    planes.push_back(plane<double>( ((B-A).Cross(D-A)).unit(), A ));
    planes.push_back(plane<double>( ((E-A).Cross(B-A)).unit(), A ));
    planes.push_back(plane<double>( ((D-A).Cross(E-A)).unit(), A ));

    planes.push_back(plane<double>( ((C-G).Cross(F-G)).unit(), G ));
    planes.push_back(plane<double>( ((F-G).Cross(H-G)).unit(), G ));
    planes.push_back(plane<double>( ((H-G).Cross(C-G)).unit(), G ));

    //Cycle through planes to see if the point is *above* (as per our out-positive
    // orientation).
    for(auto it = planes.begin(); it != planes.end(); ++it){
        if(it->Is_Point_Above_Plane(in)) return false;
    }

    return true;

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
#pragma GCC diagnostic pop

