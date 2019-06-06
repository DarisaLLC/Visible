//YgorMathBSpline.cc.

#include <cmath>       //Needed for fabs, signbit, sqrt, etc...
#include <iterator>
#include <limits>      //Needed for std::numeric_limits::max().
#include <memory>
#include <numeric>   //Needed for std::accumulate.
#include <stdexcept>
#include <utility>     //Needed for std::pair.
#include <vector>

#include <gsl/gsl_bspline.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_vector.h>
#include <stdlib.h>

#include "core/moreMath.h"
#include "core/mathBSpline.h"
#include "core/gen_utils.hpp"



//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------- basis_spline: B-Splines built on samples_1D --------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

basis_spline::basis_spline(const samples_1D<double> &samps, 
                           double xmin_in, 
                           double xmax_in, 
                           size_t k, 
                           size_t ncoeffs,
                           basis_spline_breakpoints breakpoints){

    int gsl_status;

    //Average datum that overlap on x_i within eps. This is needed if adaptive breakpoints are being used because they
    // will try to ensure each bucket has an equal number (non-zero) of datum.
    samples_1D<double> sampscopy(samps);
    if(breakpoints == basis_spline_breakpoints::adaptive_datum_density){
        const double eps = std::numeric_limits<double>::epsilon();
        sampscopy.Average_Coincident_Data(eps);
    }

    //Basic input sanity checking.
    if( std::isfinite(xmin_in) && std::isfinite(xmax_in)){
        if( xmin_in < xmax_in ){
            this->xmin = xmin_in;
            this->xmax = xmax_in;
        }else{ // Domain empty or user provided domain backward. Catch possible (xmin = xmax) error later.
            this->xmin = xmax_in;
            this->xmax = xmin_in;
        }
    }else{
        this->xmin = sampscopy.Get_Extreme_Datum_x().first[0];
        this->xmax = sampscopy.Get_Extreme_Datum_x().second[0];
    }
    if( this->xmin == this->xmax ){
        throw std::invalid_argument("Domain consists of a single point. Approximation not useful");
    }

    //Figure out exactly how many and which datum are applicable.
    size_t N = sampscopy.samples.size();  //NOT the number of usable datum!
    std::vector<size_t> indices;
    for(size_t i = 0; i < N; i++){
        const double x_i = sampscopy.samples[i][0];
        const double f_i = sampscopy.samples[i][2];
        if(isininc(this->xmin, x_i, this->xmax) && std::isfinite(f_i)) indices.push_back(i);
    }
    this->datum = indices.size();
    if( this->datum < 5 ){
        throw std::invalid_argument("Cannot do anything useful with < 5 datum");
    }

    //Determine how many breaks (between buckets) will be needed.
    if(static_cast<long int>(ncoeffs) - static_cast<long int>(k) + 2 <= 0){
        throw std::runtime_error("chosen number of coefficients cannot be supported by choice of order (k).");
    }
    const size_t nbreaks = ncoeffs + 2 - k;

    this->bw = gsl_bspline_alloc(k, nbreaks);
    this->B = gsl_vector_alloc(ncoeffs);

    this->x = gsl_vector_alloc(this->datum);
    this->y = gsl_vector_alloc(this->datum);
    this->X = gsl_matrix_alloc(this->datum, ncoeffs);
    this->c = gsl_vector_alloc(ncoeffs);
    this->w = gsl_vector_alloc(this->datum);
    this->cov = gsl_matrix_alloc(ncoeffs, ncoeffs);
    this->mw = gsl_multifit_linear_alloc(this->datum, ncoeffs);

    //Figure out if df_i uncertainties should be used. We autodetect whether the uncertainties are (1) valid and
    // (2) appropriate for this routine.
    bool use_uncert = true;
    for(const auto & samp : sampscopy.samples){
        double dyi = samp[3];
        if(!std::isfinite(dyi) || (std::abs(dyi) == 0.0)){
            use_uncert = false;
            break;
        }
    }
    if(use_uncert && !sampscopy.uncertainties_known_to_be_independent_and_random){
        FUNCWARN("Uncertainties are not known to be independent and random, so will not be used for b-spline fitting");
        use_uncert = false;
    }
    if(use_uncert){
        FUNCWARN("Currently, x_i uncertainties (dx_i), if present, are ignored by this routine!");
    }

    //Populate the GSL vectors with data for fitting.
    for(size_t i = 0; i < this->datum; ++i){
        const double x_i  = sampscopy.samples[indices[i]][0];
        const double y_i  = sampscopy.samples[indices[i]][2];
        const double dy_i = sampscopy.samples[indices[i]][3];
        const double w_i  = ( use_uncert ? std::pow(dy_i,-2.0) : 1.0 ); //Least-squares fit weighting.
        gsl_vector_set(this->x, i, x_i);
        gsl_vector_set(this->y, i, y_i);
        gsl_vector_set(this->w, i, w_i);
    }

    //Figure out where to put the breakpoints.
    if(breakpoints == basis_spline_breakpoints::equal_spacing){
        gsl_status = gsl_bspline_knots_uniform(this->xmin, this->xmax, this->bw);
        if(gsl_status != GSL_SUCCESS){
            throw std::runtime_error("Unable to compute knots from uniform breakpoints.");
        }

    }else if(basis_spline_breakpoints::adaptive_datum_density){
        auto *bps = gsl_vector_alloc(nbreaks);

        //Split the indices into (nbreaks-1) buckets.
        const size_t num_buckets = (nbreaks - 1); // The missing break is an endpoint.
        const size_t datum_per_bucket = this->datum / num_buckets; // Each bucket has this many.
        const size_t extras = this->datum % num_buckets; // But this many buckets have one extra.
        size_t used_extras = 0;

        //Distribute the number of datum amongst the buckets as evenly as possible.
        //
        // Note: This routine favours placing extra datum the maximal distance from the other extra datum.
        //       There is a bias to the left (i.e., earlier x_i) if there is a tie. So the placement order
        //       will be something like:
        //        1. Left-most bucket.
        //        2. Right-most bucket.
        //        3. Centre bucket.
        //        4. Bucket in middle between left-most and centre buckets.
        //        5. Bucket in middle between centre and right-mosts buckets.
        //        6. Bucket in middle between left-most and left-centre buckets.
        //        ...
        //
        //       Also note that if the centre bucket is not truly the centre, then the above will be
        //       altered to briefly be right-biased (until the extra datum become balanced again).
        //
        std::vector<size_t> datum_count_for_bucket(num_buckets, datum_per_bucket);
        
// TODO! HACKED SOLUTION WITH NO BALANCING!
for( ; used_extras < extras; ++used_extras) datum_count_for_bucket[used_extras] += 1;
        
        if(std::accumulate(datum_count_for_bucket.begin(), datum_count_for_bucket.end(), 0) != this->datum){
            throw std::logic_error("Miscounted datum distributed to buckets.");
        }

        //Scan through the datum, inserting breakpoints as we go.
        gsl_vector_set(bps, 0, this->xmin);
        gsl_vector_set(bps, num_buckets, this->xmax);
        auto index_iter = indices.begin();
        size_t l = 1;
        for(const auto &nbucket : datum_count_for_bucket){
            std::advance(index_iter, nbucket);
            if(index_iter == indices.end()) break;
            auto lhs = std::prev(index_iter, 1);

            const double x_L = sampscopy.samples[*lhs][0];
            const double x_R = sampscopy.samples[*index_iter][0];
            const double mid = 0.5*(x_L + x_R);
            gsl_vector_set(bps, l++, mid);
        }

        gsl_status = gsl_bspline_knots(bps, this->bw);
        if(gsl_status != GSL_SUCCESS){
            throw std::runtime_error("Unable to compute knots from adaptive datum density breakpoints.");
        }

        gsl_vector_free(bps);
    }


    //Construct the fit matrix X.
    for(size_t i = 0; i < this->datum; ++i){
        const double x_i = gsl_vector_get(this->x, i);

        //Compute all necessary B_j(x_i) coefficients at once. Fill them into the fitting matrix.
        gsl_bspline_eval(x_i, this->B, this->bw);
        for(size_t j = 0; j < ncoeffs; ++j){
            const double Bj = gsl_vector_get(this->B, j);
            gsl_matrix_set(this->X, i, j, Bj);
        }
    }

    //Perform the fit.
    gsl_status = gsl_multifit_wlinear(this->X, this->w, this->y, this->c, this->cov, &(this->chisq), this->mw);
    if(gsl_status != GSL_SUCCESS){
        throw std::runtime_error("Unable to perform linear least-squares. Cannot generate basis spline.");
    }
    this->dof = this->datum - ncoeffs;
    const auto tss = gsl_stats_wtss(this->w->data, 1, this->y->data, 1, this->y->size);
    this->Rsq = 1.0 - this->chisq / tss; //Coefficient of determination.

    return;
}


basis_spline::~basis_spline(){
    gsl_bspline_free(this->bw);
    gsl_vector_free(this->B);
    gsl_vector_free(this->x);
    gsl_vector_free(this->y);
    gsl_matrix_free(this->X);
    gsl_vector_free(this->c);
    gsl_vector_free(this->w);
    gsl_matrix_free(this->cov);
    gsl_multifit_linear_free(this->mw);
}


std::array<double,4> 
basis_spline::Sample(double t) const {
    double f_i;
    double df_i;

    int gsl_status;
    gsl_status = gsl_bspline_eval(t, this->B, this->bw);
    if(gsl_status != GSL_SUCCESS){
        throw std::runtime_error("Unable to evaluate bspline");
    }
    gsl_status = gsl_multifit_linear_est(this->B, this->c, this->cov, &f_i, &df_i);
    if(gsl_status != GSL_SUCCESS){
        throw std::runtime_error("Unable to apply bspline fitted parameters to generate sample");
    }

    return { t, 0.0, f_i, df_i };
}
