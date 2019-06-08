//
//  eigen_utils.hpp
//  Visible
//
//  Created by Arman Garakani on 6/7/19.
//

#ifndef eigen_utils_hpp
#define eigen_utils_hpp

#include <Eigen/QR>
#include <stdio.h>
#include <vector>


#if SFINAE
template <typename T>
class HasXmethod
{
private:
    typedef char YesType[1];
    typedef char NoType[2];
    
    template <typename C> static YesType& test( decltype(&C::x() ) ) ;
    template <typename C> static NoType& test(...);
    
    
public:
    enum { value = sizeof(test<T>(0)) == sizeof(YesType) };
};


// SFINAE test
template <typename T>
class HasYmethod
{
private:
    typedef char YesType[1];
    typedef char NoType[2];
    
    template <typename C> static YesType& test( decltype(&C::y() ) ) ;
    template <typename C> static NoType& test(...);
    
    
public:
    enum { value = sizeof(test<T>(0)) == sizeof(YesType) };
};
template<typename T, typename = typename std::enable_if<HasXmethod<T>::value, std::string>::type,
typename = typename std::enable_if<HasYmethod<T>::value, std::string>::type>
#endif

void polyfit(const std::vector<double> &xv, const std::vector<double> &yv, std::vector<double> &coeff, int order);

// Build x sequentially by adding incr
void polyfit (const std::vector<double> &y, double incr, std::vector<double> &coeff, int order);

template <class Iterator>
void polyfit (Iterator Ib, Iterator Ie, double incr, std::vector<double> &coeff, int order){
    
    auto n = Ie - Ib;
    assert (n);
    
    std::vector<double> xv, yv;
    double index = 0.0;
    Iterator ip = Ib;
    
    while (ip < Ie){
        xv.push_back(index);
        yv.push_back(*ip);
        index = index + incr;
        ip++;
    }
    polyfit(xv, yv, coeff, order);
    
}



template<typename T>
void polyfit(const std::vector<T> &xys, std::vector<double> &coeff, int order);

#endif /* eigen_utils_hpp */
