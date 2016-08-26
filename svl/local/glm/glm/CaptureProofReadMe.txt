GLM is a header only library. Hence, there is nothing to build to use it. To use GLM, a programmer only has to include <glm/glm.hpp> in his program. This include provides all the GLSL features implemented by GLM.
Core GLM features can be included using individual headers to allow faster user program compilations.
<glm/vec2.hpp>: vec2, bvec2, dvec2, ivec2 and uvec2
<glm/vec3.hpp>: vec3, bvec3, dvec3, ivec3 and uvec3
<glm/vec4.hpp>: vec4, bvec4, dvec4, ivec4 and uvec4
<glm/mat2x2.hpp>: mat2, dmat2
<glm/mat2x3.hpp>: mat2x3, dmat2x3
<glm/mat2x4.hpp>: mat2x4, dmat2x4
<glm/mat3x2.hpp>: mat3x2, dmat3x2
<glm/mat3x3.hpp>: mat3, dmat3
<glm/mat3x4.hpp>: mat3x4, dmat2
<glm/mat4x2.hpp>: mat4x2, dmat4x2
<glm/mat4x3.hpp>: mat4x3, dmat4x3
<glm/mat4x4.hpp>: mat4, dmat4
<glm/common.hpp>: all the GLSL common functions <glm/exponential.hpp>: all the GLSL exponential functions <glm/geometry.hpp>: all the GLSL geometry functions <glm/integer.hpp>: all the GLSL integer functions
<glm/matrix.hpp>: all the GLSL matrix functions
<glm/packing.hpp>: all the GLSL packing functions <glm/trigonometric.hpp>: all the GLSL trigonometric functions <glm/vector_relational.hpp>: all the GLSL vector relational functions
1.2. Faster program compilation
GLM is a header only library that makes a heavy usage of C++ templates. This design may significantly increase the compile time for files that use GLM. Hence, it is important to limit GLM inclusion to header and source files that actually use it. Likewise, GLM extensions should be included only in program sources using them.
To further help compilation time, GLM 0.9.5 introduced <glm/fwd.hpp> that provides forward declarations of GLM types.
1.3. Use sample of GLM core
// Header file
#include <glm/fwd.hpp>
// Source file
#include <glm/glm.hpp>
// Include GLM core features
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
// Include GLM extensions
#include <glm/gtc/matrix_transform.hpp>
glm::mat4 transform(
       glm::vec2 const & Orientation,
       glm::vec3 const & Translate,
       glm::vec2 const & Up)
{
}
glm::mat4 Projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f); glm::mat4 ViewTranslate = glm::translate(glm::mat4(1.0f), Translate); glm::mat4 ViewRotateX = glm::rotate(ViewTranslate, Orientation.y, Up); glm::mat4 View = glm::rotate(ViewRotateX, Orientation.x, Up);
glm::mat4 Model = glm::mat4(1.0f);
return Projection * View * Model;
1.4. Dependencies
When <glm/glm.hpp> is included, GLM provides all the GLSL features it implements in C++.
There is no dependence with external libraries or external headers such as gl.h, glcorearb.h, gl3.h, glu.h or windows.h. However, if <boost/static_assert.hpp> is included, Boost static assert will be used all over GLM code to provide compiled time errors unless GLM is built with a C++ 11 compiler in which case static_assert. If neither are detected, GLM will rely on its own implementation of static assert.
