    #ifndef _ELLIPSE_
    #define _ELLIPSE_

#include "core/core.hpp"

namespace svl {
    

 inline double scaleAngle_0_2pi(double a)
 {
     while(a >= svl::constants::two_pi ) a -= svl::constants::two_pi;
     while(a < 0.) a += svl::constants::two_pi;
     return a;
 }

class ellipseShape
{
 
public:
    int id;
    // ellipse position      [px]
    double x, y;
    // ellipse long/short axis length [px]
    double a, b;
    // ellipse rotation in radiants [rad]
    double phi;
    // mean fit error
    double fit_error;
    // ellipse support
    double support;
    
    ellipseShape() {}
    ellipseShape(const ellipseShape &e){
        x = e.x;
        y = e.y;
        a = e.a;
        b = e.b;
        phi = e.phi;
    }

    ellipseShape(const cv::RotatedRect& rect){
        x = rect.center.x;
        y = rect.center.y;
        // box size is double the axis lengths
        a = double(rect.size.width)/2.;
        b = double(rect.size.height)/2.;
        // note: the angle returned is in degrees!
        phi = scaleAngle_0_2pi(rect.angle*svl::constants::pi/180.);
        // note: for unknown reasons sometimes a < b!
        if(a < b)
        {
            swap(a, b);
            phi = scaleAngle_0_2pi(phi + svl::constants::pi_2);
        }
        setEllipse(x, y, a, b, phi);
    }

    void setEllipse(const double &_x, const double &_y, const double &_a, const double &_b, const double &_phi){
        x = _x, y = _y, a = _a, b = _b, phi = _phi;
        a2 = a*a;
        a4 = a2*a2;
        b2 = b*b;
        b4 = b2*b2;
        x2 = 0;
        y2 = 0;
        m_e = std::sqrt(1.0 - (b2/a2));
        m_c = a * m_e;
        bool bz = isZero(b);
        
        auto acot = [](double cc){
            return svl::constants::pi_2 - std::atan(cc);
        };
        
        if (a < m_c){
            m_p = bz ? 0.0 : 0.5 * acot((a-m_c)/(b+b));
        }else if (a > m_c){
            m_p = bz ? svl::constants::pi_2 : svl::constants::pi_2 + 0.5 * acot((a-m_c)/(b+b));
        }else if (a == m_c)
            m_p = svl::constants::pi_2;
        
    }
    
    bool isValid () {
        return !isZero(a2) && !isZero(a);
    }
    

    double eccentricity () {
        return m_e;
    }
    
    double ccwRotationXtoMajor (){
        return m_p;
    }
    
    void get(cv::RotatedRect &r){
        r.center.x = x, r.center.y = y;
        r.size.width = a*2., r.size.height = b*2.;
        r.angle = phi*180./M_PI;
        
        
    }
    
    double ellipseCircumference(double a, double b){
        return M_PI*(1.5*(a + b) - sqrt(a*b));
    }
    
    static bool insideEllipse(double _a, double _b, double _x0, double _y0, double _phi, double _x, double _y) {
        double dx = ((_x - _x0)*cos(_phi) + (_y-_y0)*sin(_phi)) / _a;
        double dy = (-(_x - _x0)*sin(_phi) + (_y-_y0)*cos(_phi)) / _b;
        double distance = dx * dx + dy * dy;
        return (distance < 1.0) ? 1 : 0;
        
    }
    
    bool insideEllipse(double _x, double _y) const{
        double dx = ((_x - x)*cos(phi) + (_y-y)*sin(phi)) / a;
        double dy = (-(_x - x)*sin(phi) + (_y-y)*cos(phi)) / b;
        double distance = dx * dx + dy * dy;
        return (distance < 1.0) ? 1 : 0;
        
    }
    
    double normalizedDistanceFromOrigin(double _x, double _y) const{
        double dx = ((_x - x)*cos(phi) + (_y-y)*sin(phi)) / a;
        double dy = (-(_x - x)*sin(phi) + (_y-y)*cos(phi)) / b;
        return 1.0 - (dx * dx + dy * dy);
    }
    
    cv::Mat imagePoints (){
        cv::Point2f pi[4];
        cv::RotatedRect boxEllipse;
        get (boxEllipse);
        boxEllipse.points(pi);
        cv::Point2f& pc = boxEllipse.center;
        cv::Mat imagePoints = (cv::Mat_<double>(5,2) << pc.x, pc.y, pi[0].x, pi[0].y, pi[1].x, pi[1].y, pi[2].x, pi[2].y, pi[3].x, pi[3].y);
        return imagePoints;
    }
    
    unsigned ellipseSupport(const std::vector<cv::Point2f> &points, double inl_dist, std::vector<bool> &inl_idx){
        {
            double co = cos(-phi), si = sin(-phi), n, d, dist;
            unsigned z;
            unsigned nbInl=0;
            
        
            cv::Point2f p, q;
            
            fit_error = 0.;
            inl_idx.resize(points.size());
            
            for(z=0; z<points.size(); z++)
            {
                // transform point in image coords to ellipse coords
                // Note that this piece of code is called often.
                // Implementing this explicitely here is faster than using
                // TransformToEllipse(), as cos and sin only need to be evaluated once.
                //p = Vector2(points[z].x,points[z].y);
                p=points[z];
                p.x -= x;
                p.y -= y;
                q.x = co*p.x - si*p.y;
                q.y = si*p.x + co*p.y;
                // calculate absolute distance to ellipse
                if(isZero(q.x) && isZero(q.y))
                    dist = b;
                else
                {
                    x2 = q.x * q.x;
                    y2 = q.y * q.y;
                    n = fabs(x2/a2 + y2/b2 - 1.);
                    d = 2.*sqrt(x2/a4 + y2/b4);
                    dist = n/d;
                }
                if(dist <= inl_dist)
                {
                    inl_idx[z]=true;
                    nbInl++;
                    fit_error += dist;
                }
                else
                    inl_idx[z]=false;
            }
            
            double circumference = ellipseCircumference(a, b);
            
            if (nbInl>0 && !isZero(circumference))
            {
                fit_error /= (double)nbInl;
                support = (double)nbInl / circumference;
            }
            else
            {
                fit_error = DBL_MAX;
                support=0;
            }
            
            return nbInl;
        }
    }
    
    
private:
    bool isZero (const float d){
        return fabs(d) <= std::numeric_limits<float>::epsilon();
    }
    double a2, b2, a4, b4;  // a^2 etc., stored for Distance()
    double x2,y2;
    double m_e, m_c, m_p;

};

}

#endif

