#ifndef _algo_h
#define _algo_h


// algo.h    Stand-alone algorithm methods

#include "types.h"

#include <opencv2/features2d/features2d.hpp>

typedef std::vector<cv::KeyPoint> Keypoints;
typedef cv::Mat Descriptors;
typedef std::vector<cv::DMatch> Matches; 
typedef std::vector<Matches> VectorMatches;

typedef std::vector<Descriptors> VectorDescriptors;

typedef std::vector<cv::Mat> Mats;
typedef std::vector<cv::Point2f> Point2fs;
typedef std::vector<cv::Point2d> Point2ds;
typedef std::vector<cv::Point3f> Point3fs;
typedef std::vector<cv::Point3d> Point3ds;


// *****************
// *               *
// *  AlgoMatcher  *
// *               *
// *****************

// Low-level matching between two sets of keypoints

struct AlgoMatcherResults
{
  Keypoints image_keypoints;     // Image keypoints
  Descriptors image_descriptors; // Image descriptors

  Keypoints logo_keypoints;      // Logo keypoints
  Descriptors logo_descriptors;  // Logo descriptors

  VectorMatches match1_2;  // knn matching between image 1 and 2
  VectorMatches match2_1;  // knn matching between image 2 and 2
  Matches matches_sym;     // Symmetrical matches between 1 and 2
  Matches matches_fm;      // Matches from fundamental matrix
  Matches matches_hom;     // Matches of points which maps closely with homography
  Matches matches;         // Matches between 1 and 2
 cv::Mat fundemental;     // Fundamental matrix

  cv::Mat homography;      // Homography
  double determinant;
  double svd_ratio;

  AlgoMatcherResults () : determinant (0.), svd_ratio (0.) {}
};


struct AlgoDetectResults
{
  Keypoints keypoints;     // Keypoints
  Descriptors descriptors; // Descriptors

  double elapsed_keypoints;    // Time (msec) to compute keypoints
  double elapsed_descriptors;  // Time (msec) to compute descriptors

  AlgoDetectResults () : elapsed_keypoints(0.), elapsed_descriptors(0.) {}
};


class AlgoMatcher
{
public:
  AlgoMatcher (const cv::Mat& image);
  AlgoMatcher (const Keypoints& image_keypoints, const Descriptors& image_descriptors);

  AlgoMatcherResults match (const Keypoints& logo_keypoints, const Descriptors& logo_descriptors, bool use_clustering = false, float ratio_value = 0.75f);

  // Compute the keypoints and descriptors for an image (grayscale)
  static AlgoDetectResults detect (const cv::Mat& image);

  // Normalize a descriptor to unit length
  static void normalize (Descriptors& descriptors);

  // Identify good matches using RANSAC. Return fundemental matrix. The matrix isn't used
  static cv::Mat ransacTest (const Matches& matches, const Keypoints& keypoints1, const Keypoints& keypoints2, Matches& outMatches, double distance=3.0, double confidence=0.80, bool refineF = true);

  // Clear matches for which NN ratio is > than threshold. return the number of removed points 
  // (corresponding entries being cleared, i.e. size will be 0) 
  static int ratioTest (VectorMatches &matches, float ratio = 0.75f); // was 0.70f);

  // Insert symmetrical matches in symMatches vector 
  static void symmetryTest (const VectorMatches& matches1, const VectorMatches& matches2, Matches& symMatches);

protected:
  AlgoMatcherResults results_;

  // Build the feature detector objects (if needed)
  static void build_objects();

  // Pointer to the feature point detector object 
  static cv::Ptr<cv::FeatureDetector> detector_; 

  // Pointer to the feature descriptor extractor object 
  static cv::Ptr<cv::DescriptorExtractor> extractor_; 

  // Pointer to the matcher object (robust matcher only. For flann matcher we need a separate matcher for each color channel)
  static cv::Ptr<cv::DescriptorMatcher> matcher_; 

};


// ********************
// *                  *
// *  AlgoHomography  *
// *                  *
// ********************

// Tools for computing homography from points

struct AlgoHomographyResults
{
  cv::Mat H;            // Homography (either unintialized or 3x3) in pixel coordinates
  cv::Mat Hnorm;        // Homography (from normalized coordinates)
  double error;         // Average error (normalized coordinates)
  double error_act;     // Average error (actual coordinates)
  double determinant;   // Determinant of H
  double svd_ratio;     // Ratio of first and last eigenvalue of H
  double maxdistance;   // Maximum match distance (if available. This will be -1 if unknown)

  cv::Mat logo_xform;   // Xform from raw to normalized coordinates
  cv::Mat image_xform;

  Point2fs logopoints;      // Logo points which pass the homography (normalized coordinates)
  Point2fs imagepoints;     // Image points which pass the homography (normalized coordinates)
  Point2fs logopoints_raw;  // Logo points which pass the homography (pixel coordinates)
  Point2fs imagepoints_raw; // Image points which pass the homography (pixel coordinates)
  Matches  initialmatches;  // Input matches (only available if matches supplied in the ctor)
  Matches  finalmatches;    // Matches (only available if matches supplied in the ctor)
  Floats   distances;       // Match distances (if available. This might be missing)

  AlgoHomographyResults() : error (0.), error_act (0.), determinant (0.), svd_ratio (0.), maxdistance (-1.) {}
};

class AlgoHomography
{
public:
  AlgoHomography (const Keypoints& logo_keypoints, const Keypoints& image_keypoints, const Matches& matches);
  AlgoHomography (const Point2fs& logopoints, const Point2fs& imagepoints);

  // Compute the homography and related values using the specified threshold
  AlgoHomographyResults analyze (double repro_threshold = 6.0);

  // Normalize points for homography to improve numeric stability. Map the points so that the mean value is 0 and the size is 1.
  // Returns the transform to map points into the new space, and modifies the passed points
  static cv::Mat normalizePoints (Point2fs& points);

  // Compute total error / number of points. H and outputMask from findHomography
  static double homography_error (const Point2fs& src, const Point2fs& dst, const std::vector<unsigned char>& outputMask, const cv::Mat& H);

protected:
  Point2fs logopoints_, imagepoints_;          // logo and image points (in normalized coordinates)
  Point2fs logopoints_raw_, imagepoints_raw_;  // logo and image points (in original pixel coordinates)

  Matches  initialmatches;                     // Input matches (only if matches supplied in the ctor)
  Keypoints initial_logo_keypoints;
  Keypoints initial_image_keypoints;

  Floats   distances_;                         // Match distances (if we are supplied with Matches)
  cv::Mat  logo_xform_, image_xform_;          // Transformation from raw to normalized coordinates
};



#endif
