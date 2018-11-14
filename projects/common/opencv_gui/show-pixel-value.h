#ifndef OPENCV_SHOW_PIXEL_VALUE_H
#define OPENCV_SHOW_PIXEL_VALUE_H

#include <opencv2/core/core.hpp>
#include <unordered_set>

// A wrapper for cv::imshow that shows individual pixel values on mouse hover
class ShowPixelValue {
public:
    // Show the input image in a window named windowName.
    ShowPixelValue(const std::string& windowName, const cv::Mat& input);
    ~ShowPixelValue();

    void KeepUpdating(ShowPixelValue* other);
    void StopUpdating(ShowPixelValue* other);

private:
    const cv::Mat input;
    const std::string windowName;

    cv::Mat output;

    std::unordered_set<ShowPixelValue*> updateTargets;

    void Update(int mouseX, int mouseY, bool showPoint);
    void UpdateOthers(int mouseX, int mouseY);
    static void OnMouse(int event, int mouseX, int mouseY, int flags, void* userdata);
};

#endif // OPENCV_SHOW_PIXEL_VALUE_H