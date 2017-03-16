

#ifndef __VISIBLE_LAYOUT__
#define __VISIBLE_LAYOUT__

#include "cinder/Rect.h"
#include "cinder/Signals.h"
#include "cinder/app/Event.h"

#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace ci;
using namespace ci::signals;

/*
 *   Visible Layout
 
 ************************    **********
 *                      *    * Plots  *
 *  Image               *    **********
 *                      *    *        *
 *                      *    **********
 *                      *    *        *
 ************************    **********
 
 ************************    **********
 *    Time Line         *    **********
 ************************    **********
 
 **************************************
 *  Log Output                        *
     **************************************
 
 
 */

class layout : tiny_media_info
{
public:
    
    typedef	 signals::Signal<void( ivec2 & )>		LayoutSignalWindowSize_t;
    LayoutSignalWindowSize_t&	getSignalWindowSize() { return m_windowSizeSignal; }
    
    layout (ivec2 trim, bool keep_aspect = true):m_keep_aspect ( keep_aspect), m_trim (trim), m_isSet (false)
    {
        
        m_image_frame_size_norm = vec2(0.75, 0.75);
        m_single_plot_size_norm = vec2(0.20, 0.25);
        m_timeline_size_norm = vec2(0.75, 0.10);
        m_log_size_norm = vec2(0.95, 0.10);
        
        
    }
    
    // Constructor
    void init (const WindowRef& uiWin, const tiny_media_info& tmi)
    
    {
        *((tiny_media_info*)this) = tmi;
        m_canvas_size = uiWin->getSize ();
        Area wi (0, 0, uiWin->getSize().x, uiWin->getSize().y);
        m_window_rect = Rectf(wi);
        Area ai (0, 0, tmi.getWidth(), tmi.getHeight());
        m_image_rect = Rectf (ai);
        m_aspect = layout::aspect(tmi.getSize());
        m_isSet = true;
    }
    
    inline bool isSet () const { return m_isSet; }
    
    inline void update_window_size (const ivec2& new_size )
    {
        if (m_canvas_size == new_size) return;
        
        ivec2 ns = new_size;
        if (m_keep_aspect)
        {
            float newAr = new_size.x / (float) new_size.y ;
            if (aspectRatio() > newAr)
                ns.y= std::floor(ns.x/ aspectRatio ());
            else
                ns.x = std::floor(ns.y * aspectRatio () );
        }
        
        m_canvas_size = new_size;
        Area wi (0, 0, m_canvas_size.x, m_canvas_size.y);
        m_window_rect = Rectf(wi);
        
        m_windowSizeSignal.emit(m_canvas_size);
    }
    
    inline Rectf display_frame_rect ()
    {
        assert(isSet());
        
        // Get the image frame
        Rectf imf = image_frame_rect ();
        Rectf rawf = m_image_rect;
        if (imf.intersects(rawf))
        {
            auto it = rawf.getCenteredFit(imf, true);
            // Leave it centered and not align it to the top
            //       it.offset(-it.getUpperLeft());
            rawf = it;
        }
        return rawf;
    }
    
    inline Rectf display_timeline_rect ()
    {
        assert (m_image_frame_size_norm.x == m_timeline_size_norm.x);
        vec2 scali (1.0, m_timeline_size_norm.y / m_image_frame_size_norm.y);
        // Get the image frame rect
        Rectf ir = display_frame_rect ();
        // Move it down the height of the image rect + trim
        vec2 offset (ir.getCenter().x, ir.getY2() + ir.getY1());
        ir.offsetCenterTo (offset);
        // Cut the height to timeline height
        ir.scaleCentered (scali);
        return ir;
    }
    
    // Modify window size
    inline void scale (vec2& scale_by)
    {
        m_canvas_size.x  = std::floor(m_canvas_size.x * scale_by.x);
        m_canvas_size.y  = std::floor(m_canvas_size.y * scale_by.y);
        m_windowSizeSignal.emit(m_canvas_size);
    }
    
    
    const vec2& normImageFrameSize () const { return m_image_frame_size_norm; }
    void normImageFrameSize (vec2 ns) const { m_image_frame_size_norm = ns; }
    
    const vec2& normSinglePlotSize () const { return m_single_plot_size_norm; }
    void normSinglePlotSize (vec2 ns) const { m_single_plot_size_norm = ns; }
    
    
    const float& aspectRatio () const { return m_aspect; }
    
    // Trim on all sides
    inline ivec2& trim () const { return m_trim; }
    vec2 trim_norm () { return vec2(trim().x, trim().y) / vec2(m_canvas_size.x, m_canvas_size.y); }
    
    
    inline Rectf desired_window_rect () { return Rectf (trim().x, trim().y, trim().x + canvas_size().x, trim().y + canvas_size().y); }
    inline ivec2& desired_window_size () const { return m_canvas_size; }
    inline ivec2 canvas_size () { return desired_window_size() - trim() - trim (); }
    
    
    inline void plot_rects (std::vector<Rectf>& plots )
    {
        update_plots ();
        
        vec2 cs (m_canvas_size.x, m_canvas_size.y);
        
        for (Rectf& plot : m_plot_rects)
        {
            plot.x1 *= cs.x;
            plot.x2 *= cs.x;
            plot.y1 *= cs.y;
            plot.y2 *= cs.y;
            
        }
        plots = m_plot_rects;
        
    }
    
