

#ifndef __VISIBLE_LAYOUT__
#define __VISIBLE_LAYOUT__

#include "cinder/Rect.h"

using namespace std;
using namespace ci;

class layout
{
public:
    layout (ivec2 expected, ivec2 trim): m_expected (expected), m_trim (trim) {}
    
    inline ivec2& desired_window_size () const { return m_expected; }
    inline ivec2& trim () const { return m_trim; }


    vec2 trim_norm () { return vec2(trim().x, trim().y) / vec2(getWindowWidth(), getWindowHeight()); }
    inline ivec2 canvas_size () { return getWindowSize() - trim() - trim (); }
    inline vec2& image_frame_size_norm (){ static vec2 ni (0.67, 0.75); return ni;}
    inline ivec2 image_frame_size ()
    {
        static vec2& np = image_frame_size_norm ();
        return ivec2 (np.x * desired_window_size().x, np.y * desired_window_size().y);
    }
    
    
    inline vec2 plots_frame_position_norm ()
    {
        vec2 np = vec2 (image_frame_size_norm().x + trim_norm().x, trim_norm().y);
        return np;
    }
    inline vec2 image_frame_position_norm ()
    {
        vec2 np = vec2 (trim_norm().x, trim_norm().y);
        return np;
    }
    
    inline vec2 plots_frame_position ()
    {
        vec2 np = vec2 (plots_frame_position_norm().x * desired_window_size().x, plots_frame_position_norm().y * desired_window_size().y);
        return np;
    }
    inline vec2 image_frame_position ()
    {
        vec2 np = vec2 (image_frame_position_norm().x * desired_window_size().x, image_frame_position_norm().y * desired_window_size().y);
        return np;
    }
    
    
    inline vec2 single_plot_size_norm (){ vec2 np = vec2 (1.0 - image_frame_size_norm().x, 0.25); return np;}
    inline vec2 plots_frame_size_norm (){ vec2 np = vec2 (1.0 - image_frame_size_norm().x,
                                                          3 * single_plot_size_norm().y); return np;}
    
    
    inline ivec2 plots_frame_size () { return ivec2 ((canvas_size().x * plots_frame_size_norm().x),
                                                     (canvas_size().y * plots_frame_size_norm().y)); }
    
    inline ivec2 single_plot_size () { return ivec2 ((canvas_size().x * single_plot_size_norm().x),
                                                     (canvas_size().y * single_plot_size_norm().y)); }
    
    inline Rectf image_frame_rect ()
    {
        vec2 tl = image_frame_position();
        vec2 br = vec2 (tl.x + image_frame_size().x, tl.y + image_frame_size().y);
        
        return Rectf (tl, br);
    }
    
    inline vec2 canvas_norm_size () { return vec2(canvas_size().x, canvas_size().y) / vec2(getWindowWidth(), getWindowHeight()); }
    inline Rectf text_norm_rect () { return Rectf(0.0, 1.0 - 0.125, 1.0, 0.125); }
    inline void plot_rects (std::vector<Rectf>& plots )
    {
        plots.resize(3);
        auto plot_tl = plots_frame_position();
        auto plot_size = single_plot_size();
        
        plots[0] = Rectf (plot_tl, vec2 (plot_tl.x + plot_size.x, plot_tl.y + plot_size.y));
        plots[1] = Rectf (vec2(plots[0].getUpperLeft().x, plots[0].getUpperLeft().y + plot_size.y),
                          vec2(plots[0].getLowerRight().x, plots[0].getLowerRight().y + plot_size.y));
        plots[2] = Rectf (vec2(plots[1].getUpperLeft().x, plots[1].getUpperLeft().y + plot_size.y),
                          vec2(plots[1].getLowerRight().x, plots[1].getLowerRight().y + plot_size.y));
    }
    
    
    
    
    
private:
    mutable ivec2 m_expected;
    mutable ivec2 m_trim;
    
    
};

#endif
