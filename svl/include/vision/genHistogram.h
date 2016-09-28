
#include <atomic>
#include "roiWindow.h"

template <typename P>
struct getHistogram
{
    typedef typename PixelType<P>::pixel_ptr_t ptr_t;
    getHistogram() {}

    void process(const roiWindow<P> & src, std::vector<uint32_t> & histogram)
    {
        int N = P::maximum();
        is_valid = false;
        histogram.resize(N + 1, 0);

        uint32_t lastRow = src.height() - 1, row = 0;
        const uint32_t opsPerLoop = 8;
        uint32_t unrollCnt = src.width() / opsPerLoop;
        uint32_t unrollRem = src.height() % opsPerLoop;

        for (; row <= lastRow; row++)
        {
            ptr_t pixelPtr = src.rowPointer(row);

            for (uint32_t touchCount = 0; touchCount < unrollCnt; touchCount++)
            {
                histogram[*pixelPtr++]++;
                histogram[*pixelPtr++]++;
                histogram[*pixelPtr++]++;
                histogram[*pixelPtr++]++;
                histogram[*pixelPtr++]++;
                histogram[*pixelPtr++]++;
                histogram[*pixelPtr++]++;
                histogram[*pixelPtr++]++;
            }

            for (uint32_t touchCount = 0; touchCount < unrollRem; touchCount++)
                histogram[*pixelPtr++]++;
        }
        is_valid = true;
    }
    std::atomic<bool> is_valid;
};