    inline Rectf display_plots_rect ()
    {
        update_plots ();
        
        vec2 cs (m_canvas_size.x, m_canvas_size.y);
        Rectf ir = m_plots_display;
        ir.x1 *= cs.x;
        ir.x2 *= cs.x;
        ir.y1 *= cs.y;
        ir.y2 *= cs.y;
        
        return ir;
    }
    
    
private:
    bool m_isSet;
    
    // image_fram is rect placeholder for image. It will contain the image
    // It is assumed that Image will be on the top left portion
    inline vec2 image_frame_position_norm ()
    {
        vec2 np = vec2 (trim_norm().x, trim_norm().y);
        return np;
    }
    
    
    inline Rectf image_frame_rect ()
    {
        vec2 np = vec2 (image_frame_position_norm().x * desired_window_size().x, image_frame_position_norm().y * desired_window_size().y);
        vec2 pp = image_frame_size_norm ();
        vec2 pn (np.x + pp.x * desired_window_size().x, np.y + pp.y * desired_window_size().y);
        return Rectf(np, pn);
    }
    
    inline vec2& image_frame_size_norm (){ return m_image_frame_size_norm;}
    inline ivec2 image_frame_size ()
    {
        vec2& np = image_frame_size_norm ();
        return ivec2 (np.x * desired_window_size().x, np.y * desired_window_size().y);
    }
    
    // Plot areas on the right
    inline vec2 plots_frame_position_norm ()
    {
        vec2 np = vec2 (1.0 - trim_norm().x - plots_frame_size_norm().x , trim_norm().y);
        return np;
    }
    
    inline vec2 plots_frame_position ()
    {
        vec2 np = vec2 (plots_frame_position_norm().x * desired_window_size().x, plots_frame_position_norm().y * desired_window_size().y);
        return np;
    }
    
    inline vec2 single_plot_size_norm (){ return m_single_plot_size_norm;}
    inline vec2 plots_frame_size_norm (){ vec2 np = vec2 (single_plot_size_norm().x, 3 * single_plot_size_norm().y); return np;}
    
    
    inline ivec2 plots_frame_size () { return ivec2 ((canvas_size().x * plots_frame_size_norm().x),
                                                     (canvas_size().y * plots_frame_size_norm().y)); }
    
    inline ivec2 single_plot_size () { return ivec2 ((canvas_size().x * single_plot_size_norm().x),
                                                     (canvas_size().y * single_plot_size_norm().y)); }
    
    
    inline vec2 canvas_norm_size () { return vec2(canvas_size().x, canvas_size().y) / vec2(getWindowWidth(), getWindowHeight()); }
    inline Rectf text_norm_rect () { return Rectf(0.0, 1.0 - 0.125, 1.0, 0.125); }
    
    
    
    inline void update_plots ()
    {
        m_plot_rects.resize(3);
        auto plot_tl = plots_frame_position_norm();
        auto plot_size = single_plot_size_norm ();
        vec2 plot_vertical (0.0, plot_size.y);
        
        m_plot_rects[0] = Rectf (plot_tl, vec2 (plot_tl.x + plot_size.x, plot_tl.y + plot_size.y));
        plot_tl += plot_vertical;
        m_plot_rects[1] = Rectf (plot_tl, vec2 (plot_tl.x + plot_size.x, plot_tl.y + plot_size.y));
        plot_tl += plot_vertical;
        m_plot_rects[2] = Rectf (plot_tl, vec2 (plot_tl.x + plot_size.x, plot_tl.y + plot_size.y));
        
        m_plots_display = Rectf(m_plot_rects[0].getUpperLeft(), m_plot_rects[2].getLowerRight());
        
    }
    
    
    // Represent norm rectangles for the whole screen and image viewer
    // Real Rects are updated according to the layout and screen size
    mutable Rectf m_window_rect, m_image_rect;
    mutable ivec2 m_canvas_size;
    mutable ivec2 m_trim;
    mutable float m_aspect;
    mutable bool m_keep_aspect;
    mutable vec2 m_image_frame_size_norm;
    mutable vec2 m_single_plot_size_norm;
    mutable vec2 m_timeline_size_norm;
    mutable vec2 m_log_size_norm;
    mutable Rectf m_plots_display;
    mutable std::vector<Rectf> m_plot_rects;
    
    LayoutSignalWindowSize_t m_windowSizeSignal;
    
    static float aspect (const ivec2& s) { return s.x / (float) s.y; }
    
};

#endif
