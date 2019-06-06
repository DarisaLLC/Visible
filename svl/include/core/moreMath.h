//YgorMath.h

#ifndef YGOR_MATH_H_
#define YGOR_MATH_H_
#include <stddef.h>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
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

namespace svl {

template <class T> class samples_1D;

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------- vec_3: A three-dimensional vector -------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class is the basis of the geometry-heavy classes below. It can happily be used on its own.
template <class T> class vec_3 {
    public:
        using value_type = T;
        T x, y, z;
    
        //Constructors.
        vec_3();
        vec_3(T a, T b, T c);
        vec_3( const vec_3 & );
    
        //Operators - vec_3 typed.
        vec_3 & operator=(const vec_3 &);
    
        vec_3   operator+(const vec_3 &) const;
        vec_3 & operator+=(const vec_3 &);
        vec_3   operator-(const vec_3 &) const;
        vec_3 & operator-=(const vec_3 &);
    
        bool operator==(const vec_3 &) const;
        bool operator!=(const vec_3 &) const;
        bool operator<(const vec_3 &) const;   //This is mostly for sorting / placing into a std::map. There is no great
                                              // way to define '<' for a vector, so be careful with it.
        bool isfinite(void) const;        //Logical AND of std::isfinite() on each coordinate.

        vec_3   operator*(const T &) const;
        vec_3 & operator*=(const T &);
        vec_3   operator/(const T &) const;
        vec_3 & operator/=(const T &);

        //Operators - T typed.
        T & operator[](size_t);
    
        //Member functions.
        T Dot(const vec_3 &) const;        // ---> Dot product.
        vec_3 Cross(const vec_3 &) const;   // ---> Cross product.
        vec_3 Mask(const vec_3 &) const;    // ---> Term-by-term product:   (a b c).Outer(1 2 0) = (a 2*b 0)
    
        vec_3 unit() const;                // ---> Return a normalized version of this vector.
        T length() const;                 // ---> (pythagorean) length of vector.
        T distance(const vec_3 &) const;   // ---> (pythagorean) distance between vectors.
        T sq_dist(const vec_3 &) const;    // ---> Square of the (pythagorean) distance. Avoids a sqrt().
        T angle(const vec_3 &, bool *OK=nullptr) const;  // ---> The |angle| (in radians, [0:pi]) separating two vectors.

        vec_3 zero(void) const; //Returns a zero-vector.

        vec_3<T> rotate_around_x(T angle_rad) const; // Rotate by some angle (in radians, [0:pi]) around a cardinal axis.
        vec_3<T> rotate_around_y(T angle_rad) const;
        vec_3<T> rotate_around_z(T angle_rad) const;

        bool GramSchmidt_orthogonalize(vec_3<T> &, vec_3<T> &) const; //Using *this as seed, orthogonalize (n.b. not orthonormalize) the inputs.

        std::string to_string(void) const;
       // vec_3<T> from_string(const std::string &in); //Sets *this and returns a copy.
    
        //Friends.
        template<class Y> friend std::ostream & operator << (std::ostream &, const vec_3<Y> &); // ---> Overloaded stream operators.
     //   template<class Y> friend std::istream & operator >> (std::istream &, vec_3<Y> &);
};


//This is a function for rotation unit vectors in some plane. It requires angles to describe the plane of rotation, angle of rotation. 
// It also requires a unit vector with which to rotate the plane about.
//
// Note: Prefer Gram-Schmidt orthogonalization or the cross-product if you can. This routine has some unfortunate poles...
vec_3<double> rotate_unit_vector_in_plane(const vec_3<double> &A, const double &theta, const double &R);

//This function evolves a pair of position and velocity (x(t=0),v(t=0)) to a pair (x(t=T),v(t=T)) using the
// classical expression for a time- and position-dependent force F(x;t). It is highly unstable, so the number
// of iterations must be specified. If this is going to be used for anything important, make sure that the
// number of iterations is chosen sufficiently high so as to produce negligible errors.
std::tuple<vec_3<double>,vec_3<double>> Evolve_x_v_over_T_via_F(const std::tuple<vec_3<double>,vec_3<double>> &x_and_v,
                                                              std::function<vec_3<double>(vec_3<double> x, double T)> F,
                                                              double T, long int steps);


//---------------------------------------------------------------------------------------------------------------------------
//----------------------------------------- vec_2: A two-dimensional vector --------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class is the basis of the geometry-heavy classes below. It can happily be used on its own.
template <class T> class vec_2 {
    public:
        using value_type = T;
        T x, y;

        //Constructors.
        vec_2();
        vec_2(T a, T b);
        vec_2( const vec_2 & );

        //Operators - vec_2 typed.
        vec_2 & operator=(const vec_2 &);

        vec_2   operator+(const vec_2 &) const;
        vec_2 & operator+=(const vec_2 &);
        vec_2   operator-(const vec_2 &) const;
        vec_2 & operator-=(const vec_2 &);

        bool operator==(const vec_2 &) const;
        bool operator!=(const vec_2 &) const;
        bool operator<(const vec_2 &) const;   //This is mostly for sorting / placing into a std::map. There is no great
                                              // way to define '<' for a vector, so be careful with it.
        bool isfinite(void) const;        //Logical AND of std::isfinite() on each coordinate.

        vec_2   operator*(const T &) const;
        vec_2 & operator*=(const T &);
        vec_2   operator/(const T &) const;
        vec_2 & operator/=(const T &);

        //Operators - T typed.
        T & operator[](size_t);

        //Member functions.
        T Dot(const vec_2 &) const;        // ---> Dot product.
        vec_2 Mask(const vec_2 &) const;    // ---> Term-by-term product:   (a b).Outer(5 0) = (5*a 0*b)

        vec_2 unit() const;                // ---> Return a normalized version of this vector.
        T length() const;                 // ---> (pythagorean) length of vector.
        T distance(const vec_2 &) const;   // ---> (pythagorean) distance between vectors.
        T sq_dist(const vec_2 &) const;    // ---> Square of the (pythagorean) distance. Avoids a sqrt().

        vec_2 zero(void) const; //Returns a zero-vector.

        vec_2<T> rotate_around_z(T angle_rad) const; // Rotate by some angle (in radians, [0:pi]) around a cardinal axis.

        std::string to_string(void) const;
     //   vec_2<T> from_string(const std::string &in); //Sets *this and returns a copy.

        //Friends.
        template<class Y> friend std::ostream & operator << (std::ostream &, const vec_2<Y> &); // ---> Overloaded stream operators.
//        template<class Y> friend std::istream & operator >> (std::istream &, vec_2<Y> &);
};


template <class T> class samples_1D {
    public:
        //--------------------------------------------------- Data members -------------------------------------------------
        //Samples are laid out like [x_i, sigma_x_i, f_i, sigma_f_i] sequentially, so if there are three samples data will be
        // laid out in memory like:
        //   [x_0, sigma_x_0, f_0, sigma_f_0, x_1, sigma_x_1, f_1, sigma_f_1, x_2, sigma_x_2, f_2, sigma_f_2 ].
        // and can thus be used as a c-style array if needed. If you can pass stride and offset (e.g., to GNU GSL), use:
        //   - for x_i:        stride = 4,  offset from this->samples.data() to first element = 0. 
        //   - for sigma_x_i:  stride = 4,  offset from this->samples.data() to first element = 1. 
        //   - for f_i:        stride = 4,  offset from this->samples.data() to first element = 2. 
        //   - for sigma_f_i:  stride = 4,  offset from this->samples.data() to first element = 3. 
        std::vector<std::array<T,4>> samples;

        //This flag controls how uncertainties are treated. By default, no assumptions about normality are made. If you are
        // certain that the sample uncertainties are: independent, random, and normally-distributed (i.e., no systematic 
        // biases are thought to be present, and the underlying measurement process follows a normal distribution) then this
        // flag can be set to reduce undertainties in accordance with the knowledge that arithmetical combinations of values 
        // tend to cancel uncertainties. Be CERTAIN to actually inspect this fact!
        //
        // Conversely, setting this flag may under- or over-value uncertainties because we completely ignore covariance.
        // If it is false, you at least know for certain that you are over-valuing uncertainties. Be aware that BOTH 
        // settings imply that the uncertainties are considered small enough that a linear error propagation procedure 
        // remains approximately valid!
        bool uncertainties_known_to_be_independent_and_random = false;        


        //User-defined metadata, useful for packing along extra information about the data. Be aware that the data can
        // become stale if you are not updating it as needed!
        std::map<std::string,std::string> metadata;


        //-------------------------------------------------- Constructors --------------------------------------------------
        samples_1D();
        samples_1D(const samples_1D<T> &in);
        samples_1D(std::vector<std::array<T,4>> samps);

        //Providing [x_i, f_i] data. Assumes sigma_x_i and sigma_f_i uncertainties are (T)(0).
        samples_1D(const std::list<vec_2<T>> &samps);

        //------------------------------------------------ Member Functions ------------------------------------------------
        samples_1D<T> operator=(const samples_1D<T> &rhs);

        //If not supplied, sigma_x_i and sigma_f_i uncertainties are assumed to be (T)(0).
        void push_back(T x_i, T sigma_x_i, T f_i, T sigma_f_i, bool inhibit_sort = false);
        void push_back(const std::array<T,4> &samp, bool inhibit_sort = false);
        void push_back(const std::array<T,2> &x_dx, const std::array<T,2> &y_dy, bool inhibit_sort = false);
        void push_back(T x_i, T f_i, bool inhibit_sort = false);
        void push_back(const vec_2<T> &x_i_and_f_i, bool inhibit_sort = false);
        void push_back(T x_i, T f_i, T sigma_f_i, bool inhibit_sort = false);

        bool empty(void) const;  // == this->samples.empty()
        size_t size(void) const; // == this->samples.size()

        //An explicit way for the user to sort. Not needed unless you are directly editing this->samples for some reason.
        void stable_sort(void); //Sorts on x-axis. Lowest-first.

        //Get the datum with the minimum and maximum x_i or f_i. If duplicates are found, there is no rule specifying which.
        std::pair<std::array<T,4>,std::array<T,4>> Get_Extreme_Datum_x(void) const;
        std::pair<std::array<T,4>,std::array<T,4>> Get_Extreme_Datum_y(void) const;

        //Normalizes data so that \int_{-inf}^{inf} f(x) (times) f(x) dx = 1 multiplying by constant factor.
        void Normalize_wrt_Self_Overlap(void);

        //Sets uncertainties to zero. Useful in certain situations, such as computing aggregate std dev's.
        samples_1D<T> Strip_Uncertainties_in_x(void) const;
        samples_1D<T> Strip_Uncertainties_in_y(void) const;

        //Ensure there is a single datum with the given x_i (within +-eps), averaging coincident data if necessary.
        void Average_Coincident_Data(T eps);

        //Replaces {x,y}-values with rank. {dup,N}-plicates get an averaged (maybe non-integer) rank.
        samples_1D<T> Rank_x(void) const;
        samples_1D<T> Rank_y(void) const;

        //Swaps x_i with f_i and sigma_x_i with sigma_f_i.
        samples_1D<T> Swap_x_and_y(void) const;
        
        //Sum of all x_i or f_i. Uncertainties are propagated properly.
        std::array<T,2> Sum_x(void) const;
        std::array<T,2> Sum_y(void) const;

        //Computes the [statistical mean (average), std dev of the *data*]. Be careful to choose the MEAN or AVERAGE 
        // carefully -- the uncertainty is different! See source for more info.
        std::array<T,2> Average_x(void) const; 
        std::array<T,2> Average_y(void) const;

        //Computes the [statistical mean (average), std dev of the *mean*]. Be careful to choose the MEAN or AVERAGE 
        // carefully -- the uncertainty is different! See source for more info.
        std::array<T,2> Mean_x(void) const;
        std::array<T,2> Mean_y(void) const;

        //Computes the [statistical weighted mean (average), std dev of the *weighted mean*]. See source. 
        std::array<T,2> Weighted_Mean_x(void) const;
        std::array<T,2> Weighted_Mean_y(void) const;

        //Finds or computes the median datum with linear interpolation halfway between datum if N is odd.
        std::array<T,2> Median_x(void) const;
        std::array<T,2> Median_y(void) const;

        //Select those within [L,H] (inclusive). Ignores sigma_x_i for inclusivity purposes. Keeps uncertainties intact.
        samples_1D<T> Select_Those_Within_Inc(T L, T H) const;

        //Returns [at_x, sigma_at_x (==0), f, sigma_f] interpolated at at_x. If at_x is not within the range of the 
        // samples, expect {at_x,(T)(0),(T)(0),(T)(0)}.
        std::array<T,4> Interpolate_Linearly(const T &at_x) const;

        //Returns linearly interpolated crossing-points.
     //   samples_1D<T> Crossings(T value) const;

        //Returns the locations linearly-interpolated peaks.
        samples_1D<T> Peaks(void) const;

        //Resamples the data into approximately equally-spaced samples using linear interpolation.
        samples_1D<T> Resample_Equal_Spacing(size_t N) const;

        //Binarize (threshold) the samples along x using Otsu's criteria.
        T Find_Otsu_Binarization_Threshold(void) const;

        //Multiply all sample f_i's by a given factor. Uncertainties are appropriately scaled too.
        samples_1D<T> Multiply_With(T factor) const; // i.e., "operator *".
        samples_1D<T> Sum_With(T factor) const; // i.e., "operator +".

        //Apply an absolute value functor to all f_i.
        samples_1D<T> Apply_Abs(void) const;

        //Shift all x_i's by a factor. No change to uncertainties.
        samples_1D<T> Sum_x_With(T dx) const;
        samples_1D<T> Multiply_x_With(T dx) const;
 
        //Distribution operators. Samples are retained, so linear interpolation is used.
        samples_1D<T> Sum_With(const samples_1D<T> &in) const; //i.e. "operator +".
        samples_1D<T> Subtract(const samples_1D<T> &in) const; //i.e. "operator -".

        //Purge non-finite samples.
        samples_1D<T> Purge_Nonfinite_Samples(void) const;

        //Generic driver for numerical integration. See source for more info.
        template <class Function> std::array<T,2> Integrate_Generic(const samples_1D<T> &in, Function F) const;

        //Computes the integral: \int_{-inf}^{+inf} f(x) dx and uncertainty estimate. See source.
        std::array<T,2> Integrate_Over_Kernel_unit() const;

        //Computes the integral: \int_{xmin}^{xmax} f(x) dx and uncertainty estimate. See source.
        std::array<T,2> Integrate_Over_Kernel_unit(T xmin, T xmax) const;

        //Computes integral over exp. kernel: \int_{xmin}^{xmax} f(x)*exp(A*(x+x0))dx and uncertainty estimate. See source.
        std::array<T,2> Integrate_Over_Kernel_exp(T xmin, T xmax, std::array<T,2> A, std::array<T,2> x0) const;

        //Group datum into (N) equal-dx bins, reducing variance. Ignores sigma_x_i. *Not* a histogram! See source.
        samples_1D<T> Aggregate_Equal_Sized_Bins_Weighted_Mean(long int N, bool explicitbins = false) const;

        //Group datum into equal-datum (N datum per bin) bins, reducing variance. *Not* a histogram! See source.
        samples_1D<T> Aggregate_Equal_Datum_Bins_Weighted_Mean(long int N) const; //Handles all uncertainties.

        //Generate histogram with (N) equal-dx bins. Bin height is sum of f_i in bin. Propagates sigma_f_i, ignores sigma_x_i.
        samples_1D<T> Histogram_Equal_Sized_Bins(long int N, bool explicitbins = false) const;

        //Spearman's Rank Correlation Coefficient. Ignores uncertainties. Returns: {rho, # of datum, z-value, t-value}.
        std::array<T,4> Spearmans_Rank_Correlation_Coefficient(bool *OK=nullptr) const;

        //Computes {Sxy,Sxx,Syy} which are used for linear regression and other procedures.
        std::array<T,3> Compute_Sxy_Sxx_Syy(bool *OK=nullptr) const;

      

        //Calculates the discrete derivative using forward finite differences. (The right-side endpoint uses backward 
        // finite differences to handle the boundary.) Data should be reasonably smooth -- no interpolation is used.
        samples_1D<T> Derivative_Forward_Finite_Differences(void) const;

        //Calculates the discrete derivative using backward finite differences. (The right-side endpoint uses forward 
        // finite differences to handle the boundary.) Data should be reasonably smooth -- no interpolation is used.
        samples_1D<T> Derivative_Backward_Finite_Differences(void) const;

        //Calculates the discrete derivative using centered finite differences. (The endpoints use forward and backward 
        // finite differences to handle boundaries.) Data should be reasonably smooth -- no interpolation is used.
        samples_1D<T> Derivative_Centered_Finite_Differences(void) const;

    
        //Checks if the key is present without inspecting the value.
        bool MetadataKeyPresent(std::string key) const;

        //Attempts to cast the value if present. Optional is disengaged if key is missing or cast fails.
        //template <class U> std::experimental::optional<U> GetMetadataValueAs(std::string key) const;


        //Various IO routines.
        bool Write_To_File(const std::string &filename) const; //Writes data to file as 4 columns. Use it to plot/fit.
        std::string Write_To_String(void) const; //Writes data to file as 4 columns. Use it to plot/fit.

        bool Read_From_File(const std::string &filename); //Reads data from a file as 4 columns. True iff all went OK.

        void Plot(const std::string &Title = "") const; //Spits out a default plot of the data. 
        void Plot_as_PDF(const std::string &Title = "",const std::string &Filename = "/tmp/plot.pdf") const; //May overwrite existing files!

        //----------------------------------------------------- Friends ----------------------------------------------------
        //Overloaded stream operators for reading, writing, and serializing the samples.
        template<class Y> friend std::ostream & operator << (std::ostream &, const samples_1D<Y> &);
        template<class Y> friend std::istream & operator >> (std::istream &, samples_1D<Y> &);
};



}


#endif
