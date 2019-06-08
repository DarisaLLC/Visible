//
//  eigen_utils.cpp
//  Visible
//
//  Created by Arman Garakani on 6/7/19.
//

#include "eigen_utils.hpp"
#include "core/coreGLM.h"

void polyfit (const std::vector<glm::vec2> &xy, std::vector<double> &coeff, int order)
{
    std::vector<double> xv, yv;
    for (const glm::vec2& p : xy){
        xv.push_back(p.x);
        yv.push_back(p.y);
    }
    polyfit(xv, yv, coeff, order);
}


void polyfit (const std::vector<double> &y, double incr, std::vector<double> &coeff, int order)
{
    std::vector<double> xv, yv;
    double index = 0.0;
    for (const double& p : y){
        xv.push_back(index);
        yv.push_back(p);
        index = index + incr;
    }
    polyfit(xv, yv, coeff, order);
}

void polyfit(const std::vector<double> &xv, const std::vector<double> &yv, std::vector<double> &coeff, int order)
{
    Eigen::MatrixXd A(xv.size(), order+1);
    Eigen::VectorXd yv_mapped = Eigen::VectorXd::Map(&yv.front(), yv.size());
    Eigen::VectorXd result;
    
    assert(xv.size() == yv.size());
    assert(xv.size() >= order+1);
    
    // create matrix
    for (size_t i = 0; i < xv.size(); i++)
        for (size_t j = 0; j < order+1; j++)
            A(i, j) = pow(xv.at(i), j);
    
    // solve for linear least squares fit
    result = A.householderQr().solve(yv_mapped);
    
    coeff.resize(order+1);
    for (size_t i = 0; i < order+1; i++)
        coeff[i] = result[i];
}

int test_polyfit ()
{
    std::vector<double> x_values, y_values, coeff;
    double x, y;
    
    while (scanf("%lf %lf\n", &x, &y) == 2) {
        x_values.push_back(x);
        y_values.push_back(y);
    }
    
    polyfit(x_values, y_values, coeff, 3);
    printf("%f + %f*x + %f*x^2 + %f*x^3\n", coeff[0], coeff[1], coeff[2], coeff[3]);
    
    return 0;
}


