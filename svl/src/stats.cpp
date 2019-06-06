//
//  stats.cpp
//  
//
//  Created by Arman Garakani on 6/6/19.
//


#include <gsl/gsl_errno.h>     //Needed for GSL error reporting.
                               //#include <gsl/gsl_sf_hyperg.h> //(Gauss) Hypergeometric function. (This failed me!)
#include <gsl/gsl_integration.h> //Needed to numerically compute an integral in lieu of the hypergeometric function.
#include <gsl/gsl_math.h>
#include <gsl/gsl_sf_gamma.h>  //Gamma function.
#include <gsl/gsl_sf_result.h>
#include <algorithm>
#include <array>
#include <cmath>       //Needed for fabs, signbit, sqrt, etc...
#include <iterator>
#include <limits>      //Needed for std::numeric_limits::max().
#include <memory>
#include <stdexcept>
#include <vector>

#include "core/gen_utils.hpp"    //For the FUNC* and PERCENT_ERR macro functions, containerSort().
#include "core/stats.hpp"

#ifndef YGORSTATS_DISABLE_ALL_SPECIALIZATIONS
    #define YGORSTATS_DISABLE_ALL_SPECIALIZATIONS
#endif


//--------------------------------------------- "Building block" routine ----------------------------------------------------


template <class C> typename C::value_type svl::Min(C in){
    using T = typename C::value_type; //Internal type, like double or integer.
    
    if(in.empty()){
        if(std::numeric_limits<T>::has_quiet_NaN){
            return std::numeric_limits<T>::quiet_NaN();
        }else{
            FUNCERR("Cannot find minimum of zero elements and cannot emit NaN. Cannot continue");
        }
    }
    if(in.size() == 1) return in.front();
    
    svl::Running_MinMax<T> rmm;
    for(const auto &n : in) rmm.Digest(n);
    return rmm.Current_Min();
}
#ifndef YGORSTATS_DISABLE_ALL_SPECIALIZATIONS
template double   svl::Min(std::list<double>     in);
template double   svl::Min(std::vector<double>   in);

template float    svl::Min(std::list<float >     in);
template float    svl::Min(std::vector<float >   in);

template uint8_t  svl::Min(std::list<uint8_t>   in);
template uint8_t  svl::Min(std::vector<uint8_t> in);

template int8_t   svl::Min(std::list<int8_t>    in);
template int8_t   svl::Min(std::vector<int8_t>  in);

template uint16_t svl::Min(std::list<uint16_t>   in);
template uint16_t svl::Min(std::vector<uint16_t> in);

template int16_t  svl::Min(std::list<int16_t>    in);
template int16_t  svl::Min(std::vector<int16_t>  in);

template uint32_t svl::Min(std::list<uint32_t>   in);
template uint32_t svl::Min(std::vector<uint32_t> in);

template int32_t  svl::Min(std::list<int32_t>    in);
template int32_t  svl::Min(std::vector<int32_t>  in);

template uint64_t svl::Min(std::list<uint64_t>   in);
template uint64_t svl::Min(std::vector<uint64_t> in);

template int64_t  svl::Min(std::list<int64_t>    in);
template int64_t  svl::Min(std::vector<int64_t>  in);
#endif


template <class C> typename C::value_type svl::Max(C in){
    using T = typename C::value_type; //Internal type, like double or integer.
    
    if(in.empty()){
        if(std::numeric_limits<T>::has_quiet_NaN){
            return std::numeric_limits<T>::quiet_NaN();
        }else{
            FUNCERR("Cannot find maximum of zero elements and cannot emit NaN. Cannot continue");
        }
    }
    if(in.size() == 1) return in.front();
    
    svl::Running_MinMax<T> rmm;
    for(const auto &n : in) rmm.Digest(n);
    return rmm.Current_Max();
}
#ifndef YGORSTATS_DISABLE_ALL_SPECIALIZATIONS
template double   svl::Max(std::list<double>     in);
template double   svl::Max(std::vector<double>   in);

template float    svl::Max(std::list<float >     in);
template float    svl::Max(std::vector<float >   in);

template uint8_t  svl::Max(std::list<uint8_t>   in);
template uint8_t  svl::Max(std::vector<uint8_t> in);

template int8_t   svl::Max(std::list<int8_t>    in);
template int8_t   svl::Max(std::vector<int8_t>  in);

template uint16_t svl::Max(std::list<uint16_t>   in);
template uint16_t svl::Max(std::vector<uint16_t> in);

template int16_t  svl::Max(std::list<int16_t>    in);
template int16_t  svl::Max(std::vector<int16_t>  in);

template uint32_t svl::Max(std::list<uint32_t>   in);
template uint32_t svl::Max(std::vector<uint32_t> in);

template int32_t  svl::Max(std::list<int32_t>    in);
template int32_t  svl::Max(std::vector<int32_t>  in);

template uint64_t svl::Max(std::list<uint64_t>   in);
template uint64_t svl::Max(std::vector<uint64_t> in);

template int64_t  svl::Max(std::list<int64_t>    in);
template int64_t  svl::Max(std::vector<int64_t>  in);
#endif



