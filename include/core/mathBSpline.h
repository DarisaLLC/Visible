

#ifndef  _MATH_BSPLINE_H_
#define  _MATH_BSPLINE_H_

#include <gsl/gsl_bspline.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_vector.h>
#include <stddef.h>
#include <array>
#include <cmath>
#include <complex>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
using namespace svl;


//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------- basis_spline: B-Splines built on samples_1D --------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class provides a means to transform a samples_1D into a Basis Spline of aritrary order. It is a smoother.
// This class is built on the (GNU) GSL B-spline implementation. 
//
// Currently, only basic support for data uncertainties is provided.
//
// NOTE: Specification of a B-spline requires linear least-squares fitting. Keep in mind that this procedure 
//       can fail or (worse) return nonsensical results.
//
// NOTE: The following parameters required by the constructor specify the B-spline "character":
//       
//       "k" -- the 'order' of the basis spline = 'k'. You choose it based on the smoothness requirements.
//          Generally, k=4 is sufficient for most purposes. But more or fewer have niche applications:
//       
//          k=1 ==> y ~ A                       ==> Flat line segments (i.e., parallel to y=0).
//          k=2 ==> y ~ B*x + A                 ==> Line segments, no continuous derivatives.
//          k=3 ==> y ~ C*x^2 + B*x + A         ==> Curved segments, only first derivative is continuous.
//          k=4 ==> y ~ D*x^3 + C*x^2 + B*x + A ==> Curvier segments, only 1st and 2nd deriv's continuous.
//
//       "ncoeffs" -- the number of coefficients to fit for each segment. Keep low to reduce overfitting.
//
//          If there is no noise in the data, then you can keep increasing the number of coefficients quite high without
//          consequence. If there IS noise in the data, then avoid setting this too high. For example, if there are N
//          datum, then N coefficients is DEFINITELY too many. You have to inspect the fit quality (and reduced
//          chi-square, especially if you have error estimates). Here are some ballpark numbers you can use for the
//          first fit:
//
//          # of datum ~ 10  ==>   4-8
//          # of datum ~ 30  ==>  10-15
//          # of datum ~ 50  ==>  10-25
//          # of datum ~100  ==>  20-45
//          # of datum >100  ==> ~25-50 (or more, but be very careful about overfitting).
//
//          Of course, the appropriate number depends on how noisy the data is, how curvy the underlying signal is, the
//          sampling details, what level of detail you want to extract (e.g., low-frequency --> use fewer coefficients),
//          the sparsity of the data, the relative reliabilty of the data (i.e., local confidence), the nature of the
//          errors (i.e., are they Gaussian? They better be!) and the linear fitting procedure you use (n.b. linear
//          least squares, which assumes uncorrelated Gaussian noise), and a variety of other factors.
//
//          In practice, you'll have to test out a few settings and make a decision on what is 'appropriate enough' for
//          the particular application.
//
//       "nbreak" -- the number of breaks. The basis splines are split piecewise. This number helps control where those
//          breaks are. 
//
//          This parameter is currently automatically computed. It is affected by k and ncoeffs.
//       
//          Currently the breaks are evenly distributed across the provided domain. It is possible to instead choose 
//          the breakpoint locations specifically. This would be helpful for data with abrupt peaks or other sharp 
//          features because then the approximation would more closely conform to that segment. The question is how to
//          reliably/robustly detect such features in generality. I did not look at such schemes because it didn't seem
//          warranted at the time of writing.
//
// NOTE: The impetus for creating this class was to improve Chebyshev polynomial approximation with noisy data.
//       The use of default interpolation (i.e., linear interpolation) can cause the approximating Chebyshev
//       polynomial to go wild trying to approximate the (artificial) sharp features. This routine was selected
//       because it can guarantee smoothness and also smoothness of higher derivatives.
//       
//       However, the implementation is not strongly coupled to the Chebyshev polynomial approximation class.
//       You'll have to supply this class' instance to be evaluated at the Chebyshev node locations via wrapping
//       in a closure.
//

enum basis_spline_breakpoints {
    equal_spacing,              // Equal spacing between breakpoints. (Not good if a bucket has no data!)
    adaptive_datum_density      // Adaptive breakpoint placement: equal number of points in each bucket.
};

class basis_spline {
    private:

        double xmin = std::numeric_limits<double>::quiet_NaN();   // The domain of the datum/samples.
        double xmax = std::numeric_limits<double>::quiet_NaN();   // 
        size_t datum = 0;                                         // The number of datum/samples used.

        double chisq = std::numeric_limits<double>::quiet_NaN();  // Chi-square for coefficient fits.
        double dof   = std::numeric_limits<double>::quiet_NaN();  // Degrees of freedom for coefficient fits.
        double Rsq   = std::numeric_limits<double>::quiet_NaN();  // Weighted coefficient of determination.

        gsl_bspline_workspace *bw = nullptr;
        gsl_vector *B = nullptr;
        gsl_vector *c = nullptr;
        gsl_vector *w = nullptr;
        gsl_vector *x = nullptr;
        gsl_vector *y = nullptr;
        gsl_matrix *X = nullptr;
        gsl_matrix *cov = nullptr;
        gsl_multifit_linear_workspace *mw = nullptr;

    public:
        // Constructors.
        basis_spline(const samples_1D<double> &data, 
                     double xmin = std::numeric_limits<double>::quiet_NaN(),
                     double xmax = std::numeric_limits<double>::quiet_NaN(),
                     size_t k = 4,
                     size_t ncoeffs = 10,
                     basis_spline_breakpoints = basis_spline_breakpoints::adaptive_datum_density );
        ~basis_spline();
   
        // Operators.
        std::array<double,4> Sample(double x) const;

};

#endif
