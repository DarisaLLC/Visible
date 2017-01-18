#ifndef _VIEW_
#define _VIEW_

#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <ostream>
#include <vector>
#include <mutex>
#include "core/rectangle.h"
#include <assert.h>
#include "pixel_traits.h"
#include "core/core.hpp"


using namespace std;
using namespace svl;

namespace svl
{

enum image_memory_alignment_policy
{
    align_every_row = 0,
    align_first_row = 1
};


template <typename T>
class root : public svl::refcounted_ptr<root<T> >
{
public:
    typedef typename PixelType<T>::pixel_t pixel_t;
    typedef pixel_t * pixel_ptr_t;

    // Constructors
    root(image_memory_alignment_policy imap = align_every_row, int alignment_bytes = 8)
        : m_align_policy(imap), _bayer_type(NoneBayer), m_align(alignment_bytes),
            m_channels  (T::components())

    {
        
    }

    root(int32_t width, int32_t height, image_memory_alignment_policy imap = align_every_row, int alignment_bytes = 8)
        : m_align_policy(imap), _bayer_type(NoneBayer), m_align(alignment_bytes),   m_channels  (T::components())
    {
        setup_native(width, height);
    }


    /* Row update for SRC pixels, not DEST frame */
    root(uint8_t * rawPixels, int32_t RowUpdateBytes, int32_t awidth, int32_t aheight, image_memory_alignment_policy imap = align_every_row, int alignment_bytes = 8)
        : m_align_policy(imap), _bayer_type(NoneBayer), m_align(alignment_bytes),   m_channels  (T::components())
    {
        assert(aheight > 0);
        assert(awidth > 0);
        assert(rawPixels);
        this->setup_native(awidth, aheight);

        // Now copy pixels into image
        for (int32_t y = 0; y < aheight; y++)
        {
            pixel_ptr_t pels = rowPointer(y);
            //      assert(boost::is_pointer<char*>(rawPixels));
            std::copy(pels, pels + awidth, rawPixels);
            rawPixels += RowUpdateBytes;
        }
    }

    // prohibit irect copying and assignment
    root(root && other) = delete;
    root & operator=(root && other) = delete;


    virtual ~root()
    {
        if (m_storage) delete m_storage;
    }
    bayer_type bayerType() { return _bayer_type; }
    void SetbayerType(bayer_type b) { _bayer_type = b; }


    const pixel_ptr_t rowPointer(int32_t y) const
    {
        return (pixel_ptr_t)(alignedPixelData() + y * rowUpdate());
    }

    pixel_ptr_t rowPointer(int32_t y)
    {
        return (pixel_ptr_t)(alignedPixelData() + y * rowUpdate());
    }

    // Get pixel address
    const pixel_ptr_t pelPointer(int32_t x, int32_t y) const
    {
        return rowPointer(y) + x;
    }

    pixel_ptr_t pelPointer(int32_t x, int32_t y)
    {
        return rowPointer(y) + x;
    }

    pixel_t getPixel(int32_t x, int32_t y) const
    {
        return *pelPointer(x, y);
    }

    pixel_t setPixel(int32_t x, int32_t y, pixel_t val)
    {
        *pelPointer(x, y) = val;
        return val;
    }

    // be careful!
    uint8_t * raw_storage() const
    {
        return (uint8_t *)(m_storage);
    }

    const uint8_t * alignedPixelData() const
    {
        return (const uint8_t *)(m_image_data);
    }

    // Row update constant: constant address to add to next pixel next row
    int32_t rowUpdate() const { return m_rowbytes; }

    // Row update constant: constant address to add to next pixel next row
    int32_t rowPixelUpdate() const { return rowUpdate() / (T::bytes()* components()) ; }

    inline iRect bounds() const { return m_bounds; }
    inline int32_t width() const { return m_width; }
    inline int32_t height() const { return m_height; }
    inline int32_t depth() const { return m_depth; }
    inline int32_t components() const { return m_channels; }
    inline uint32_t n() const { return width() * height(); }
    inline int32_t alignment() const { return m_align; }
    inline int32_t pad () const { return m_pad; }
    inline image_memory_alignment_policy alignment_policy() { return m_align_policy; }
    inline int64_t timestamp() const { return m_timestamp; };


    // Mutators
    void set(pixel_t val);
    //        void setBorder(int pad, pixel_t clear_val = pixel_t(0));

    inline void setTimestamp(int64_t time) { m_timestamp = time; }
    void reset() { m_index = -1; }
    int index() const { return m_index; }
    void set_index(int idx) { m_index = idx; }
    int duration() const { return m_dt; }
    void set_duration(int ddt) { m_dt = ddt; }