template <class C> typename C::value_type svl::Sum(C in){
    //Sums a container of numbers. Implements the Kahan summation (aka compensated summation) algorithm to help reduce loss
    // of precision. It isn't perfect. Especially if there are both positive and negative numbers of large or varying
    // magnitude.
    //
    // NOTE: This routine currently does not have any overflow protection or detection.
    //
    // NOTE: This routine is templated on a STL iterable class like std::list<double> or std::vector<double>.
    //
    using T = typename C::value_type; //Internal type, like double or integer.
    
    if(in.empty()){
        if(std::numeric_limits<T>::has_quiet_NaN){
            return std::numeric_limits<T>::quiet_NaN();
        }else{
            FUNCERR("Cannot sum zero elements and cannot emit NaN. Cannot continue");
        }
    }
    if(in.size() == 1) return in.front();
    
    //In an effort to try reduce numerical errors, we will sum from lowest to highest magnitude.
    //
    // Note: I had a constexpr-if-based routine here, but the old compiler in Debian failed to compile it.
    //       This version works just as well.
    auto sort_by_magnitude = [](T l, T r) -> bool {
        const auto l_d = static_cast<long double>(l);
        const auto r_d = static_cast<long double>(r);
        return (std::abs(l_d) < std::abs(r_d));
    };
    containerSort(in, sort_by_magnitude);
    
    //Now walk over the numbers, accumulating into the sum.
    T sum = in.front();   //Prime the sum.
    T compen = (T)(0);    //A compensation term, for correcting numerical lost summing the previous term.
    for(auto it = std::next(in.begin()); it != in.end(); ++it){
        //Since *it is a single (small compared to sum) number, and compen will be small,
        // apply the correction term to it ASAP.
        const T y = *it - compen;
        
        //The sum is potentially large compared to this single term, so precision is lost here.
        const T nextsum = sum + y;
        
        //Now figure out if anything was lost by individually subracting the sum and compensated
        // term. Also roll nextsum into sum.
        compen = nextsum - sum;
        compen -= y;
        
        sum = nextsum;
    }
    
    //Now there is potentially a small compensation term and the large sum term. In the general case the
    // compensation term cannot be summed into the sum but it might work in certain circumstances.
    return sum - compen;
    
    // NOTE: If you want an estimate of how much loss has occured, I believe the compen term is effectively
    //       a measure of numerical uncertainty introduced. It could be returned for various purposes.
    //return { sum, compen };
}
#ifndef YGORSTATS_DISABLE_ALL_SPECIALIZATIONS
template double   svl::Sum(std::list<double>     in);
template double   svl::Sum(std::vector<double>   in);

template float    svl::Sum(std::list<float >     in);
template float    svl::Sum(std::vector<float >   in);

template uint8_t svl::Sum(std::list<uint8_t>   in);
template uint8_t svl::Sum(std::vector<uint8_t> in);

template int8_t  svl::Sum(std::list<int8_t>    in);
template int8_t  svl::Sum(std::vector<int8_t>  in);

template uint16_t svl::Sum(std::list<uint16_t>   in);
template uint16_t svl::Sum(std::vector<uint16_t> in);

template int16_t  svl::Sum(std::list<int16_t>    in);
template int16_t  svl::Sum(std::vector<int16_t>  in);

template uint32_t svl::Sum(std::list<uint32_t>   in);
template uint32_t svl::Sum(std::vector<uint32_t> in);

template int32_t  svl::Sum(std::list<int32_t>    in);
template int32_t  svl::Sum(std::vector<int32_t>  in);

template uint64_t svl::Sum(std::list<uint64_t>   in);
template uint64_t svl::Sum(std::vector<uint64_t> in);

template int64_t  svl::Sum(std::list<int64_t>    in);
template int64_t  svl::Sum(std::vector<int64_t>  in);

#endif


template <class C> typename C::value_type svl::Sum_Squares(C in){
    //Sums the squares of the given numbers. Overflow is not currently detected, though it could be if needed.
   // using T = typename C::value_type; //Internal type, like double or integer.
    for(auto &elem : in) elem = std::pow(elem,2);
    return svl::Sum(std::move(in));
}
#ifndef YGORSTATS_DISABLE_ALL_SPECIALIZATIONS
template double   svl::Sum_Squares(std::list<double>     in);
template double   svl::Sum_Squares(std::vector<double>   in);

template uint64_t svl::Sum_Squares(std::list<uint64_t>   in);
template uint64_t svl::Sum_Squares(std::vector<uint64_t> in);

template int64_t  svl::Sum_Squares(std::list<int64_t>    in);
template int64_t  svl::Sum_Squares(std::vector<int64_t>  in);
#endif


template <class C> typename C::value_type svl::Percentile(C in, double frac){
    //Finds a percentile of the given numbers, using an average of the two middle numbers if an even number of
    // of numbers is provided. frac*100 is the 'k^{th} percentile', so:
    //   frac = 0.50 is the 50^{th} percentile (i.e., the median),
    //   frac = 0.10 is the 10^{th} percentile,
    //   frac = 0.01 is the 1^{th} percentile, etc..
    //
    // Note: Linear interpolation is used if no individual element corresponds exactly.
    //
    // Note: The median may not *exactly* (i.e., bit-for-bit) match any individual element, even if it
    //       falls exactly on the requested percentile. (This simplifies the implementation.)
    //
    using T = typename C::value_type; //Internal type, like double or integer.
    
    if(in.empty()){
        if(std::numeric_limits<T>::has_quiet_NaN){
            return std::numeric_limits<T>::quiet_NaN();
        }else{
            FUNCERR("Cannot find percentile of zero elements and cannot emit NaN. Cannot continue");
        }
    }else if(in.size() == 1){
        return in.front();
    }else if(!isininc(static_cast<T>(0),frac,static_cast<T>(1))){
        FUNCERR("Invalid argument provided: frac must be [0,1]");
    }
    
    auto sort_low_to_high = [](T l, T r) -> bool {
        return l < r;
    };
    containerSort(in, sort_low_to_high);
    
    const auto N = static_cast<long int>(in.size()) - 1;
    const auto M = frac * N;
    const auto MP = std::floor(M); // The integer part of M.
    const auto R = M - MP; // The extra bit (remainder).
    
    const auto MP_int = static_cast<long int>(MP);
    
    if(MP_int == N) return in.back();
    
    auto L_it = std::next(in.begin(), MP_int);
    auto R_it = std::next(L_it);
    
    return static_cast<T>( (1.0 - R)*(*L_it) + R*(*R_it) );
}
#ifndef YGORSTATS_DISABLE_ALL_SPECIALIZATIONS
template double   svl::Percentile(std::list<double>     in, double frac);
template double   svl::Percentile(std::vector<double>   in, double frac);

