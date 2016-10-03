
#ifndef _MULTIWINDOW_H_
#define _MULTIWINDOW_H_


#include <iostream>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <ctime> // std::time
#include "roiRoot.h"
#include "core/rectangle.h"
#include "roiWindow.h"
#include <mutex>


using namespace std;

namespace svl
{
    
    /* +---------------+---------------+---------------+ */
    /* |               |               |               | */
    /* |               |               |               | */
    /* |               |               |               | */
    /* |               |               |               | */
    /* |               |               |               | */
    /* |      Left     |   Center      |    Right      | */
    /* |               |               |               | */
    /* +---------------+---------------+---------------+ */
    

    
    template <typename T, typename trait_t = typename PixelType<T>::pixel_trait_t , int W = 512, int H = 128>
    class roiMultiWindow    : public roiWindow<trait_t>
    {
        
    public:
        typedef iRect rect_t;
        typedef rect_t::point_t point_t;
        
        typedef typename PixelType<T>::pixel_t pixel_t;
        typedef pixel_t * pixel_ptr_t;
        typedef int64_t timestamp_t;
        typedef roiWindow<trait_t> window_t;
        typedef std::pair<uint32_t, window_t> indexed_window_t;
        
        // Constructors
        roiMultiWindow();
        roiMultiWindow(const std::vector<std::string>& names_l2r, int64_t timestamp = 0,
                       image_memory_alignment_policy im = image_memory_alignment_policy::align_first_row);
        
        
        //! copy / assignment  takes one arguments
        roiMultiWindow(const roiMultiWindow & other);
        const roiMultiWindow & operator=(const roiMultiWindow & rhs);
      
        // Comparison Functions
        /*
         effect     Returns true if the two windows have the same frameBuf
         and have the same size, offset and root-offset.
         note       If both windows are unbound, but have the same offset,
         root-offset and size, returns true.
         */
        bool operator==(const roiMultiWindow & other) const;
        bool operator!=(const roiMultiWindow & other) const;
 
        const roiWindow<trait_t>& plane (const std::string& name)
        {
            int idx = index ( name );
            if (idx < 0)
                return roiWindow<trait_t> ();
            return plane (idx);
        }
        
        const roiWindow<trait_t>& plane (const uint32_t index)
        {
            uint32_t idx = index % T::planes_c;
            return m_planes[idx];
        }
        
        const std::string& name (const uint32_t& index)
        {
            return m_names[index];
        }
        
        int32_t index (const std::string& name)
        {
            for (uint32_t i = 0; i < T::planes_c; i++)
                if (m_names[i].compare(name) == 0) return i;
            return -1;
        }
        
        // Destructor
        virtual ~roiMultiWindow() {}
        

        
    protected:
        
        window_t m_planes [T::planes_c];
        iRect m_bounds [T::planes_c];
        std::string m_names [T::planes_c];
        uint32_t m_indexes [T::planes_c];
        
        
    };
    

        
    
}

#endif // _MULT_WINDOW_H_
