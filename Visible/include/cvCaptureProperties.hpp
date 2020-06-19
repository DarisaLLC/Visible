//
//  cvCaptureProperties.h
//  Visible
//
//  Created by Arman Garakani on 6/16/20.
//

#ifndef cvCaptureProperties_h
#define cvCaptureProperties_h
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include <string>
#include <memory>
#include <map>

namespace cvProperties {
// ---------------------------- Structures ----------------------------------
struct Property {
    int id;
    double value;
};

enum Type {
    Camera, Movie
};

// ------------------------------ Opencv explicitation --------------------------------
const std::vector<int> GenerateAllPropertiesId();
const std::string GetPropertyName(int id);


// -------------------------------- Helper Methods ----------------------------------
namespace Helper {
Property Get(const cv::VideoCapture& cap, int property_id);
bool Set(cv::VideoCapture& cap, const Property& prop);

std::map<int, Property> GetAll(const cv::VideoCapture& cap);
} // Helper
} //

#endif /* cvCaptureProperties_h */
