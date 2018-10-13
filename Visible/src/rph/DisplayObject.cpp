

#include "DisplayObject.h"

namespace rph{
    ci::vec2 DisplayObject2D::DisplayObject2D::getRegPointVec2(){
        return getRegPointVec2( getRect(), getRegPoint() );
    }
    
    ci::vec2 DisplayObject2D::getRegPointVec2(ci::Rectf rect, RegistrationPoint regPoint){
        switch(regPoint){
            case TOPLEFT:
                return ci::vec2(0);
            case TOPCENTER:
                return ci::vec2(-rect.getWidth()/2, 0);
            case TOPRIGHT:
                return ci::vec2(-rect.getWidth(), 0);
            case CENTERLEFT:
                return ci::vec2(0, -rect.getHeight()/2);
            case CENTERCENTER:
                return ci::vec2(-rect.getWidth()/2, -rect.getHeight()/2);
            case CENTERRIGHT:
                return ci::vec2(-rect.getWidth(), -rect.getHeight()/2);
            case BOTTOMLEFT:
                return ci::vec2(0, -rect.getHeight());
            case BOTTOMCENTER:
                return ci::vec2(-rect.getWidth()/2, -rect.getHeight());
            case BOTTOMRIGHT:
                return ci::vec2(-rect.getWidth(), -rect.getHeight());
        }
        return ci::vec2(0);
    }
    
    ci::vec2 DisplayObject2D::getRegPointVec2(ci::vec2 size, RegistrationPoint regPoint){
        switch(regPoint){
            case TOPLEFT:
                return ci::vec2(0);
            case TOPCENTER:
                return ci::vec2(-size.x/2, 0);
            case TOPRIGHT:
                return ci::vec2(-size.x, 0);
            case CENTERLEFT:
                return ci::vec2(0, -size.y/2);
            case CENTERCENTER:
                return ci::vec2(-size.x/2, -size.y/2);
            case CENTERRIGHT:
                return ci::vec2(-size.x, -size.y/2);
            case BOTTOMLEFT:
                return ci::vec2(0, -size.y);
            case BOTTOMCENTER:
                return ci::vec2(-size.x/2, -size.y);
            case BOTTOMRIGHT:
                return ci::vec2(-size.x, -size.y);
        }
        return ci::vec2(0);
    }
    
    DisplayObject2D::RegistrationPoint DisplayObject2D::getRegPoint( std::string regPoint ){
        if( regPoint == "TOPLEFT"){
            return TOPLEFT;
        }else if( regPoint == "TOPCENTER"){
            return TOPCENTER;
        }else if( regPoint == "TOPRIGHT"){
            return TOPRIGHT;
        }else if( regPoint == "CENTERLEFT"){
            return CENTERLEFT;
        }else if( regPoint == "CENTERCENTER"){
            return CENTERCENTER;
        }else if( regPoint == "CENTERRIGHT"){
            return CENTERRIGHT;
        }else if( regPoint == "BOTTOMLEFT"){
            return BOTTOMLEFT;
        }else if( regPoint == "BOTTOMCENTER"){
            return BOTTOMCENTER;
        }else if( regPoint == "BOTTOMRIGHT"){
            return BOTTOMRIGHT;
        }else{
            return TOPLEFT;
        }
    }
}
