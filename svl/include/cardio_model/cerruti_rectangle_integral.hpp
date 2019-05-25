#ifndef _CERRUTI_INT_HPP
#define _CERRUTI_INT_HPP

using namespace std;
#include "cardio_model/metric.hpp"

namespace cm
{
  
    namespace cerruti_rect {


        /**
         integrate

         @param X X distance from center
         @param Y Y distance from the center
         @param a a b are the half-sizes of the rectangular load
         @param b
         @return return integral value
         */
        static inline integral4_t integrate (double& X, double& Y , const double& a, const double& b)
        {
            integral4_t I;
       
            double x = X + a;
            double y = Y + b;
            double x2 = x / 2.0;
            double y2 = y / 2.0;

            /**
             lambdas for partial integration computation
             
             @param a a b are the half-sizes of the rectangular load
             @param b
             @return part integral
             */
            
            auto integ1 = [](const double& p, const double& q)
            {
                return q*asinh(p/q)+p*asinh(q/p);
            };
            auto integ2 = [](const double& p, const double& q)
            {
                return q*asinh(p/q);
            };
            auto integ3 = [](const double& p, const double& q)
            {
                return p+q - std::sqrt(p*p+q*q);
            };
            auto integ4 = [](const double& p, const double& q)
            {
                double rtn = p*atan(q/p)+0.5*q*log(1+std::pow(p/q, 2.0) );
                return - rtn;
            };

            
            
            // Normal point within rectangle
            if (std::fabs(X)<=a && std::fabs(Y)<=b)
            {
                if (std::fabs(X) != a & std::fabs(Y) != b)
                {
                    //                    % arbitrary point
                    I[0] = integ1(x2,y2)+integ1(a-x2,y2)+integ1(x2,b-y2)+integ1(a-x2,b-y2);
                    I[1] = integ2(x2,y2)+integ2(a-x2,y2)+integ2(x2,b-y2)+integ2(a-x2,b-y2);
                    I[2] = integ3(x2,y2)-integ3(a-x2,y2)-integ3(x2,b-y2)+integ3(a-x2,b-y2);
                    I[3] = integ4(x2,y2)-integ4(a-x2,y2)+integ4(x2,b-y2)-integ4(a-x2,b-y2);
                }
                else if (std::fabs(X) != a & std::fabs(Y) == b)
                {
                    //elseif abs(X)~=a & abs(Y)==b
                    //% horizontal edge, not a corner
                    if (y==0)
                    {
                        I[0] =  integ1(x2,b)+integ1(a-x2,b);
                        I[1] =  integ2(x2,b)+integ2(a-x2,b);
                        I[2] = -integ3(x2,b)+integ3(a-x2,b);
                        I[3] =  integ4(x2,b)-integ4(a-x2,b);
                    }
                    else
                    {
                        I[0] = integ1(x2,b)+integ1(a-x2,b);
                        I[1] = integ2(x2,b)+integ2(a-x2,b);
                        I[2] = integ3(x2,b)-integ3(a-x2,b);
                        I[3] = integ4(x2,b)-integ4(a-x2,b);
                    }
                }
                else if (std::fabs(X) == a & std::fabs(Y) != b)
                {
                    //elseif abs(X)==a & abs(Y)~=b
                    // % vertical edge, not a corner
                    if (x==0)
                    {
                        I[0] =  integ1(a,y2)+integ1(a,b-y2);
                        I[1] =  integ2(a,y2)+integ2(a,b-y2);
                        I[2] = -integ3(a,y2)+integ3(a,b-y2);
                        I[3] = -integ4(a,y2)-integ4(a,b-y2);
                    }
                    else
                    {
                        I[0] = integ1(a,y2)+integ1(a,b-y2);
                        I[1] = integ2(a,y2)+integ2(a,b-y2);
                        I[2] = integ3(a,y2)-integ3(a,b-y2);
                        I[3] = integ4(a,y2)+integ4(a,b-y2);
                    }
                }
                else
                {
                    // % It is one of the corners
                    I[0] =  integ1(a,b);
                    I[1] =  integ2(a,b);
                    if (x==0 & y==0)
                    {
                        //                % left lower corner
                        I[2] =  integ3(a,b);
                        I[3] = -integ4(a,b);
                    }
                    else if (x==0 & y>0)
                        //            % left upper corner
                    {
                        I[2] = -integ3(a,b);
                        I[3] = -integ4(a,b);
                    }
                    else if (x>0 & y==0)
                        //            % right lower corner
                    {
                        I[2] = -integ3(a,b);
                        I[3] = integ4(a,b);
                    }
                    else if (x>0 & y>0)
                        //     % right upper corner
                    {
                        I[2] = integ3(a,b);
                        I[3] = integ4(a,b);
                    }
                }

            }
            // If the point is outside
            else
            {
                int sgnx, sgny;
                
                if (X < -a)
                {
                    X = -X;
                    x = X+a;
                    x2 = x/2;
                    sgnx = -1;
                }
                else
                    sgnx = 1;
                
                if (Y < -b)
                {
                    Y = -Y;
                    y = Y+b;
                    y2 = y/2;
                    sgny = -1;
                }
                else
                    sgny = 1;
                
                double x2_a = x2 - a;
                double y2_b = y2 - b;
                
                if (std::fabs(X) > a && std::fabs(Y) > b)
                {
                    //                % point beyond both the horizontal and vertical edges
                    auto J1 = integrate(x2,y2,x2,y2);
                    auto J2 = integrate(x2_a,y2_b,x2_a,y2_b);
                    auto J3 = integrate(x2,y2_b,x2,y2_b);
                    auto J4 = integrate(x2_a,y2,x2_a,y2);
                    I = J1+J2-J3-J4;
                }
                if (std::fabs(X) > a && std::fabs(Y) <= b)
                    //elseif abs(X)>a & abs(Y)<=b
                    //% point beyond left or right edge
                {
                    auto J1 = integrate(x2,Y,x2,b);
                    auto J2 = integrate(x2_a,Y,x2_a,b);
                    I = J1-J2;
                }
                if (std::fabs(X) <= a && std::fabs(Y) > b)
                    //                        elseif abs(X)<=a & abs(Y)>b
                    //                        % point beyond upper or lower edge
                {
                    auto J1 = integrate(X,y2,a,y2);
                    auto J3 = integrate(X,y2_b,a,y2_b);
                    I = J1-J3;
                }
                
                if (sgnx<0)
                {
                    I(3) = -I(3);
                    I(4) = -I(4);
                }
                if (sgny<0) I(3)   = -I(3);
            }
            
            return I;
        }
  
    
        
      }
}



#endif
