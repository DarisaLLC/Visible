
#ifndef _WINDOW_H_
#define _WINDOW_H_


#include <iostream>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <ctime> // std::time
#include "roiRoot.h"
#include "core/rectangle.h"
#include <mutex>


using namespace std;

namespace svl
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
    typedef root<T> root_t;
    typedef std::shared_ptr<root_t> sharedRoot_t;
    

    // Constructors
    //! default constructor takes no arguments
    /*!
         */
    roiWindow()
        : m_frame_buf(0), m_bounds()
    {
    }


    //! copy / assignment  takes one arguments
    /*!
         \param other
         \desc  Copy and assignment constructors increment the refcount
         */

    roiWindow(const roiWindow & other)
        : m_frame_buf(other.frameBuf()), m_bounds(other.m_bounds)
    {
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
        return *this;
    }


    /*
         * Create a roiWindow with an underlying frame using location and size of the
         * supplied rectangle
         */
    roiWindow(iRect & bound, image_memory_alignment_policy im = image_memory_alignment_policy::align_every_row)
    {
        int w = bound.ul().first + bound.width();
        int h = bound.ul().second + bound.height();
        if (w >= bound.width() && h >= bound.height())
        {
            m_frame_buf = sharedRoot_t (new root<T>(w, h, im));
            m_bounds = iRect(bound.ul().first, bound.ul().second, w, h);
        }
        else
        {
            //	  rmExceptionMacro(<< "IP Window::init");
        }
    }

    roiWindow(int w, int h, image_memory_alignment_policy im = image_memory_alignment_policy::align_every_row)
    {
        m_frame_buf = sharedRoot_t (new root<T>(w, h, im));
        m_bounds = iRect(0, 0, w, h);
    }

    roiWindow(sharedRoot_t ptr, const iRect & Rect)
    {
        m_bounds = Rect;
        if (ptr->bounds().contains(m_bounds))
        {
            m_frame_buf = ptr;
        }
    }


    roiWindow(sharedRoot_t ptr, int32_t x, int32_t y, int32_t width, int32_t height)
    {
        m_bounds = iRect(x, y, width, height);
        if (ptr->bounds().contains(m_bounds))
        {
            m_frame_buf = ptr;
        }
    }


    roiWindow(const roiWindow<T> &, int32_t x, int32_t y, int32_t width, int32_t height);


    roiWindow(sharedRoot_t ptr)
    {
        m_frame_buf = ptr;
        m_bounds = iRect(0, 0, ptr->width(), ptr->height());
        
    }

    /*
     * Read from a file. Only integer values supported. Small images only. In efficient. Do not use in production
     *
     */
    roiWindow(const std::string& fqfn, image_memory_alignment_policy im = image_memory_alignment_policy::align_first_row);

    // Destructor
    virtual ~roiWindow() {}

    // Accessors
    const sharedRoot_t & frameBuf() const { return m_frame_buf; }
    sharedRoot_t & frameBuf() { return m_frame_buf; }

    int32_t depth() const { return m_frame_buf->depth(); }
    uint32_t n() const { return m_bounds.area(); }
    int32_t components() const { return m_frame_buf->components(); }

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
    int32_t bytes() const { return T::bytes(); }


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


    std::pair<pixel_t, pixel_t> range() const;
   
    std::pair<pixel_t, iPair> maxValueLocation() const;
   
    // become an entire window to this framebuf
    void entire();
    roiWindow<T> window(int32_t tl_x, int32_t tl_y, int32_t width, int32_t height);
    bool step(const iPair & tp);
  

    // Set pixel value for (x,y). Return new value
    pixel_t setPixel(int32_t px, int32_t py, pixel_t val = pixel_t(0));

    void set(pixel_t val = pixel_t(0));
    void setBorder(int pad, pixel_t clear_val = pixel_t(0));
    bool copy_pixels_from(pixel_ptr_t pixels, int columns, int rows, int stride);
    bool copy_pixels_to(roiWindow<T> & other);
    void randomFill( uint32_t seed );
    void output (std::ostream& outs = std::cout, uint8_t delim = ',', bool is_normalized = true) const;
    void to_file (const std::string& fqfn) const;
  
  
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
    sharedRoot_t m_frame_buf; // Ref-counted pointer to frame buffer
    iRect m_bounds;


};


}

#endif // _WINDOW_H_
