//
//  ellipse_fit.cpp
//  Visible
//
//  Created by Arman Garakani on 2/4/19.
//

#include "vision/ellipse_fit.hpp"
#include <math.h>


Eigen::VectorXd Ellipse::directEllipse(const std::vector<cv::Point2f>& points, Ellipse& dst)
{
    const size_t nPoints = points.size ();
    // only consider x and y coordinates: supposing points are in a plane
    Eigen::MatrixXd XY(nPoints,2);
    // TODO: optimise
    for (unsigned int i = 0; i < nPoints; ++i) {
        XY (i,0) = points[i].x;
        XY (i,1) = points[i].y;
    }
    
    Eigen::Vector2d centroid;
    centroid << XY.block(0,0, nPoints,1).mean (),
    XY.block(0,1, nPoints, 1).mean ();
    
    Eigen::MatrixXd D1 (nPoints,3);
    D1 << (XY.block (0,0, nPoints,1).array () - centroid(0)).square (),
    (XY.block (0,0, nPoints,1).array () - centroid(0))*(XY.block (0,1, nPoints,1).array () - centroid(1)),
    (XY.block (0,1, nPoints,1).array () - centroid(1)).square ();
    Eigen::MatrixXd D2 (nPoints,3);
    D2 << XY.block (0,0, nPoints,1).array () - centroid(0),
    XY.block (0,1, nPoints,1).array () - centroid(1),
    Eigen::MatrixXd::Ones (nPoints,1);
    
    Eigen::Matrix3d S1 = D1.transpose () * D1;
    Eigen::Matrix3d S2 = D1.transpose () * D2;
    Eigen::Matrix3d S3 = D2.transpose () * D2;
    
    Eigen::Matrix3d T = -S3.inverse () * S2.transpose ();
    Eigen::Matrix3d M_orig = S1 + S2 * T;
    Eigen::Matrix3d M; M.setZero ();
    M << M_orig.block<1,3>(2,0)/2, -M_orig.block<1,3>(1,0), M_orig.block<1,3>(0,0)/2;
    
    Eigen::EigenSolver<Eigen::Matrix3d> es(M);
    Eigen::EigenSolver<Eigen::Matrix3d>::EigenvectorsType evecCplx = es.eigenvectors ();
    
    Eigen::Matrix3d evec = evecCplx.real ();
    
    Eigen::VectorXd cond (3);
    // The condition has the form 4xz - y^2 > 0 (infinite elliptic cone) for all
    // three eigen vectors. If none of the eigen vectors fulfils the inequality,
    // the direct ellipse method fails.
    cond = (4*evec.block<1,3>(0,0).array () * evec.block<1,3>(2,0).array () -
            evec.block<1,3>(1,0).array ().square ()).transpose ();
    
    Eigen::MatrixXd A0 (0,0);
    // TODO: A0 should always be of size 3x1 --> fix
    for (unsigned int i = 0; i < cond.size (); ++i) {
        if (cond(i) > 0.0) {
            A0.resize (3,i+1);
            A0.block(0,i,3,1) = evec.block<3,1>(0,i);
        }
    }
    if (A0.size () < 3) {
        std::ostringstream oss
        ("intersect::directEllipse: Could not create ellipse approximation. Maybe try circle instead?");
        throw std::runtime_error (oss.str ());
    }
    // A1.rows () + T.rows () should always be equal to 6!!
    Eigen::MatrixXd A(A0.rows () + T.rows (), A0.cols ());
    A.block(0,0,A0.rows (), A0.cols ()) = A0;
    A.block(A0.rows (), 0, T.rows (), A0.cols ()) = T*A0;
    
    double A3 = A(3,0) - 2*A(0,0) * centroid(0) - A(1,0) * centroid(1);
    double A4 = A(4,0) - 2*A(2,0) * centroid(1) - A(1,0) * centroid(0);
    double A5 = A(5,0) + A(0,0) * centroid(0)*centroid(0) + A(2,0) * centroid(1)*centroid(1) +
    A(1,0) * centroid(0) * centroid(1) - A(3,0) * centroid(0) - A(4,0) * centroid(1);
    
    A(3,0) = A3;  A(4,0) = A4;  A(5,0) = A5;
    A = A/A.norm ();
    
    dst.a = A(0,0);
    dst.b = A(1,0);
    dst.c = A(2,0);
    dst.d = A(3,0);
    dst.e = A(4,0);
    dst.f = A(5,0);
    
    return A.block<6,1>(0,0);
    
}


Ellipse::Ellipse()
{
    algeFlag = false;
    a = b = c = d = e = f = 0;
    
    geomFlag = false;
    cx = cy = 0;
    rl = rs = 0;
    phi = 0;
}

void Ellipse::alge2geom()
{
    if(!algeFlag)
        return;
    
    double tmp1 = b*b - 4*a*c;
    double tmp2 = sqrt((a-c)*(a-c)+b*b);
    double tmp3 = a*e*e + c*d*d - b*d*e + tmp1*f;
    
    double r1 = -sqrt(2*tmp3*(a+c+tmp2)) / tmp1;
    double r2 = -sqrt(2*tmp3*(a+c-tmp2)) / tmp1;
    rl = r1>=r2 ? r1 : r2;
    rs = r1<=r2 ? r1 : r2;
    
    cx = (2*c*d - b*e) / tmp1;
    cy = (2*a*e - b*d) / tmp1;
    
    phi = 0.5 * atan2(b, a-c);
    if(r1>r2)
        phi += M_PI_2;
    
    geomFlag = true;
}

void Ellipse::geom2alge()
{
    if(!geomFlag)
        return;
    
    a = rl*rl*sin(phi)*sin(phi) + rs*rs*cos(phi)*cos(phi);
    b = 2 * (rs*rs - rl*rl) * sin(phi) * cos(phi);
    c = rl*rl*cos(phi)*cos(phi) + rs*rs*sin(phi)*sin(phi);
    d = -2*a*cx - b*cy;
    e = -b*cx - 2*c*cy;
    f = a*cx*cx + b*cx*cy + c*cy*cy - rl*rl*rs*rs;
    
    algeFlag = true;
}



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
    
    bool flag = solveGeneralEigens (sMtrx, cMtrx, eigVV);
    if(!flag)
        std::cout << "Eigensystem solving failure!" << std::endl;
    
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
    auto  N=xData.size();
    Eigen::VectorXf product(N);
    
    for(int i=0; i<N; ++i)
        product(i)=xData(i)*yData(i);
    
    return product;
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
    VectorXf u(xData.size());
    for (auto ii = 0; ii < xData.size(); ii++)u[ii] = 1;
    designMtrx.row(5) = u;
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
    auto tmp = dMtrx;
    tmp.transposeInPlace();
    return dMtrx * tmp;
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
