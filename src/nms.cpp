//
//  nms.cpp
//  Visible
//
//  Created by Arman Garakani on 2/2/21.
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#include "nms.hpp"

/*
 From OpenCv
 
 In addition to the class members, the following operations on rectangles are implemented:
 -   \f$\texttt{rect} = \texttt{rect} \pm \texttt{point}\f$ (shifting a rectangle by a certain offset)
 -   \f$\texttt{rect} = \texttt{rect} \pm \texttt{size}\f$ (expanding or shrinking a rectangle by a
 certain amount)
 -   rect += point, rect -= point, rect += size, rect -= size (augmenting operations)
 -   rect = rect1 & rect2 (rectangle intersection)
 -   rect = rect1 | rect2 (minimum area rectangle containing rect1 and rect2 )
 -   rect &= rect1, rect |= rect1 (and the corresponding augmenting operations)
 -   rect == rect1, rect != rect1 (rectangle comparison)
 
 This is an example how the partial ordering on rectangles can be established (rect1 \f$\subseteq\f$
 rect2):
 @code
 template<typename _Tp> inline bool
 operator <= (const Rect_<_Tp>& r1, const Rect_<_Tp>& r2)
 {
 return (r1 & r2) == r1;
 }
 @endcode
 For your convenience, the Rect_\<\> alias is available: cv::Rect
 
*/
/**
 * @brief nms
 * Non maximum suppression
 * @param srcRects
 * @param resRects
 * @param thresh
 * @param neighbors
 */
void nms(const std::vector<cv::Rect>& srcRects,
                std::vector<cv::Rect>& resRects,
                float thresh,
                int neighbors)
{
    resRects.clear();
    
    const size_t size = srcRects.size();
    if (!size)
        return;
    
        // Sort the bounding boxes by the bottom - right y - coordinate of the bounding box
    std::multimap<int, size_t> idxs;
    for (size_t i = 0; i < size; ++i)
    {
    idxs.emplace(srcRects[i].br().y, i);
    }
    
        // keep looping while some indexes still remain in the indexes list
    while (idxs.size() > 0)
        {
            // grab the last rectangle
        auto lastElem = --std::end(idxs);
        const cv::Rect& rect1 = srcRects[lastElem->second];
        
        int neigborsCount = 0;
        
        idxs.erase(lastElem);
        
        for (auto pos = std::begin(idxs); pos != std::end(idxs); )
            {
                // grab the current rectangle
            const cv::Rect& rect2 = srcRects[pos->second];
            
            float intArea = static_cast<float>((rect1 & rect2).area());
            float unionArea = rect1.area() + rect2.area() - intArea;
            float overlap = intArea / unionArea;
            
                // if there is sufficient overlap, suppress the current bounding box
            if (overlap > thresh)
                {
                pos = idxs.erase(pos);
                ++neigborsCount;
                }
            else
                {
                ++pos;
                }
            }
        if (neigborsCount >= neighbors)
            resRects.push_back(rect1);
        }
}

/**
 * @brief nms2
 * Non maximum suppression with detection scores
 * @param srcRects
 * @param scores
 * @param resRects
 * @param thresh
 * @param neighbors
 */
void nms2(const std::vector<cv::Rect>& srcRects,
                 const std::vector<float>& scores,
                 std::vector<cv::Rect>& resRects,
                 float thresh,
                 int neighbors,
                 float minScoresSum)
{
    resRects.clear();
    
    const size_t size = srcRects.size();
    if (!size)
        return;
    
    assert(srcRects.size() == scores.size());
    
        // Sort the bounding boxes by the detection score
    std::multimap<float, size_t> idxs;
    for (size_t i = 0; i < size; ++i)
    {
    idxs.emplace(scores[i], i);
    }
    
        // keep looping while some indexes still remain in the indexes list
    while (idxs.size() > 0)
        {
            // grab the last rectangle
        auto lastElem = --std::end(idxs);
        const cv::Rect& rect1 = srcRects[lastElem->second];
        
        int neigborsCount = 0;
        float scoresSum = lastElem->first;
        
        idxs.erase(lastElem);
        
        for (auto pos = std::begin(idxs); pos != std::end(idxs); )
            {
                // grab the current rectangle
            const cv::Rect& rect2 = srcRects[pos->second];
            
            float intArea = static_cast<float>((rect1 & rect2).area());
            float unionArea = rect1.area() + rect2.area() - intArea;
            float overlap = intArea / unionArea;
            
                // if there is sufficient overlap, suppress the current bounding box
            if (overlap > thresh)
                {
                scoresSum += pos->first;
                pos = idxs.erase(pos);
                ++neigborsCount;
                }
            else
                {
                ++pos;
                }
            }
        if (neigborsCount >= neighbors && scoresSum >= minScoresSum)
            resRects.push_back(rect1);
        }
}


#pragma GCC diagnostic pop
