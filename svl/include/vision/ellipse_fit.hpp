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


#ifndef ellipse_fit_hpp
#define ellipse_fit_hpp


#include <iostream>
#include <Eigen/Dense>

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
    
    //Transpose matrix
    //std::vector<std::vector<T> >
    MatrixXf transposeMatrix(const MatrixXf &mtrx);
    
    //Do matrix multiplication, mtrx1: j*l; mtrx2: l*i; return: j*i
    MatrixXf doMtrxMul(const MatrixXf &mtrx1, const MatrixXf &mtrx2);
    
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

/*******************************************************************************
 * Template Class Defination
 ******************************************************************************/
DirectEllipseFit::DirectEllipseFit(const VectorXf &xData,
                                      const VectorXf &yData)
{
    m_xData = xData;
    m_yData = yData;
}

Ellipse DirectEllipseFit::doEllipseFit()
{
    //Data preparation: normalize data
    VectorXf xData = symmetricNormalize(m_xData);
    VectorXf yData = symmetricNormalize(m_yData);
    
    //Bulid n*6 design matrix, n is size of xData or yData
    MatrixXf dMtrx = getDesignMatrix(xData, yData);
    
    //Bulid 6*6 scatter matrix
    MatrixXf sMtrx = getScatterMatrix(dMtrx);
    
    //Build 6*6 constraint matrix
    MatrixXf cMtrx = getConstraintMatrix();
    
    //Solve eigensystem
    MatrixXf eigVV;
    
//    bool flag = solveGeneralEigens(sMtrx, cMtrx, eigVV);
  //  if(!flag)
   //     qDebug()<<"Eigensystem solving failure!";
    
    Ellipse ellip = calcEllipsePara(eigVV);
    
    return ellip;
}


float DirectEllipseFit::getMeanValue(const VectorXf &data)
{
    return data.mean();
}


float DirectEllipseFit::getMaxValue(const VectorXf &data)
{
    return data.maxCoeff();
}


float DirectEllipseFit::getMinValue(const VectorXf &data)
{
    return data.minCoeff();
}


float DirectEllipseFit::getScaleValue(const VectorXf &data)
{
    return (0.5 * (getMaxValue(data) - getMinValue(data)));
}


VectorXf DirectEllipseFit::symmetricNormalize(const VectorXf &data)
{
    float mean = getMeanValue(data);
    float normScale = getScaleValue(data);
    VectorXf symData = (data.array() - mean) / normScale;
    return symData;
}


VectorXf DirectEllipseFit::dotMultiply(const VectorXf &xData,
                                            const VectorXf &yData)
{
    return xData * yData;
//    for(int i=0; i<xData.size(); ++i)
//        product.append(xData.at(i)*yData.at(i));
//
//    return product;
}


MatrixXf DirectEllipseFit::getDesignMatrix(
                                                          const VectorXf &xData, const VectorXf &yData)
{
    MatrixXf designMtrx;
    designMtrx.conservativeResize(6,xData.count());
    designMtrx.row(0) = dotMultiply(xData, xData);
    designMtrx.row(1) = dotMultiply(xData, yData);
    designMtrx.row(2) = dotMultiply(yData, yData);
    designMtrx.row(3) = xData;
    designMtrx.row(4) = yData;
 //   designMtrx.row(5) = VectorXf::Constant(1.0f);
    return designMtrx;
}


DirectEllipseFit::Matrix6f DirectEllipseFit::getConstraintMatrix()
{
    Matrix6f consMtrx = Matrix6f::Constant(6,6,0.0f);
    consMtrx(1,1) = 1;
    consMtrx(0,2) = -2;
    consMtrx(2,0) = -2;
    
    return consMtrx;
}


MatrixXf DirectEllipseFit::getScatterMatrix(const MatrixXf &dMtrx)
{
    return dMtrx*dMtrx.transpose();
//    MatrixXf tMtrx = transposeMatrix(dMtrx);
//    return doMtrxMul(tMtrx, dMtrx);
}


MatrixXf DirectEllipseFit::transposeMatrix( const MatrixXf &mtrx)
{
    return mtrx.transpose();
    
//    for(int i=0; i<mtrx.first().size(); ++i){
//        std::vector<T> tmpVec;
//        for(int j=0; j<mtrx.size(); ++j){
//            tmpVec.append(mtrx.at(j).at(i));
//        }
//        outMtrx.append(tmpVec);
//    }
//    
//    return outMtrx;
}


MatrixXf DirectEllipseFit::doMtrxMul(const MatrixXf &mtrx1, const MatrixXf &mtrx2)
{
    return mtrx1 * mtrx2;
    
//    MatrixXf mulMtrx;
//
//    for(int i=0; i<mtrx2.size(); ++i){
//        std::vector<T> tmpVec;
//        for(int j=0; j<mtrx1.first().size(); ++j){
//            T tmpVal = 0;
//            //l is communal for mtrx1 and mtrx2
//            for(int l=0; l<mtrx1.size(); ++l){
//                tmpVal += mtrx1.at(l).at(j) * mtrx2.at(i).at(l);
//            }
//            tmpVec.append(tmpVal);
//        }
//        mulMtrx.append(tmpVec);
//    }
//
//    return mulMtrx;
}


bool DirectEllipseFit::solveGeneralEigens(const MatrixXf &sMtrx,
                                             const MatrixXf &cMtrx,
                                             MatrixXf &eigVV)
{
    GeneralizedEigenSolver<MatrixXf> ges;
    ges.compute(sMtrx, cMtrx);
    std::cout << ges.eigenvalues().transpose();
    return true;
}



Ellipse DirectEllipseFit::calcEllipsePara(const MatrixXf &eigVV)
{
    //Extract eigenvector corresponding to negative eigenvalue
    int eigIdx = -1;
    for(int i=0; i<eigVV.rows(); ++i){
        float tmpV = eigVV(i,0);
        if(tmpV<1e-6 && !isinf(tmpV)){
            eigIdx = i;
            break;
        }
    }
    if(eigIdx<0)
        return Ellipse();
    
    //Unnormalize and get coefficients of conic section
    float tA = eigVV(eigIdx,1);
    float tB = eigVV(eigIdx,2);
    float tC = eigVV(eigIdx,3);
    float tD = eigVV(eigIdx,4);
    float tE = eigVV(eigIdx,5);
    float tF = eigVV(eigIdx,6);
    
    float mx = getMeanValue(m_xData);
    float my = getMeanValue(m_yData);
    float sx = getScaleValue(m_xData);
    float sy = getScaleValue(m_yData);
    
    Ellipse ellip;
    ellip.a = tA * sy * sy;
    ellip.b = tB * sx * sy;
    ellip.c = tC * sx * sx;
    ellip.d = -2*tA*sy*sy*mx - tB*sx*sy*my + tD*sx*sy*sy;
    ellip.e = -tB*sx*sy*mx - 2*tC*sx*sx*my + tE*sx*sx*sy;
    ellip.f = tA*sy*sy*mx*mx + tB*sx*sy*mx*my + tC*sx*sx*my*my
    - tD*sx*sy*sy*mx - tE*sx*sx*sy*my + tF*sx*sx*sy*sy;
    ellip.algeFlag = true;
    
    ellip.alge2geom();
    
    return ellip;
}


#endif /* ellipse_fit_hpp */
