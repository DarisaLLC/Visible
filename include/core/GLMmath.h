//
//  GLMmath.h
//  svl
//


#pragma once

#include <cmath>
#include <cstring>
#include <iostream>
#include <cassert>
#include <limits>
#include "core.hpp"


#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/io.hpp"
#include "glm/gtx/norm.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtc/type_ptr.hpp"

using namespace std;

namespace svl
{
    
    using glm::quat;
    using glm::dquat;
    
    
        using glm::vec2;
        using glm::vec3;
        using glm::vec4;
        using glm::ivec2;
        using glm::ivec3;
        using glm::ivec4;
        using glm::uvec2;
        using glm::uvec3;
        using glm::uvec4;
        using glm::dvec2;
        using glm::dvec3;
        using glm::dvec4;
        
        //! Returns a vector which is orthogonal to \a vec
        template<typename T, glm::precision P>
        glm::tvec3<T, P> orthogonal( const glm::tvec3<T, P> &vec )
        {
            if( math<T>::abs( vec.y ) < (T)0.99 ) // abs(dot(u, Y)), somewhat arbitrary epsilon
                return glm::tvec3<T, P>( -vec.z, 0, vec.x ); // cross( this, Y )
            else
                return glm::tvec3<T, P>( 0, vec.z, -vec.y ); // cross( this, X )
        }
        
        
        template<uint8_t DIM,typename T> struct VECDIM { };
        
        template<> struct VECDIM<2,float>	{ typedef vec2	TYPE; };
        template<> struct VECDIM<3,float>	{ typedef vec3	TYPE; };
        template<> struct VECDIM<4,float>	{ typedef vec4	TYPE; };
        template<> struct VECDIM<2,double>	{ typedef dvec2	TYPE; };
        template<> struct VECDIM<3,double>	{ typedef dvec3	TYPE; };
        template<> struct VECDIM<4,double>	{ typedef dvec4	TYPE; };
        template<> struct VECDIM<2,int>		{ typedef ivec2	TYPE; };
        template<> struct VECDIM<3,int>		{ typedef ivec3	TYPE; };
        template<> struct VECDIM<4,int>		{ typedef ivec4	TYPE; };
    
}
