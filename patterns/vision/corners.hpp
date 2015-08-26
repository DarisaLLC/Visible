#ifndef __CORNERS__
#define __CORNERS__

#include <exception>

#include <array>
#include <vector>
#include "roiWindow.h"

using namespace DS;

float corner_assess(roiWindow<P8U>& peaks, roiWindow<P8U>& ang)
{
    fPair center;
    float score = 0.0f;

    if (peaks.isBound() && ang.isBound())
    {
        int length = peaks.width() * peaks.width() + ang.width() * ang.width();
        assert(length != 0);

        if (GetMotionCenter(peaks, ang, center))
        {
            int x = peaks.bound().ul().first;
            int y = peaks.bound().ul().second;
            float dist1st = center.first - x;
            float dist2nd = center.second - y;
            float dist = dist1st * dist1st + dist2nd * dist2nd;
            score = 1.0f - dist / length;
        }
    }
    return score;
}



void PeakDetect(const roiWindow<P8U>& space, std::vector<iPair>& peaks)
{

    // Make sure it is empty
    peaks.resize(0);

    const int rowUpdate(space.rowUpdate());
    int height = space.height() - 3;
    int width = space.width() - 3;
    int accept = 10;
    for (int row = 3; row < height; row++)
    {
        uint8_t* row_ptr = space.pelPointer(3, row);
        uint8_t* pel = row_ptr;
        for (int col = 3; col < width; col++, pel++)
        {
            if (*pel >= accept &&
                *pel > *(pel - 1) &&
                *pel > *(pel - rowUpdate - 1) &&
                *pel > *(pel - rowUpdate) &&
                *pel > *(pel - rowUpdate + 1) &&
                *pel > *(pel + 1) &&
                *pel > *(pel + rowUpdate + 1) &&
                *pel > *(pel + rowUpdate) &&
                *pel > *(pel + rowUpdate - 1))
            {
                iPair dp(col, row);
                peaks.push_back(dp);
            }
        }
    }
}


template<typename P>
int skip(uint8_t*current, P dummy)
{
    if (*((P*)current) == P(0) ) return sizeof(P);
}


int skip_none_edge(uint8_t* current, int limit)
{
    if (limit > sizeof(uint64_t) && *((uint64_t*)current) == 0) return sizeof(uint64_t);
    if (limit > sizeof(uint64_t) && *((uint32_t*)current) == 0) return sizeof(uint32_t);
    if (limit > sizeof(uint64_t) && *((uint16_t*)current) == 0) return sizeof(uint16_t);
    if (limit > sizeof(uint64_t) && *((uint8_t*)current) == 0) return sizeof(uint8_t);
    return 0;
}

void generate_field8(const roiWindow<P8U>& src, roiWindow<P8U>& field, std::vector<iPair>& peaks)
{
    int width = src.width();
    int height = src.height();
    if (!field.isBound())
    {
        field = roiWindow<P8U>(width, height);
    }
    field.set();

    std::vector<roiWindow<P8U> > mGradMaps;
    for (int i = 0; i < 3; i++)
    {
        mGradMaps.push_back(roiWindow<P8U>(width, height));
    }

    Gradient(src, mGradMaps[0], mGradMaps[1]);
    roiWindow<P8U> smooth(width, height);
    Gauss3by3(mGradMaps[0], smooth);
    Gauss3by3(smooth, field);

    for (int hh = 0; hh < height; hh++)
    {
        uint8_t* row_ptr = mGradMaps[0].pelPointer(0, hh);
        uint8_t* edge_ptr = row_ptr;
        uint8_t* field_row_ptr = field.pelPointer(0, hh);
        uint8_t* field_ptr = field_row_ptr;

        for (int ii = 0; ii < width; ii++, edge_ptr++, field_ptr++)
        {
            int16_t gauss = *field_ptr;
            int16_t hole = *edge_ptr;
            if (gauss <= hole) hole = 0;
            else hole = gauss - hole;
            *field_ptr = (uint8_t)hole ;
        }
    }

    PeakDetect(field, peaks);
}




void generate_field(shared_ptr<uint16_t>& buffer, int width, int height, roiWindow<P8U>& field,  std::vector<iPair>& peaks)
{
    roiWindow < P8U> s8u = convert(buffer.get(), width, height);
    generate_field8(s8u, field, peaks);
}


void generate_field(roiWindow<P16U>& buffer, roiWindow<P8U>& field,  std::vector<iPair>& peaks)
{
    roiWindow < P8U> s8u = convert(buffer.rowPointer(0), buffer.width(), buffer.height());
    generate_field8(s8u, field, peaks);
}




#if 0
    iPair roi_size(64, 64);
    iPair roi_half_size(roi_size.first / 2, roi_size.second / 2);
    iPair col_range(roi_size.first / 2, width - roi_size.first / 2);
    iPair row_range(roi_size.second / 2, height - roi_size.second / 2);
    
    try
    {
        uint32_t row = row_range.first;
        do
        {
            uint8_t* row_ptr = mGradMaps[2].pelPointer(0, row);
            uint8_t* edge_ptr = row_ptr;
            uint32_t col = col_range.first;
            int inc = 0;
            do
            {
                inc = skip_none_edge(edge_ptr, col_range.second - col);

                if (inc == 0)
                {
                    iPair loc(col - roi_half_size.first, row - roi_half_size.second);
                    roiWindow<P8U> mag(mGradMaps[2], loc.first, loc.second, roi_size.first, roi_size.second);
                    roiWindow<P8U> ang(mGradMaps[1], loc.first, loc.second, roi_size.first, roi_size.second);
                    float score = corner_assess(mag, ang);
                    field.setPixel(col, row, (uint8_t)(256 * score));
                    inc = 1;
                }
                col += inc;
                edge_ptr += inc;
            }
            while (col < col_range.second);
            row++;
        }
        while (row < row_range.second);
    }
    catch (std::exception & e)
    {
        std::cout << e.what() << std::endl;
    }
#endif




#endif