template float    svl::Percentile(std::list<float >     in, double frac);
template float    svl::Percentile(std::vector<float >   in, double frac);

template uint8_t  svl::Percentile(std::list<uint8_t>    in, double frac);
template uint8_t  svl::Percentile(std::vector<uint8_t>  in, double frac);

template int8_t   svl::Percentile(std::list<int8_t>     in, double frac);
template int8_t   svl::Percentile(std::vector<int8_t>   in, double frac);

template uint16_t svl::Percentile(std::list<uint16_t>   in, double frac);
template uint16_t svl::Percentile(std::vector<uint16_t> in, double frac);

template int16_t  svl::Percentile(std::list<int16_t>    in, double frac);
template int16_t  svl::Percentile(std::vector<int16_t>  in, double frac);

template uint32_t svl::Percentile(std::list<uint32_t>   in, double frac);
template uint32_t svl::Percentile(std::vector<uint32_t> in, double frac);

template int32_t  svl::Percentile(std::list<int32_t>    in, double frac);
template int32_t  svl::Percentile(std::vector<int32_t>  in, double frac);

template uint64_t svl::Percentile(std::list<uint64_t>   in, double frac);
template uint64_t svl::Percentile(std::vector<uint64_t> in, double frac);

template int64_t  svl::Percentile(std::list<int64_t>    in, double frac);
template int64_t  svl::Percentile(std::vector<int64_t>  in, double frac);

#endif


template <class C> typename C::value_type svl::Median(C in){
    return svl::Percentile(in, 0.5);
}
#ifndef YGORSTATS_DISABLE_ALL_SPECIALIZATIONS
template double   svl::Median(std::list<double>     in);
template double   svl::Median(std::vector<double>   in);

template float    svl::Median(std::list<float >     in);
template float    svl::Median(std::vector<float >   in);

template uint8_t svl::Median(std::list<uint8_t>   in);
template uint8_t svl::Median(std::vector<uint8_t> in);

template int8_t  svl::Median(std::list<int8_t>    in);
template int8_t  svl::Median(std::vector<int8_t>  in);

template uint16_t svl::Median(std::list<uint16_t>   in);
template uint16_t svl::Median(std::vector<uint16_t> in);

template int16_t  svl::Median(std::list<int16_t>    in);
template int16_t  svl::Median(std::vector<int16_t>  in);

template uint32_t svl::Median(std::list<uint32_t>   in);
template uint32_t svl::Median(std::vector<uint32_t> in);

template int32_t  svl::Median(std::list<int32_t>    in);
template int32_t  svl::Median(std::vector<int32_t>  in);

template uint64_t svl::Median(std::list<uint64_t>   in);
template uint64_t svl::Median(std::vector<uint64_t> in);

template int64_t  svl::Median(std::list<int64_t>    in);
template int64_t  svl::Median(std::vector<int64_t>  in);

#endif


template <class C> typename C::value_type svl::Mean(C in){
    //Finds the mean of the given numbers.
    using T = typename C::value_type; //Internal type, like double or integer.
    
    if(in.empty()){
        if(std::numeric_limits<T>::has_quiet_NaN){
            return std::numeric_limits<T>::quiet_NaN();
        }else{
            FUNCERR("Cannot find mean of zero elements and cannot emit NaN. Cannot continue");
        }
    }
    const auto N = static_cast<T>(in.size());
    return svl::Sum(std::move(in)) / N;
}
#ifndef YGORSTATS_DISABLE_ALL_SPECIALIZATIONS
template double   svl::Mean(std::list<double>     in);
template double   svl::Mean(std::vector<double>   in);

template float    svl::Mean(std::list<float >     in);
template float    svl::Mean(std::vector<float >   in);

template uint8_t svl::Mean(std::list<uint8_t>   in);
template uint8_t svl::Mean(std::vector<uint8_t> in);

template int8_t  svl::Mean(std::list<int8_t>    in);
template int8_t  svl::Mean(std::vector<int8_t>  in);

template uint16_t svl::Mean(std::list<uint16_t>   in);
template uint16_t svl::Mean(std::vector<uint16_t> in);

template int16_t  svl::Mean(std::list<int16_t>    in);
template int16_t  svl::Mean(std::vector<int16_t>  in);

template uint32_t svl::Mean(std::list<uint32_t>   in);
template uint32_t svl::Mean(std::vector<uint32_t> in);

