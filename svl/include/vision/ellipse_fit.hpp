//
//  ellipse_fit.hpp
//  Visible
//
//  Created by Arman Garakani on 2/4/19.
//

/*******************************************************************************
 * ProName: DirectEllipseFit.h
 * Author:  Zhenyu Yuan
 * Data:    2016/7/27
 * -----------------------------------------------------------------------------
 * INSTRUCTION: Perform ellipse fitting by direct method
 * DEPENDANCE:  clapack and Qt
 * REFERENCE:
 *      (1) Fitzgibbon, A., et al. (1999). "Direct least square fitting of ellipses."
 *          IEEE Transactions on pattern analysis and machine intelligence 21(5):
 *          476-480.
 *      (2) http://homepages.inf.ed.ac.uk/rbf/CVonline/LOCAL_COPIES/FITZGIBBON/ELLIPSE/
 * CONVENTION:
 *      (1) Matrix expressed as std::vector<std::vector<T> >, fast direction is rows,
 *          slow direction is columns, that is, internal std::vector indicates column
 *          vector of T, external std::vector indicates row vector of column vectors
 *      (2) Matrix expressed as 1-order array, arranged in rows, that is, row by
 *          row.
 ******************************************************************************/


#pragma once

#ifndef ellipse_fit_hpp
#define ellipse_fit_hpp


#include <iostream>
#include <Eigen/Dense>
#include "vision/opencv_utils.hpp"

using namespace Eigen;


class Ellipse
{
public:
    Ellipse();
    /**
     * @brief alge2geom:    algebraic parameters to geometric parameters
     * @ref:    https://en.wikipedia.org/wiki/Ellipse#In_analytic_geometry
     *          http:homepages.inf.ed.ac.uk/rbf/CVonline/LOCAL_COPIES/FITZGIBBON/ELLIPSE/
     * @note:   The calculation of phi refer to wikipedia is not correct,
     *          refer to Bob Fisher's matlab program.
     *          What's more, the calculated geometric parameters can't back to
     *          initial algebraic parameters from geom2alge();
     */
    void alge2geom();
    /**
     * @brief geom2alge:    geometric parameters to algebraic parameters
     * @ref:    https://en.wikipedia.org/wiki/Ellipse#In_analytic_geometry
     */
    void geom2alge();

    static Eigen::VectorXd directEllipse(const std::vector<cv::Point2f>& points, Ellipse& );
    
public:
    //algebraic parameters as coefficients of conic section
    float a, b, c, d, e, f;
    bool algeFlag;
    
    //geometric parameters
    float cx;   //centor in x coordinate
    float cy;   //centor in y coordinate
    float rl;   //semimajor: large radius
    float rs;   //semiminor: small radius
    float phi;  //azimuth angel in radian unit
    bool geomFlag;
    
    friend std::ostream &    operator << (std::ostream &os,  const Ellipse&  e)
    {
        os << "{" << e.cx << "," << e.cy << " /  " << toDegrees(e.phi) << "(" << e.rl << "," << e.rs << ")" << "}" << std::endl;
        os << "{" << e.a << "," << e.b << "," << e.c << "," << e.c << "," << e.d << "," << e.f << "}" << std::endl;
        return os;
    }
    
};

class DirectEllipseFit
{
public:
    
    typedef Matrix<float,6,6> Matrix6f;
    
   // DirectEllipseFit(const std::vector<T> &xData, const std::vector<T> &yData);
    DirectEllipseFit(const VectorXf &xData, const VectorXf &yData);
    Ellipse doEllipseFit();
    
 
    
private:
    float getMeanValue(const VectorXf &data);
    float getMaxValue(const VectorXf &data);
    float getMinValue(const VectorXf &data);
    float getScaleValue(const VectorXf &data);
    VectorXf symmetricNormalize(const VectorXf &data);
    //Make sure xData and yData are of same size
    VectorXf dotMultiply(const VectorXf &xData, const VectorXf &yData);
    
    //Get n*6 design matrix D, make sure xData and yData are of same size
    //std::vector<std::vector<T> >
    MatrixXf getDesignMatrix(const VectorXf &xData,
                                         const VectorXf &yData);
    //Get 6*6 constraint matrix C
    // std::vector<std::vector<T> >
    Matrix6f getConstraintMatrix();
    
    //Get 6*6 scatter matrix S from design matrix
    //std::vector<std::vector<T> >
    MatrixXf getScatterMatrix(const MatrixXf &dMtrx);
    
//    //Transpose matrix
//    //std::vector<std::vector<T> >
//    MatrixXf transposeMatrix(const MatrixXf &mtrx);
//    
//    //Do matrix multiplication, mtrx1: j*l; mtrx2: l*i; return: j*i
//    MatrixXf doMtrxMul(const MatrixXf &mtrx1, const MatrixXf &mtrx2);
    
    /**
     * @brief solveGeneralEigens:   Solve generalized eigensystem
     * @note        For real eiginsystem solving.
     * @param sMtrx:    6*6 square matrix in this application
     * @param cMtrx:    6*6 square matrix in this application
     * @param eigVV:    eigenvalues and eigenvectors, 6*7 matrix
     * @return  success or failure status
     */
    bool solveGeneralEigens(const MatrixXf &sMtrx,
                            const MatrixXf &cMtrx,
                            MatrixXf &eigVV);
   
    
    /**
     * @brief calcEllipsePara:  calculate ellipse parameter form eigen information
     * @param eigVV:    eigenvalues and eigenvectors
     * @return ellipse parameter
     */
    Ellipse calcEllipsePara(const MatrixXf &eigVV);
    
private:
    VectorXf m_xData, m_yData;
};


#endif /* ellipse_fit_hpp */
