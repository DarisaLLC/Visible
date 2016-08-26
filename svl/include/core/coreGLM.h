#pragma once

#include "glm/gtc/type_ptr.hpp"

#include "glm/gtx/io.hpp"
#include "glm/gtx/norm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/transform2.hpp"
#include "glm/gtx/euler_angles.hpp"

namespace cpcore {

using glm::length;
using glm::length2;
using glm::normalize;
using glm::distance;
using glm::distance2;
using glm::dot;
using glm::cross;
using glm::mix;
using glm::rotate;
using glm::translate;
using glm::inverse;
using glm::angleAxis;
using glm::angle;
using glm::axis;
using glm::scale;
using glm::value_ptr;
using glm::orientate3;
using glm::orientate4;
using glm::toMat4;

}
