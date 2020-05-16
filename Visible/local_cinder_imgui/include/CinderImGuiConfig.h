//
//  CinderImGuiConfig.h
//  Visible
//
//  Created by Arman Garakani on 5/15/20.
//

#ifndef CinderImGuiConfig_h
#define CinderImGuiConfig_h


#include "cinder/Color.h"
#include "cinder/Vector.h"
#include "cinder/CinderAssert.h"
#include "cinder/gl/platform.h"


//---- Define assertion handler. Defaults to calling assert().
// If your macro uses multiple statements, make sure is enclosed in a 'do { .. } while (0)' block so it can be used as a single statement.
#define IM_ASSERT(_EXPR)  CI_ASSERT(_EXPR)
//#define IM_ASSERT(_EXPR)  ((void)(_EXPR))     // Disable asserts

//---- Define attributes of all API symbols declarations, e.g. for DLL under Windows
// Using dear imgui via a shared library is not recommended, because of function call overhead and because we don't guarantee backward nor forward ABI compatibility.
#define IMGUI_API CI_API

// Custom implicit cast operators
// Custom implicit cast operators
#ifndef CINDER_IMGUI_NO_IMPLICIT_CASTS
#define IM_VEC2_CLASS_EXTRA                                            \
ImVec2(const ci::vec2& f) { x = f.x; y = f.y; }                        \
operator ci::vec2() const { return ci::vec2(x,y); }                    \
ImVec2(const ci::ivec2& f) { x = static_cast<float>( f.x ); y = static_cast<float>( f.y ); }  \
operator ci::ivec2() const { return ci::ivec2(x,y); }

#define IM_VEC4_CLASS_EXTRA                                            \
ImVec4(const glm::vec4& f) { x = f.x; y = f.y; z = f.z; w = f.w; }     \
operator glm::vec4() const { return ci::vec4(x,y,z,w); }               \
ImVec4(const ci::ColorA& f) { x = f.r; y = f.g; z = f.b; w = f.a; }    \
operator ci::ColorA() const { return ci::ColorA(x,y,z,w); }            \
ImVec4(const ci::Color& f) { x = f.r; y = f.g; z = f.b; w = 1.0f; }    \
operator ci::Color() const { return ci::Color(x,y,z); }
#endif
#endif /* CinderImGuiConfig_h */