template int32_t  svl::Mean(std::list<int32_t>    in);
template int32_t  svl::Mean(std::vector<int32_t>  in);

template uint64_t svl::Mean(std::list<uint64_t>   in);
template uint64_t svl::Mean(std::vector<uint64_t> in);

template int64_t  svl::Mean(std::list<int64_t>    in);
template int64_t  svl::Mean(std::vector<int64_t>  in);

#endif


template <class C> typename C::value_type svl::Unbiased_Var_Est(C in){
    //This is an unbiased estimate of a population's variance, as computed from a finite sample size.
    // If you have the *entire* population (ie. every entity) then this will produce a slightly incorrect
    // value. See http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance for more info.
    //
    // This computes the variance (aka sigma^2, aka [std dev of the data]^2). If the distribution is
    // normal, the variance computed will be the approximate variance of each *individual* datum. (So all
    // datum are assumed to have equal variance!) Take a sqrt to get the 'std dev of the data' which has
    // the same assumption. The 'std dev of the data' is the std dev of each *individual* datum.
    //
    // If you are combining datum to compute a mean, you instead should use the 'variance of the mean'.
    // This can be calculated as (variance of the data)/N. (This sigma is reduced because all the datum are
    // used to produce a single number -- the mean.) The sqrt of the 'variance of the data' is known as
    // the 'std dev of the mean' or 'std err'. The 'std dev of the mean' or 'std err' should be used
    // for, e.g., error bars and confidence intervals on plots. Use 1*sigma for 68% confidence, 2*sigma
    // for 95% confidence intervals.
    //
    using T = typename C::value_type; //Internal type, like double or integer.
    if(in.empty()){
        if(std::numeric_limits<T>::has_quiet_NaN){
            return std::numeric_limits<T>::quiet_NaN();
        }else{
            FUNCERR("Cannot compute variance of zero elements and cannot emit NaN. Cannot continue");
        }
    }else if(in.size() == 1){
        if(std::numeric_limits<T>::has_infinity){
            return std::numeric_limits<T>::infinity();
        }else{
            FUNCERR("Cannot compute variance of single element and cannot emit inf. Cannot continue");
        }
    }
    //if(in.size() <= 5) FUNCWARN("Very few points were used to estimate variance. Be weary of result");
    
    const T N = static_cast<T>(in.size());
    return (svl::Sum_Squares(in) - std::pow(svl::Sum(in),2)/N) / (N - (T)(1));
}
#ifndef YGORSTATS_DISABLE_ALL_SPECIALIZATIONS
template double   svl::Unbiased_Var_Est(std::list<double>     in);
template double   svl::Unbiased_Var_Est(std::vector<double>   in);

template float    svl::Unbiased_Var_Est(std::list<float >     in);
template float    svl::Unbiased_Var_Est(std::vector<float >   in);

template uint64_t svl::Unbiased_Var_Est(std::list<uint64_t>   in);
template uint64_t svl::Unbiased_Var_Est(std::vector<uint64_t> in);

template int64_t  svl::Unbiased_Var_Est(std::list<int64_t>    in);
template int64_t  svl::Unbiased_Var_Est(std::vector<int64_t>  in);
#endif

//----------------------------------------- Running Accumulators and Tallies ----------------------------------------------
template <typename T>
svl::Running_MinMax<T>::Running_MinMax() : PresentMin(std::numeric_limits<T>::max()),
PresentMax(std::numeric_limits<T>::min()) { };
#ifndef YGORSTATS_DISABLE_ALL_SPECIALIZATIONS
template svl::Running_MinMax<double  >::Running_MinMax();
template svl::Running_MinMax<float   >::Running_MinMax();
template svl::Running_MinMax<uint64_t>::Running_MinMax();
template svl::Running_MinMax<int64_t >::Running_MinMax();
#endif

template <typename T>
void svl::Running_MinMax<T>::Digest(T in){
    //Be aware of sending only NaN's and +-infs through this routine.
    this->PresentMin = std::min(this->PresentMin, in);
    this->PresentMax = std::max(this->PresentMax, in);
    return;
}
#ifndef YGORSTATS_DISABLE_ALL_SPECIALIZATIONS
template void svl::Running_MinMax<double  >::Digest(double   in);
template void svl::Running_MinMax<float   >::Digest(float    in);
template void svl::Running_MinMax<uint64_t>::Digest(uint64_t in);
template void svl::Running_MinMax<int64_t >::Digest(int64_t  in);
#endif

template <typename T>
T svl::Running_MinMax<T>::Current_Min(void) const {
    if(this->PresentMin > this->PresentMax){
        throw std::runtime_error("Not enough (finite?) data digested to provide min/max");
    }
    return this->PresentMin;
}
#ifndef YGORSTATS_DISABLE_ALL_SPECIALIZATIONS
template double   svl::Running_MinMax<double  >::Current_Min(void) const;
template float    svl::Running_MinMax<float   >::Current_Min(void) const;
template uint64_t svl::Running_MinMax<uint64_t>::Current_Min(void) const;
template int64_t  svl::Running_MinMax<int64_t >::Current_Min(void) const;
#endif

