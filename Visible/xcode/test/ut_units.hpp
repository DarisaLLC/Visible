#ifndef __BOOST_UNITS__
#define __BOOST_UNITS__

#include <complex>
#include <iostream>

#include "boost/typeof/std/complex.hpp"

#include "boost/units/systems/si/energy.hpp"
#include <boost/units/systems/si/force.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/electric_potential.hpp>
#include <boost/units/systems/si/current.hpp>
#include <boost/units/systems/si/resistance.hpp>
#include <boost/units/systems/si/io.hpp>
#include <Eigen/Dense>
#include <Eigen/Core>
#include <cassert>
#include "core/pair.hpp"

using namespace Eigen;
using namespace svl;
using Eigen::MatrixXd;
using namespace boost::units;
using namespace boost::units::si;
using namespace std;
namespace bi=boost::units;

// 2-d array, column major
template <typename T>
using CArrXXt = Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic>;
using CArrXXd = Eigen::ArrayXXd;

// 2-d array, row major
template <typename T>
using RArrXXt =
  Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using RArrXXd = RArrXXt<double>;

quantity<energy>
work(const quantity<force>& F, const quantity<bi::si::length>& dx)
{
    return F * dx; // Defines the relation: work = force * distance.
}



typedef vector <double> record_t;
typedef vector <record_t> data_t;


std::shared_ptr<std::ifstream> make_shared_ifstream(std::ifstream * ifstream_ptr)
{
    return std::shared_ptr<std::ifstream>(ifstream_ptr, ifStreamDeleter());
}

std::shared_ptr<std::ifstream> make_shared_ifstream(std::string filename)
{
    return make_shared_ifstream(new std::ifstream(filename, std::ifstream::in));
}

void flip(const data_t& src, data_t& dst)
{
    std::vector<size_t> sizes;
    for (auto n = 0; n < src.size(); n++)
        sizes.push_back(src[n].size());
    auto result = std::minmax_element(sizes.begin(), sizes.end() );
    if (*result.first != *result.second || *result.first == src.size()){
        std::cout << src.size() << "   " << *result.first << "," << *result.second << std::endl;
        return;
    }
    dst.resize(*result.first);
    for (auto row = 0; row < src.size(); row++){
        for (auto col = 0; col < dst.size(); col++)
        {
            auto val = src[row][col];
            dst[col].push_back(val);
        }
    }
  
}
//-----------------------------------------------------------------------------
// Let's overload the stream input operator to read a list of CSV fields (which a CSV record).
// Remember, a record is a list of doubles separated by commas ','.
istream& operator >> ( istream& ins, record_t& record )
{
    // make sure that the returned record contains only the stuff we read now
    record.clear();
    
    // read the entire line into a string (a CSV record is terminated by a newline)
    string line;
    getline( ins, line );
    
    // now we'll use a stringstream to separate the fields out of the line
    stringstream ss( line );
    string field;
    while (getline( ss, field, ',' ))
    {
        // for each field we wish to convert it to a double
        // (since we require that the CSV contains nothing but floating-point values)
        stringstream fs( field );
        double f = 0.0;  // (default value is 0.0)
        fs >> f;
        
        // add the newly-converted field to the end of the record
        record.push_back( f );
    }
    
    // Now we have read a single line, converted into a list of fields, converted the fields
    // from strings to doubles, and stored the results in the argument record, so
    // we just return the argument stream as required for this kind of input overload function.
    return ins;
}

//-----------------------------------------------------------------------------
// Let's likewise overload the stream input operator to read a list of CSV records.
// This time it is a little easier, just because we only need to worry about reading
// records, and not fields.
istream& operator >> ( istream& ins, data_t& data )
{
    // make sure that the returned data only contains the CSV data we read here
    data.clear();
    
    // For every record we can read from the file, append it to our resulting data
    record_t record;
    while (ins >> record)
    {
        data.push_back( record );
    }
    
    // Again, return the argument stream as required for this kind of input stream overload.
    return ins;
}

//-----------------------------------------------------------------------------
// Now to put it all to use.
int load_sm(const std::string& file, data_t& data, bool debug = false)
{
    // Here is the file containing the data. Read it into data.
    auto infileRef =  make_shared_ifstream(file );
    *infileRef >> data;
    if (!infileRef->eof())
        return 1;
    
 
    
    if(debug)
        cout << "CSV file contains " << data.size() << " records.\n";
 
    if(debug){
        cout << "The second field in the fourth record contains the value " << data[ 0 ][ 0 ] << ".\n";
    }
    return 0;
}

