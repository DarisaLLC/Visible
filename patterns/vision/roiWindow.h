
#ifndef _WINDOW_H_
#define _WINDOW_H_


#include <iostream>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <ctime> // std::time
#include "roiRoot.h"
#include "rectangle.h"
#include <mutex>


using namespace std;

namespace DS
{


template <typename T>
class roiWindow
{

public:
    typedef iRect rect_t;
    typedef rect_t::point_t point_t;
    typedef T traits_t;
    typedef typename PixelType<T>::pixel_t pixel_t;
    typedef pixel_t * pixel_ptr_t;
    typedef int64_t timestamp_t;

    // Constructors
    //! default constructor takes no arguments
    /*!
         */
    roiWindow()
        : m_frame_buf(0), m_bounds()
    {
        update_ipl_roi();
    }


    //! copy / assignment  takes one arguments
    /*!
         \param other
         \desc  Copy and assignment constructors increment the refcount
         */

    roiWindow(const roiWindow & other)
        : m_frame_buf(other.frameBuf()), m_bounds(other.m_bounds)
    {
        update_ipl_roi();
    }


    //! copy / assignment  takes one arguments
    /*!
         \param rhs
         \desc  Copy and assignment constructors increment the refcount
         */

    const roiWindow & operator=(const roiWindow & rhs)
    {
        if (this == &rhs) return *this;

        m_frame_buf = rhs.frameBuf();
        m_bounds = rhs.bound();
        update_ipl_roi();
        return *this;
    }


    /*
         * Create a qWindow with an underlying frame using location and size of the
         * supplied rectangle
         */
    roiWindow(iRect & bound, image_memory_alignment_policy im = image_memory_alignment_policy::align_every_row)
    {
        int w = bound.ul().first + bound.width();
        int h = bound.ul().second + bound.height();
        if (w >= bound.width() && h >= bound.height())
        {
            sharedRoot<T> ptr = new root<T>(w, h, im);
            m_frame_buf = ptr;
            m_bounds = iRect(bound.ul().first, bound.ul().second, w, h);
            update_ipl_roi();
        }
        else
        {
            //	  rmExceptionMacro(<< "IP Window::init");
        }
    }

    roiWindow(int w, int h, image_memory_alignment_policy im = image_memory_alignment_policy::align_every_row)
    {
        sharedRoot<T> ptr = new root<T>(w, h, im);
        m_frame_buf = ptr;
        m_bounds = iRect(0, 0, w, h);
        update_ipl_roi();
    }

    roiWindow(sharedRoot<T> ptr, const iRect & Rect)
    {
        m_bounds = Rect;
        if (ptr->bounds().contains(m_bounds))
        {
            m_frame_buf = ptr;
            update_ipl_roi();
        }
    }


    roiWindow(sharedRoot<T> ptr, int32_t x, int32_t y, int32_t width, int32_t height)
    {
        m_bounds = iRect(x, y, width, height);
        if (ptr->bounds().contains(m_bounds))
        {
            m_frame_buf = ptr;
            update_ipl_roi();
        }
    }


    roiWindow(const roiWindow<T> &, int32_t x, int32_t y, int32_t width, int32_t height);


    roiWindow(sharedRoot<T> ptr)
    {
        m_frame_buf = ptr;
        m_bounds = iRect(0, 0, ptr->width(), ptr->height());
        update_ipl_roi();
    }


    // Destructor
    virtual ~roiWindow() {}

    // Accessors
    const sharedRoot<T> & frameBuf() const { return m_frame_buf; }
    sharedRoot<T> & frameBuf() { return m_frame_buf; }

    int32_t depth() const { return m_frame_buf->depth(); }
    uint32_t n() const { return m_bounds.area(); }

    const iPair & position() { return m_bounds.origin(); }

    int32_t x() const { return m_bounds.origin().first; }
    int32_t y() const { return m_bounds.origin().second; }

    int32_t width() const { return m_bounds.width(); }
    int32_t height() const { return m_bounds.height(); }
    double aspect() const { return static_cast<double>(width()) / static_cast<double>(height()); }
    bool contains(int & x, int & y) const { return m_bounds.contains(x, y); }
    bool contains(iRect & rect) const { return m_bounds.contains(rect); }

    iPair size() const { return iPair(width(), height()); }

    const iRect & bound() const { return m_bounds; }
    iRect frame() const { return m_frame_buf->bounds(); }
    iRect sizeRectangle() const { return iRect(0, 0, width(), height()); }

    // Is this window bound ?
    bool isBound() const
    {
        bool rtn = m_frame_buf != 0 && m_frame_buf->bounds().contains(m_bounds);
        return rtn;
    }


    //! isWithin / isWithin takes a position in the frameBuf coordinate
    int32_t bits() const { return T::bits(); }
    int32_t bytes() const { return T::bits(); }


    pixel_t getPixel(int32_t px, int32_t py) const
    {
        assert(!(px < 0 || py < 0 || !isBound())); // throw window_geometry_error("getPixel_pair");
        return m_frame_buf->getPixel(px + x(), py + y());
    }

    pixel_t getPixel(const iPair & where) const
    {
        assert(!(where.x() < 0 || where.y() < 0 || !isBound()));
        return m_frame_buf->getPixel(where.first + x(), where.second + y());
    }

