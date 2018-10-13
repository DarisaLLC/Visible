//
//  cvMatCerealSerialization.h
//  Visible
//
//  Created by Arman Garakani on 9/21/18.
//

#ifndef cvMatCerealSerialization_h
#define cvMatCerealSerialization_h

#include "app_core.h"
#include <opencv2/opencv.hpp>
#include <cereal/cereal.hpp>

VISIBLE_BEGIN_NAMESPACE(cv)

template <class Archive>
void save(Archive& ar, const cv::Mat& mat, const std::uint32_t version)
{
    int rows, cols, type;
    bool continuous;
    
    rows = mat.rows;
    cols = mat.cols;
    type = mat.type();
    continuous = mat.isContinuous();
    
    ar& rows& cols& type& continuous;
    
    if (continuous)
    {
        const int data_size = rows * cols * static_cast<int>(mat.elemSize());
        auto mat_data = cereal::binary_data(mat.ptr(), data_size);
        ar& mat_data;
    }
    else
    {
        const int row_size = cols * static_cast<int>(mat.elemSize());
        for (int i = 0; i < rows; i++)
        {
            auto row_data = cereal::binary_data(mat.ptr(i), row_size);
            ar& row_data;
        }
    }
};

template <class Archive>
void load(Archive& ar, cv::Mat& mat, const std::uint32_t version)
{
    int rows, cols, type;
    bool continuous;
    
    ar& rows& cols& type& continuous;
    
    if (continuous)
    {
        mat.create(rows, cols, type);
        const int data_size = rows * cols * static_cast<int>(mat.elemSize());
        auto mat_data = cereal::binary_data(mat.ptr(), data_size);
        ar& mat_data;
    }
    else
    {
        mat.create(rows, cols, type);
        const int row_size = cols * static_cast<int>(mat.elemSize());
        for (int i = 0; i < rows; i++)
        {
            auto row_data = cereal::binary_data(mat.ptr(i), row_size);
            ar& row_data;
        }
    }
};

VISIBLE_END_NAMESPACE(cv)

#endif /* cvMatCerealSerialization_h */