int load_record(const std::string& file, record_t& data, bool debug = false)
{
    // Here is the file containing the data. Read it into data.
    auto infileRef =  make_shared_ifstream(file );
    *infileRef >> data;
    if (!infileRef->eof())
        return 1;
    
    
    
    if(debug)
        cout << "CSV file contains " << data.size() << " records.\n";


    return 0;
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
        
        EXPECT_EQ(true, (i * z == v));
        //std::cout << "V   = " << v << std::endl
        //<< "I   = " << i << std::endl
        //<< "Z   = " << z << std::endl
        // Calculate from Ohm's law voltage = current * resistance.
        //<< "I * Z = " << i * z << std::endl
        // Check defined V is equal to calculated.
        //<< "I * Z == V? " << std::boolalpha << (i * z == v) << std::endl
    }
};

namespace eigen_ut
{
    static void clone_column (Eigen::MatrixXf& F)
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
        Eigen::MatrixXf F_(8,8);
        
        F_.setZero ();
        
        for (auto i = 0; i < F_.rows(); i++)
            F_(i,0) = i+1;
        
        clone_column (F_);

        Eigen::SelfAdjointEigenSolver<MatrixXf> eigensolver(F_);
        EXPECT_EQ(true,eigensolver.info() == Eigen::Success);
        auto num = eigensolver.eigenvalues().size();
        auto lambda = eigensolver.eigenvalues()[0];
        auto lambda2 = eigensolver.eigenvalues()[num-1];
        EXPECT_NEAR(lambda, -13.1371, 0.0001);
        EXPECT_NEAR(lambda2, 29.638, 0.0001);
        
     //   const auto & ev = eigensolver.eigenvalues().as;
//        auto maxe = std::max_element(ev.begin(), ev.end());
        

    }
    
}
#endif


    
#if NotYet
    
    float X[]={185,182.854,181.225,180.086,178.973,177.868,176.755,175.622,174.296,173.13,171.969,170.798,169.279,168.079,166.893,164.909,163.696,162.138,160.956,159.762,158.209,157.017,155.826,154.192,152.988,150.933,148.857,147.078,144.993,142.906,140.81,139.01,136.911,135.068,132.962,130.85,128.978,127.072,124.962,123.033,120.929,118.985,117.026,114.933,112.97,110.997,109.017,107.03,104.968,102.983,100.992,98.997,96.9998,95.0016,93.0035,91.007,89.0142,86.9663,84.9676,82.9798,81.0034,79.0373,76.9419,74.9862,73.043,70.9357,69.0133,66.9003,64.9952,62.8779,60.9968,58.8842,57.0397,54.9433,52.8446,51.0558,48.9744,46.8964,44.8226,43.1216,41.9258,39.8931,38.2492,37.0425,35.8484,34.2504,33.055,31.8722,30.6703,29.1848,28.0179,26.8576,25.6873,24.2945,23.136,21.9922,20.8515,19.7037,18.3912,17.2477};
    float Y[]={61.198,61.5732,62.3475,63.1181,63.9648,64.8266,65.6698,66.469,67.4408,68.2048,68.9483,69.6458,70.5159,71.1517,71.7898,72.8201,73.4095,74.264,74.9162,75.5511,76.3976,77.0326,77.6426,78.4249,78.9721,79.8223,80.5938,81.2261,81.9788,82.7216,83.4149,84.0314,84.6993,85.2384,85.8604,86.4056,86.9039,87.3347,87.8113,88.1805,88.5699,88.8985,89.1848,89.4768,89.7476,89.9753,90.1773,90.3597,90.5298,90.663,90.7535,90.804,90.82,90.8109,90.7843,90.7399,90.6681,90.5499,90.3878,90.1938,89.9716,89.7166,89.3934,89.0862,88.7556,88.3275,87.9361,87.4582,87.0212,86.503,86.0123,85.4124,84.8682,84.1757,83.4614,82.8385,82.0713,81.272,80.4356,79.7146,79.1696,78.2374,77.4562,76.9088,76.3161,75.5034,74.8949,74.2351,73.5835,72.6877,71.9707,71.2272,70.4893,69.5496,68.7953,68.0116,67.2182,66.4283,65.4503,64.6586};
    
    Eigen::Map<Eigen::VectorXf> xVec(X,4);
    Eigen::Map<Eigen::VectorXf> yVec(Y,4);
    DirectEllipseFit ellipFit(xVec, yVec);
    Ellipse ellip = ellipFit.doEllipseFit();
    std::cout << ellip << std::endl;
    return ellip.phi;
#endif

