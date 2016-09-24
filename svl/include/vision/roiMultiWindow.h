
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
    
    
    template <typename T, int W = 512, int H = 128>
    class roiMultiWindow
    {
        
    public:
        typedef iRect rect_t;
        typedef rect_t::point_t point_t;
        typedef typename PixelType<T>::pixel_trait_t trait_t;
        typedef typename PixelType<T>::pixel_t pixel_t;
        typedef pixel_t * pixel_ptr_t;
        typedef int64_t timestamp_t;
        typedef roiWindow<trait_t> window_t;
        typedef std::pair<uint32_t, window_t> indexed_window_t;
        
        // Constructors
        roiMultiWindow();
        roiMultiWindow(const std::vector<std::string>& names_l2r,
                       image_memory_alignment_policy im = image_memory_alignment_policy::align_first_row)
        : m_all (W * T::planes_c,H)
        {
            static std::string defaults [3] { "left", "center", "right" };
            if (names_l2r.size() == T::planes_c)
            {
                m_name2index[names_l2r[0]] = 0;
                m_name2index[names_l2r[1]] = 1;
                m_name2index[names_l2r[2]] = 2;
            }
            else
            {
                m_name2index[defaults[0]] = 0;
                m_name2index[defaults[1]] = 1;
                m_name2index[defaults[2]] = 2;
            }

            iPair spacing (W, 0);
            iPair msize (W, H);
            m_bounds[0].size (msize);
            m_bounds[1] = m_bounds[0];
            m_bounds[1].translate (spacing);
            m_bounds[2] = m_bounds[1];
            m_bounds[2].translate (spacing);
            
            auto allr = m_bounds[0] | m_bounds[1] | m_bounds[2];
            assert (allr == m_all.bound ());
            
            for (unsigned ww = 0; ww < T::planes_c; ww++)
            {
                m_planes[ww] = roiWindow<trait_t>(m_all.frameBuf(),m_bounds[ww]);
            }
            
        }
        
        
        //! copy / assignment  takes one arguments
        roiMultiWindow(const roiMultiWindow & other);
        const roiMultiWindow & operator=(const roiMultiWindow & rhs);
        

        // Destructor
        virtual ~roiMultiWindow() {}
        

        
    protected:
        
        window_t m_all;
        window_t m_planes [T::planes_c];
        std::map<std::string, uint32_t> m_name2index;
        iRect m_bounds [T::planes_c];
        
        
    };
    

        
    
}

#endif // _MULT_WINDOW_H_