    const pixel_ptr_t rowPointer(int32_t row) const
    {
        return (m_frame_buf->rowPointer(row + y()) + x());
    }

    pixel_ptr_t rowPointer(int32_t row)
    {
        return (m_frame_buf->rowPointer(row + y()) + x());
    }

    const pixel_ptr_t pelPointer(int32_t col, int32_t row) const
    {
        return (m_frame_buf->rowPointer(row + y()) + x() + col);
    }

    pixel_ptr_t pelPointer(int32_t col, int32_t row)
    {
        return (m_frame_buf->rowPointer(row + y()) + x() + col);
    }

    virtual int32_t rowUpdate() const { return m_frame_buf->rowUpdate(); }
    virtual int32_t rowPixelUpdate() const { return m_frame_buf->rowPixelUpdate(); }

    inline bool sameTime(const roiWindow & other) const
    {
        return frameBuf()->timestamp() == other.frameBuf()->timestamp();
    }

    // Return (creation) time stamp
    inline timestamp_t timestamp() const
    {
        return frameBuf()->timestamp();
    }


    std::pair<pixel_t, pixel_t> range() const
    {
        std::pair<pixel_t, pixel_t> extremes;
        extremes.first = std::numeric_limits<pixel_t>::max();
        extremes.second = std::numeric_limits<pixel_t>::min();
        for (int32_t j = 0; j < height(); j++)
        {
            pixel_ptr_t row = rowPointer(j);
            for (int32_t i = 0; i < width(); i++, row++)
            {
                pixel_t pval = *row;
                if (pval > extremes.second) extremes.second = pval;
                if (pval < extremes.first) extremes.first = pval;
            }
        }
        return extremes;
    }

    std::pair<pixel_t, iPair> maxValueLocation() const
    {
        iPair loc;
        pixel_t maxv = std::numeric_limits<pixel_t>::min();
        for (int32_t j = 0; j < height(); j++)
        {
            pixel_ptr_t row = rowPointer(j);
            for (int32_t i = 0; i < width(); i++, row++)
            {
                pixel_t pval = *row;
                if (pval > maxv)
                {
                    maxv = pval;
                    loc.first = i;
                    loc.second = j;
                }
            }
        }
        return make_pair(maxv, loc);
    }

    // become an entire window to this framebuf
    void entire()
    {
        m_bounds = m_frame_buf->bounds();
        update_ipl_roi();
    }

    roiWindow<T> window(int32_t tl_x, int32_t tl_y, int32_t width, int32_t height);

    bool step(const iPair & tp)
    {
        iRect cp(m_bounds);
        cp.translate(tp);
        bool ok = m_frame_buf->bounds().contains(cp);
        if (!ok) return ok;
        m_bounds.translate(tp);
        return true;
    }


    // Set pixel value for (x,y). Return new value
    pixel_t setPixel(int32_t px, int32_t py, pixel_t val = pixel_t(0))
    {
        assert(!(px < 0 || py < 0 || !isBound()));
        m_frame_buf->setPixel(px + x(), py + y(), val);
        return val;
    }

    void set(pixel_t val = pixel_t(0));
    void setBorder(int pad, pixel_t clear_val = pixel_t(0));


    bool copy_pixels_from(pixel_ptr_t pixels, int columns, int rows, int stride)
    {
        bool sg = isBound() && pixels != nullptr && width() == columns && height() == rows;
        if (!sg) return !sg;
        pixel_ptr_t pels = pixels;
        for (int j = 0; j < height(); j++)
        {
            pixel_ptr_t our_row = rowPointer(j);
            std::memcpy (our_row, pels, columns);
            pels += stride;
        }
        return true;
    }


    bool copy_pixels_to(roiWindow<T> & other)
    {
        bool sg = isBound() && other.isBound() && width() == other.width() && height() == other.height();
        if (!sg) return !sg;
        for (int j = 0; j < height(); j++)
        {
            pixel_ptr_t our_row = rowPointer(j);
            pixel_ptr_t other_row = other.rowPointer(j);
            std::memcpy(other_row, our_row, width());
        }
        return true;
    }

    void print_pixel()
    {
        // = first < second
        iPair range_j(0, height());
        iPair range_i(0, width());

        std::cout << std::endl;
        //        cout.setf(ios::hex, ios::basefield);

        for (int j = range_j.first; j < range_j.second; j++)
        {
            pixel_ptr_t row_ptr = rowPointer(j);
            std::string ps;
            for (int i = range_i.first - range_i.first; i < range_i.second - range_i.first; i++)
            {
                pixel_t pel = row_ptr[i];
                std::cout << setfill(' ') << setw(3);
                std::cout << (int)pel;
            }
            std::cout << std::endl;
        }
        //        cout.unsetf(ios::hex);
    }
    // Comparison Functions
    bool operator==(const roiWindow & other) const
    {
        return (m_frame_buf == other.m_frame_buf && m_bounds == other.bound());
    }
    /*
         effect     Returns true if the two windows have the same frameBuf
         and have the same size, offset and root-offset.
         note       If both windows are unbound, but have the same offset,
         root-offset and size, returns true.
         */
    bool operator!=(const roiWindow & other) const
    {
        return !operator==(other);
    }


protected:
    sharedRoot<T> m_frame_buf; // Ref-counted pointer to frame buffer

    iRect m_bounds;
    void update_ipl_roi()
    {
    }


};


}

#endif // _WINDOW_H_