template <typename T>
T svl::Running_MinMax<T>::Current_Max(void) const {
    if(this->PresentMin > this->PresentMax){
        throw std::runtime_error("Not enough (finite?) data digested to provide min/max");
    }
    return this->PresentMax;
}
#ifndef YGORSTATS_DISABLE_ALL_SPECIALIZATIONS
template double   svl::Running_MinMax<double  >::Current_Max(void) const;
template float    svl::Running_MinMax<float   >::Current_Max(void) const;
template uint64_t svl::Running_MinMax<uint64_t>::Current_Max(void) const;
template int64_t  svl::Running_MinMax<int64_t >::Current_Max(void) const;
#endif


//--------------------------------------------- P-value (and related) routines ----------------------------------------------
double svl::P_From_StudT_1Tail(double tval, double dof){
    //This routine is applicable to any Student's t-test.
    //
    // As a reminder, it computes the "P-value" which is the probability of randomly observing a t > tval.
    const double y = std::sqrt(std::pow(tval,2) + dof);
    const double x = 0.5*(y - tval)/y;
    const double a = dof/2.0;
    const double b = dof/2.0;
    
    if(!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(x)){
        FUNCWARN("Passed parameters for which the p-value cannot be computed. Returning a quiet NaN");
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    //Regularized beta ratio function (I_x(a,b) = B_x(a,b)/B(a,b)).
    //
    //The "let it throw if an issue is encountered" approach.
    //    const double reg_beta_incom = gsl_sf_beta_inc(a,b,x);
    //    return reg_beta_incom;
    
    //The "catch the error and report the issue through floating-point channel" approach.
    gsl_set_error_handler_off(); //Disable the annoying default assert()-like error handler. NOTE: possibly non-thread safe!!!
    gsl_sf_result res;
    const auto status = gsl_sf_beta_inc_e(a,b,x,&res);
    gsl_set_error_handler(nullptr);  //reinstate the default assert()-like error handler.
    if(status){ //Issue encountered.
                //        const std::string errmsg( gsl_strerror(status) );
                //        FUNCWARN("GNU GSL beta function 'gsl_sf_beta_inc_e' encountered an error: '" << errmsg << "'."
                //              << "a, b, x = " << a << ", " << b << ", " << x << " . Reporting quiet NaN p-value");
        return std::numeric_limits<double>::quiet_NaN();
    }
    return res.val;
}
double svl::P_From_StudT_2Tail(double tval, double dof){
    //This routine is applicable to any Student's t-test.
    //
    // As a reminder, it computes the "P-value" which is the probability of randomly observing a |t| > |tval|.
    if(tval >= 0.0) return 2.0*svl::P_From_StudT_1Tail(tval,dof);
    return 2.0*svl::P_From_StudT_1Tail(-tval,dof);
}


double svl::P_From_Zscore_2Tail(double zscore){
    //This routine computes a P-value from any z-score.
    //
    // NOTE: The z-score is just the x-axis on a standard (normalized) Gaussian distribution with sigma=1 centered
    //       at the origin. It can be positive or negative. The P-value is related to the area under the Gaussian
    //       from -inf to the provided z (one-tailed) or from +-inf to the provided z, depending on which is needed.
    //
    // NOTE: The std::erf() is not a straight integral of a Gaussian. It has a strange factor of 2 so that
    //       std::erf(inf) = 1 not 0.5. That is why it is scaled by 0.5, in case you're confused.
    //
    // NOTE: The z-score is more precise than a P-value, as the z-score is usually only generated when the exact
    //       distribution is known. However, one can convert both to and from a P-value, and compute a z-score
    //       estimate in a variety of ways. Providing a z-score (and how many tails, and other relevant metadata)
    //       is equivalent to providing a P-value.
    //
    auto p_value = 2.0*svl::P_From_Zscore_Upper_Tail(std::abs(zscore));
    return p_value;
}
double svl::P_From_Zscore_Upper_Tail(double zscore){
    auto p_value = 0.5*(1.0 - std::erf(zscore/std::sqrt(2.0)));
    return p_value;
}
double svl::P_From_Zscore_Lower_Tail(double zscore){
    auto p_value = 1.0 - svl::P_From_Zscore_Upper_Tail(zscore);
    return p_value;
}


double svl::P_From_Paired_Ttest_2Tail(const std::vector<std::array<double,2>> &paired_datum, double dof_override){
    //Paired t-test for comparing the mean *difference* between paired datum. Useful for comparing two
    // distributions in which datum measure the same thing, e.g., typically measuring 'before' and 'after'.
    // Effectively, this routine compares bins on two bar plots and looks at the average difference in height
    // over the entire plot.
    //
    // NOTE: There is no *need* to normalize the data. They can have different sums -- this is OK. The mean
    //       difference is in fact what is being computed here.
    //
    // NOTE: You can override the degrees of freedom if you have altered the data. For example, fitting a function
    //       with two free parameters would decrease DOF by two. Normalizing the data by multiplying each set of
    //       numbers by a factor (e.g., setting the self-integral to one) would decrease DOF by one.
    //
    //       The default DOF = (paired.datum.size() - 1). A negative DOF implies to use the default.
    //
    // NOTE: You can further compute uncertainty estimates for the mean difference as:
    //           (mean difference) +- (T) * (sterr)
    //       where (sterr) is std::sqrt(var/N_f) as below and T is a percentile of the t-distribution with
    //       (dof) degrees of freedom. For example, for 95% confidence intervals, T would be the 2.5% point
    //       on the t-distribution.
    //
    const auto N = paired_datum.size();
    const auto N_f = static_cast<double>(N);
    if(N < 4){
        FUNCWARN("Too few datum to provide meaningful statistics");
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    //Compute the signed difference between paired elements.
    std::vector<double> diffs(N);
    for(auto i = 0; i < N; ++i) diffs[i] = paired_datum[i][1] - paired_datum[i][0];
    
    const auto mean  = svl::Mean(diffs); //Mean difference.
    const auto var   = svl::Unbiased_Var_Est(diffs); //Variance of differences.
    const auto sterr = std::sqrt(var/N_f); //Standard error of the mean difference.
    
    const auto t_stat = mean / sterr;
    const auto default_dof = N_f - 1.0;
    const auto dof = (dof_override < 0) ? default_dof : dof_override;
    
    return svl::P_From_StudT_2Tail(t_stat,dof);
}

double svl::P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(const std::vector<std::array<double,2>> &paired_datum){
    //Paired Wilcoxon signed-rank test for assessing whether population mean ranks differ between the two
    // samples. At the moment, P-values are only possible if there are ten or more unpruned points.
    //
    // NOTE: This is not the same as the "Wilcoxon rank-sum test."
    //
    // NOTE: To get the one-tail P-value, you might be able to just divide by two. However, you *must* check the
    //       sign of the W stat in order to determine which P-value to compute. There are two types:
    //         1. from -inf to W, or
    //         2. from W to +inf.
    //       Obviously these depend on the SIGN of W, whereas the two-tailed case only computes |W| to +inf.
    //       Whatever number you need, be careful to figure out the proper way to compute it.
    //
    // NOTE: This test is a non-parametric or "distribution free" test because it does NOT require normality.
    //
    
    //We pack some numbers into a working array for later. Pairs with identical values are pruned.
    std::vector<std::array<double,2>> working;
    working.reserve( paired_datum.size() );
    for(const auto &pair : paired_datum){
        const auto diff = pair[0] - pair[1];
        const auto absdiff = std::abs(diff);
        
        if(absdiff != 0){
            working.push_back({ absdiff, std::copysign(1.0, diff) });
        }
    }
    
    //Verify there is still enough data to perform a meaningful comparison.
    const auto N_red = working.size();
    if(N_red < 6){  // <--- equiv to (N_red*(N_red + 1))/2 <= 20.
        FUNCWARN("Too few datum remaining after pruning identically-valued pairs to provide meaningful statistics");
        return std::numeric_limits<double>::quiet_NaN();
        //In this case, there is no arrangement of the data such that a significant finding could result. The test
        // is simply not fed enough data if there are less than 5 remaining datum!
        //
        //NOTE: This does NOT mean it is impossible to compute the score and P-value; just that it hasn't yet been
        //      implemented (because it is tricky and it's unclear if it's really safe or not).
    }
    
    //Order the remaining data by magnitude, least to greatest.
    const auto order_by_absdiff_magnitude = [](const std::array<double,2> &L, const std::array<double,2> &R) -> bool {
        return L[0] < R[0];
    };
    std::sort(working.begin(), working.end(), order_by_absdiff_magnitude);
    
    //Rank the data on absdiff starting from one. Identically-valued data share an average rank.
    double W_pos_shtl = 0.0;   //Sum of ranks for positive-signed elements.
    double W_neg_shtl = 0.0;   //Sum of ranks for negitive-signed elements.
    long int N_tied_ranks = 0; //Number of tied ranks (+1 for each element, so +2 if rank is shared by two elements).
    for(auto i = 0; i < N_red;  ){
        auto j = i+1;
        for(  ; j < N_red; ++j){
            if(working[i][0] != working[j][0]) break;
        }
        const auto dup_elems = j - i; //Will be at least 1 for single item.
        if(dup_elems > 1) N_tied_ranks += dup_elems;
        
        const double start_rank = static_cast<double>(i+1); //One-based rank.
        const double end_rank   = static_cast<double>(dup_elems - 1) + start_rank;
        const double avg_rank   = 0.5*start_rank + 0.5*end_rank;
        for(auto k = 0; k < dup_elems; ++k){
            const auto elems_sign = working[k+i][1];
            if(elems_sign > 0){
                W_pos_shtl += avg_rank;
            }else{
                W_neg_shtl += avg_rank;
            }
        }
        i += dup_elems;
    }
    
    //Ensure the computation was OK.
    //if((W_pos_shtl + W_neg_shtl) != 0.5*N_red*(N_red + 1)) FUNCERR("Programming error");
    
    //Now finish computing the statistic.
    const auto W = std::min(W_pos_shtl,W_neg_shtl); //Should this be ::min or ::max? (Doesn't seem to matter.)
    
    //Since this is an exact test, compute the z-score and use it to generate the p-value.
    // (W is asymptotically normally distributed.)
    const auto N_red_f   = static_cast<double>(N_red);
    const auto N_tied    = static_cast<double>(N_tied_ranks);
    
    const auto pop_var   = N_red_f * (N_red_f + 1.0) * (2.0*N_red_f + 1.0) / 24.0;
    const auto pop_dvar  = (1.0/48.0)*(std::pow(N_tied,3) - N_tied);
    const auto pop_sigma = std::sqrt(pop_var - pop_dvar); //Std. dev. of pop. mean.
    const auto pop_mean  = 0.25*N_red_f*(N_red_f+1.0);
    
    const auto z_score   = svl::Z_From_Observed_Mean(W,pop_mean,pop_sigma);
    const auto p_value   = svl::P_From_Zscore_2Tail(z_score);
    
    //Note that the z-score and W itself are of possible interest to the caller. Especially W when N_red < 10
    // because it will be needed to determine significance (because there is not enough data for the z-score).
    //
    // Consider making this data available as optional pointers in the function signature before returning a
    // big struct or tuple.
    return p_value;
}


double svl::P_From_StudT_Diff_Means_From_Uneq_Vars(double M1, double V1, double N1, double M2, double V2, double N2){
    // "Two-sample unpooled t-test for unequal variances" or "Welch's t-test".
    //
    // Given two means (M1 and M2) with associated (possibly non-equal) variances, compute the significance
    // that the means are different. In other words, given the means, what is the probability that they
    // are drawn from the same population? Are the means significantly different?
    //
    //Input:
    //  - M1 and M2 - The means of either distribution. I don't believe this routine is applicable to
    //                other quantities. See numerical recipes for a nice discussion.
    //  - V1 and V2 - The variance (ie. sigma^2 == std.dev.^2) of either distribution. This is the square
    //                of the 'std dev of the data', and NOT the square of the 'std dev of the mean (aka
    //                the std err)'. The difference is that:
    //                    (std dev of the mean) = (std dev of the data)/sqrt(N).
    //  - N1 and N2 - The number of points in either distribution. *NOT* the DOF!
    //
    //This routine is often used to determine if some quantity has changed significantly. Examples could be
    // a quantity changing at two points in time, or comparison of non-linear fitting parameters obtained
    // with two different techniques (ie. least-sum of squares vs. least-median of squares).
    //
    //Logically, it is similar to the area of overlap of the two distributions. Obviously, if the
    // distributions have wildly different means or variances, this routine may not appropriate.
    //
    //NOTE: This tests significance in the difference of means. Do not conflate significance of means
    //      with significance in difference of the distributions themselves!
    //
    //NOTE: If you are finding means using a bootstrap, use the original number of datum, NOT the bootstrap
    //      count. Although the estimates are derived using the bootstrap, the inherent entropy for the
    //      estimates comes from the original datum counts.
    //
    //NOTE: Confused about the output? If (M1 == M2) && (V1 == V2) && (N1 == N2) then P = 1. In words, if
    //      the means and variances are the same, this statistic thinks there is 100% chance they are derived
    //      from the same distribution. (In reality, it might just be coincidental.)
    //      If the numbers are very far away, P = 0. There is then no chance they are derived from the same
    //      distribution.
    //
    //NOTE: Do NOT use this if the variances are significantly/substantially different. The underlying
    //      distributions may not be even remotely similar, and this routine will under-estimate the P-value.
    //
    //NOTE: If the variances are identical, this ~becomes the http://en.wikipedia.org/wiki/Student%27s_t-test
    //      (except for the dof estimate, I think!) The P values could potentially be compared in such case.
    //      Wikipedia suggests a t-test would be more precise in such a case, but I haven't investigated.
    //
    //NOTE: Want to determine whether the variances V1 and V2 are actually different? Use an F-test for that.
    //      Consult Numerical Recipes for info.
    //
    //NOTE: Want to determine whether two whole distributions are different? Use a Chi-Square or Kolmogorov-
    //      Smirnov test. See Numerical Recipes for info.
    //
    //NOTE: From: "The unequal variance t-test is an underused alternative to Student's t-test and the
    //      Mann-Whitney U test" by GD Ruxton in Behavioral Ecology (2006):
    //          "If you want to compare the central tendency of 2 populations
    //          based on samples of unrelated data, then the unequal variance
    //          t-test should always be used in preference to the Student’s
    //          t-test or Mann–Whitney U test."
    //      Conclusion: prefer this test to Mann-Whitney or Student's t, but be weary of non-normal data.
    //
    if((N1 < 2.0) || (N2 < 2.0)) FUNCERR("Not enough points available for computation");
    
    const double t_num = M1 - M2;
    const double t_den = std::sqrt((V1/N1)+(V2/N2));
    if(!std::isnormal(t_den)) FUNCERR("Encountered difficulty computing Student's t-value. Cannot continue");
    
    const double dof_num = std::pow((V1/N1)+(V2/N2),2.0);
    const double dof_den = ((V1*V1)/(N1*N1*(N1-1.0))) + ((V2*V2)/(N2*N2*(N2-1.0)));
    if(!std::isnormal(dof_den)) FUNCERR("Encountered difficulty computing dof. Cannot continue");
    
    return svl::P_From_StudT_2Tail(t_num/t_den, dof_num/dof_den);
}


double svl::P_From_Pearsons_Linear_Correlation_Coeff_1Tail(double corr_coeff, double total_num_of_datum){
    //See two-tailed variant comments.
    //
    // NOTE: Use the 2-tailed function unless there is some specific reason you know why the correlation
    //       coefficient cannot possibly extend from [-1,1].
    //
    return 0.5*svl::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(corr_coeff, total_num_of_datum);
}
double svl::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(double corr_coeff, double total_num_of_datum){
    //P-value for Pearson's r (aka "linear correlation coefficient"). This is an alternative to the t-test
    // which is thought to be more precise. The relevant integral and discussion is given by John R. Taylor
    // in Appendix C of "An Introduction to Error Analysis", 2nd Ed..
    //
    // We compute:
    //     P(|r| >= |ro|) = (2/sqrt(pi))                                             "factor a"
    //                      * (Gamma(0.5*(N-1))/Gamma(0.5*N-1))                      "factor b"
    //                      * (integral from |ro| to 1 of: pow(1-r*r, 0.5*N-2))      "factor c"
    //
    // where |ro| is the correlation coefficient provided by the user. The integral is evaluated indefinately
    // as [r*(2F1)(0.5, 2-0.5*N, 1.5, r*r) which is the "2F1" Gauss Hypergeometric function.
    // Now, the GNU GSL (2F1) doesn't handle the r*r = 1 case, so we use an identity for it instead.
    //
    // Unfortunately, the GNU GSL implementation appears to be incomplete so I've decided to just try
    // numerical integration. This is probably good enough for a p-value, which really doesn't need much
    // precision beyond a few leading terms. And the integrand is not bad when N > 5 when it will be of
    // most use.
    double N = total_num_of_datum;
    const double r = std::abs(corr_coeff);
    const double pval_factora = 2.0/std::sqrt(M_PI);
    const double pval_factorb = std::exp(std::lgamma(0.5*N-0.5) - std::lgamma(0.5*N-1.0));
    
    //GNU GSL fails here fairly often. I've also tried catching GSL errors to no avail. Try some random numbers.
    //const double pval_factorc = gsl_sf_hyperg_2F1(0.5, 2.0-0.5*N, 1.5, 1.0)
    //                            -r*gsl_sf_hyperg_2F1(0.5, 2.0-0.5*N, 1.5, r*r);
    //
    //GNU GSL does no better here. The hyperg call still fails badly.
    //const double pval_factorc = (std::tgamma(1.5)/std::tgamma(1.0))
    //                            *std::exp(std::lgamma(0.5*N-1.0) - std::lgamma(0.5*N-0.5))
    //                            -r*gsl_sf_hyperg_2F1(0.5, 2.0-0.5*N, 1.5, r*r);
    //
    //Back-up solution: perform numerical integration. Ideally, numerical integration should be eliminated by
    // implementing a suitable 2F1 hypergeometric function routine.
    double pval_factorc;
    {
        auto integrand = [](double x, void *params){
            const auto N = reinterpret_cast<double*>(params);
            if(N == nullptr) FUNCERR("Unexpected behaviour. This is probably a programming error and should never happen");
            return std::pow(1.0 - x*x, 0.5*(*N) - 2.0);
        };
        
        gsl_integration_workspace *w = gsl_integration_workspace_alloc(5000);
        double result, error;
        gsl_function F;
        //F.function = &P_From_Pearsons_Linear_Correlation_Coeff_1Tail_integrand;
        F.function = integrand;
        F.params = static_cast<void*>(&N);
        
        /*const int status =*/
        gsl_integration_qags(&F, r, 1.0, 0.0, 1e-4, 5000, w, &result, &error);
        if(w->size >= 5000){
            FUNCERR("GNU GSL failed to numerically integrate. Cannot continue");
        }
        pval_factorc = result;
        gsl_integration_workspace_free(w);
    }
    
    return pval_factora * pval_factorb * pval_factorc;
}


double svl::Q_From_ChiSq_Fit(double chi_square, double dof){
    //See Numerical Recipes, C, Section 6.2 (pp 171 or 221) or section on nonlinear fitting.
    //
    //NOTE: This function should use the chi-square which includes uncertainties.
    //NOTE: This function should not use the reduced chi-square!
    //
    // A ~decent fit has Q > 0.001 (although a little low).
    // If Q is too near 1.0, the errors are probably overestimated.
    // If Q is a little lower then 0.001, errors are probably underestimated.
    // If Q is very low (<1E-6) the model is a terrible fit.
    return gsl_sf_gamma_inc_Q(dof/2.0, chi_square/2.0);
}

//-----------------------------------------------------------------------------------------------------------
//--------------------------------------- Z-test for observed mean ------------------------------------------
//-----------------------------------------------------------------------------------------------------------
double svl::Z_From_Observed_Mean(double sample_mean, //Or observed mean.
                                   double population_mean, //Or true mean.
                                   double population_stddev, //Pop. std. dev. of the distribution (NOT mean!)
                                   double sample_size){ //Number of datum used to compute sample_mean.
                                                        //This routine computes the z-score for an observed ("sampled") mean when you already know the true
                                                        // population mean. It is rare to know the true population mean, and more often comes up when creating
                                                        // test statistics or specific algorithms with a finite (and enumerable) number of outcomes.
                                                        //
                                                        // This formula comes from the "standard normal distribution." Instead of feeding it into the Gaussian's
                                                        // exponential, you slap the name 'z-score' on it and call it a day (by definition). Easy.
                                                        //
                                                        // NOTE: You can directly compare z-scores to critical z values, or you can compute an equivalent P-value
                                                        //       and work with probability thresholds instead (e.g., 0.05, 0.01, ...). Anyways, this is the source
                                                        //       of the critical z-scores so you don't lose anything by converting.
                                                        //
    return (sample_mean - population_mean)/(population_stddev/std::sqrt(sample_size));
}

double svl::Z_From_Observed_Mean(double sample_mean, //Or observed mean.
                                   double population_mean, //Or true mean.
                                   double stddev_of_pop_mean){ //Population std. err.
                                                               //See notes in the companion function. The only difference in this case is that we are provided the std.
                                                               // dev. of the population mean instead of the std. dev. of the population's distribution. This amounts to
                                                               // skipping the reduction of population_stddev by std::sqrt(sample_size);
    return (sample_mean - population_mean)/stddev_of_pop_mean;
}