    // Load the image pointed to by rawPixels into this frame
    void loadImage(uint8_t * rawBytes, int32_t RowUpdateBytes, int32_t iwidth,
                   int32_t iheight)
    {
        assert(rawBytes);
        assert(width() == iwidth);
        assert(height() == iheight);

        int32_t bytesInRow = width() * T::bytes();

        if (RowUpdateBytes == bytesInRow)
        {
            int32_t bytesInImage = bytesInRow * height();
            std::memcpy (m_image_data, rawBytes, bytesInImage);
        }
        else
        {
            for (int32_t y = 0; y < height(); ++y)
            {
                pixel_ptr_t pels = rowPointer(y);
                std::memcpy(pels, rawBytes, width());
                rawBytes += RowUpdateBytes;
            }
        }
    }


    int rowPad() const { return m_pad; }

    void addRef() { intrusive_ptr_add_ref(this); }
    void remRef() { intrusive_ptr_release(this); }

protected:
    int32_t m_pad;
    int32_t m_align;
    int64_t m_timestamp; // Creation time stamp
    uint8_t * m_storage;
    iRect m_bounds;
    int32_t m_depth;
    int32_t m_channels;
    int32_t m_rowbytes;
    int32_t m_width, m_height;
    uint8_t * m_image_data;
    int m_index;
    int m_dt;
    bayer_type _bayer_type;

    image_memory_alignment_policy m_align_policy;

    void setup_native(int32_t width, int32_t height)
    {
        assert(height > 0);
        assert(width > 0);
        assert(m_align == 16 || m_align == 32 || m_align == 64 || m_align == 8 || m_align == 128 || m_align == 256);

        m_width = width;
        m_height = height;
        m_bounds = iRect(0, 0, width, height);

        // Make sure starting addresses are at the correct alignment boundary
        // Start of each row pointer is mAlignMod aligned

        int32_t rowBytes = width * T::bytes();
        m_pad = rowBytes % m_align;
        if (m_pad) m_pad = m_align - m_pad;

        // Total storage need:
        // If aligned_per row, add padding for every row

        m_rowbytes = rowBytes + (m_align_policy == align_every_row ? m_pad : 0);
        int size = m_rowbytes * height + m_align - 1;

        m_storage = new uint8_t[size];
        auto startPtr = m_storage;
        while ((intptr_t)startPtr % m_align)
            startPtr++;
        ptrdiff_t n = startPtr - m_storage;
        assert((n < m_align) && (n >= 0));
        m_image_data = startPtr;
    }
};


template <typename P>
class sharedRoot
{
public:
    friend class root<P>;
    // Constructors
    sharedRoot()
        : mFrameBuf(0) {}

    sharedRoot(root<P> * p)
        : mFrameBuf(p)
    {
        if (mFrameBuf)
            mFrameBuf->addRef();
    }
    sharedRoot(const sharedRoot & p)
        : mFrameBuf(p.mFrameBuf)
    {
        if (mFrameBuf)
            mFrameBuf->addRef();
    }

    // Destructor
    virtual ~sharedRoot();


    // Assignment operators
    virtual sharedRoot & operator=(const sharedRoot & p);
    virtual sharedRoot & operator=(root<P> * p);

    // Comparison operators
    bool operator==(const sharedRoot & p) const
    {
        return (mFrameBuf == p.mFrameBuf);
    }
    bool operator!=(const sharedRoot & p) const
    {
        return !(this->operator==(p));
    }
    bool operator==(const root<P> * p) const
    {
        return (mFrameBuf == p);
    }
    bool operator!=(const root<P> * p) const
    {
        return !(this->operator==(p));
    }

    // Accessors
    operator root<P> *() const
    {
        std::lock_guard<std::mutex> lock( mMutex );
        return mFrameBuf;
    }
    const root<P> & operator*() const
    {
        std::lock_guard<std::mutex> lock( mMutex );
        return *mFrameBuf;
    }
    root<P> & operator*()
    {
        std::lock_guard<std::mutex> lock( mMutex );
        return *mFrameBuf;
    }
    const root<P> * operator->() const
    {
        std::lock_guard<std::mutex> lock( mMutex );
        return mFrameBuf;
    }
    root<P> * operator->()
    {
        std::lock_guard<std::mutex> lock( mMutex );
        return mFrameBuf;
    }

    int64_t refcount() const
    {
        std::lock_guard<std::mutex> lock( mMutex );
        int64_t retVal = 0;
        if (mFrameBuf)
        {
            retVal = mFrameBuf->refcount();
        }
        return retVal;
    }


private:
    mutable root<P> * mFrameBuf;
    mutable std::mutex mMutex;
};
    
 
    
}
#endif
