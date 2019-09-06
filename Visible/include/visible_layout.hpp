

#ifndef __VISIBLE_LAYOUT__
#define __VISIBLE_LAYOUT__

#include "cinder/Rect.h"
#include "cinder/Signals.h"
#include "cinder/app/Event.h"
#include "core/signaler.h"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace ci;
using namespace ci::signals;

#define STAGE_WIDTH 1300
#define STAGE_HEIGHT 800

#define TOOLBAR_WIDTH 200
#define TOOLBAR_HEIGHT 600
#define TOOLBAR_PADDING 40


/*
 *   Visible Layout
 
 ************************    **********
 *   Image      C0      *    * Plots  *
 * ******************   *    **********
 *              C1      *    *        *
 * ******************   *    **********
 *              C2      *    *        *
 ************************    **********
 
 ************************    **********
 *    Time Line         *    **********
 ************************    **********
 
 ************************
 *    Event Time Line   *
 ************************
 
 ************************
 *    Event Time Line   *
 ************************
 
 **************************************
 *  Log Output                        *
 **************************************
 
 
 */

class layouter : public base_signaler
{
    virtual std::string
    getName () const { return "LayoutManager"; }
};

class layoutManager : tiny_media_info, layouter
{
public:
    
    typedef void (sig_plot_size_changed_cb) ();
    typedef void (sig_window_size_changed_cb) ();
    
    layoutManager (ivec2 trim, bool keep_aspect = true):m_keep_aspect ( keep_aspect), m_trim (trim), m_isSet (false)
    {
        // Load defaults
        // from config file ?
        m_image_frame_size_norm = vec2(0.33, 0.33);
        m_single_plot_size_norm = vec2(0.25, 0.15);
        m_timeline_size_norm = vec2(0.33, 0.08);
        m_log_size_norm = vec2(0.95, 0.15);
        m_scale = 1.0f;
        
        // Two signals we provide
        m_signal_window_size_changed = createSignal<sig_window_size_changed_cb>();
        m_signal_plot_size_changed = createSignal<sig_plot_size_changed_cb>();

        // And subscribe to as well
        std::function<void ()> window_size_changed_cb = std::bind (&layoutManager::update, this);
        boost::signals2::connection ml_window_connection = registerCallback(window_size_changed_cb);
        
        std::function<void ()> plot_size_changed_cb = std::bind (&layoutManager::update, this);
        boost::signals2::connection ml_plot_connection = registerCallback(plot_size_changed_cb);
        
    }
    
    // Constructor
    void init (const vec2& uiWinSize, const tiny_media_info& tmi, const int num_channels_plots = 3, const float scale = 1.0)
    
    {
        m_cc = num_channels_plots;
        m_scale = scale;
        // Load defaults
        // from config file ?
        m_image_frame_size_norm = vec2( tmi.getWidth()/uiWinSize.x, tmi.getHeight()/uiWinSize.y);
        m_single_plot_size_norm = vec2(0.25, 0.15);
        m_timeline_size_norm = vec2( m_image_frame_size_norm.x, 0.08);
        m_log_size_norm = vec2(0.95, 0.08);
        
        *((tiny_media_info*)this) = tmi;
        m_canvas_size = uiWinSize;
        Area wi (0, 0, m_canvas_size.x, m_canvas_size.y);
        m_window_rect = Rectf(wi);
        Area ai (20, 30, 20+tmi.getWidth() * m_scale, 30+tmi.getHeight() * m_scale);
        m_image_frame_size_norm.x *= m_scale;
        m_image_frame_size_norm.y *= m_scale;
        m_timeline_size_norm.x *= m_scale;
        m_timeline_size_norm.y *= m_scale;
        m_single_plot_size_norm.x = m_image_frame_size_norm.x / 2;
        m_single_plot_size_norm.y = m_image_frame_size_norm.y / 3;
        
        m_image_rect = Rectf (ai);
        m_aspect = layoutManager::aspect(tmi.getSize());
        m_isSet = true;
        
        update_window_size(m_canvas_size);
        m_slider_rects.clear();
    }
    
    // Accessors
    inline bool isSet () const { return m_isSet; }
    inline int& channelCount () const { return m_cc; }
    
    inline const Rectf& display_frame_rect ()
    {
        return m_current_display_frame_rect;
    }
    
    
    inline const ivec2 display2image (const ivec2& pixel_loc)
    {
        return m_display2image.map(pixel_loc);
    }
    
    inline const Rectf& display_timeline_rect ()
    {
        return m_current_display_timeline_rect;
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
    
    
    inline const std::vector<Rectf>& plot_rects () const
    {
        return m_plot_rects;
    }
    
    inline const Rectf& display_plots_rect () const
    {
        return m_plots_display;
    }
    
    inline const std::vector<Rectf>& slider_rects () const
    {
        return m_slider_rects;
    }

    inline const vec2& single_plot_size_norm (){ return m_single_plot_size_norm;}
    
    // Mutators
    inline void update ()
    {
        update_display_frame_rect ();
        update_display_timeline_rect ();
        update_display_plots_rects();
    }
    
    inline void update_window_size (const ivec2& new_size )
    {
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
        
        update ();
        
        // Call the layout change cb if any
        if (m_signal_window_size_changed && m_signal_window_size_changed->num_slots() > 0)
            m_signal_window_size_changed->operator()();
    }
    
    // Modify window size
    inline void scale (vec2& scale_by)
    {
        m_canvas_size.x  = std::floor(m_canvas_size.x * scale_by.x);
        m_canvas_size.y  = std::floor(m_canvas_size.y * scale_by.y);
        update_window_size (m_canvas_size);
    }
    
    // Modify Normalized Single Plot Size
    void single_plot_size_norm (const vec2& new_norm)
    {
        m_single_plot_size_norm = new_norm;
        // Call the layout change cb if any
        if (m_signal_plot_size_changed && m_signal_plot_size_changed->num_slots() > 0)
            m_signal_plot_size_changed->operator()();
    }
    
    uint32_t add_slider_rect ()
    {
        Rectf slider = display_timeline_rect ();
        slider.offset(vec2(0, (1 + m_slider_rects.size()) * slider.getHeight()));
        m_slider_rects.push_back(slider);
        // @todo FIX ME Please 
        return uint32_t(m_slider_rects.size() - 1);
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
     //   vec2 np = vec2 (image_frame_position_norm().x * desired_window_size().x, image_frame_position_norm().y * desired_window_size().y);
        vec2 np (20, 30);
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
    
    inline vec2& timeline_size_norm (){ return m_timeline_size_norm;}
    
    inline ivec2 timeline_frame_size ()
    {
        vec2& np = timeline_size_norm ();
        return ivec2 (np.x * desired_window_size().x, np.y * desired_window_size().y);
    }
    
    // Plot areas on the right
    inline vec2 plots_frame_position_norm ()
    {
        vec2 np = vec2 (image_frame_position_norm().x + image_frame_size_norm().x, 1.5 * trim_norm().y);
        return np;
    }
    
    inline vec2 plots_frame_position ()
    {
        auto fp = image_frame_rect ();
        vec2 np = fp.getUpperRight();
        return np;
    }
    

    inline vec2 plots_frame_size_norm (){ vec2 np = vec2 (single_plot_size_norm().x, m_cc * single_plot_size_norm().y); return np;}
    
    
    inline ivec2 plots_frame_size () { return ivec2 ((canvas_size().x * plots_frame_size_norm().x),
                                                     (canvas_size().y * plots_frame_size_norm().y)); }
    
    inline ivec2 single_plot_size () { return ivec2 ((canvas_size().x * single_plot_size_norm().x),
                                                     (canvas_size().y * single_plot_size_norm().y)); }
    
    
    inline vec2 canvas_norm_size () { return vec2(canvas_size().x, canvas_size().y) / vec2(getWindowWidth(), getWindowHeight()); }
    inline Rectf text_norm_rect () { return Rectf(0.0, 1.0 - 0.125, 1.0, 0.125); }
    
    
    
    
    inline void unnorm_rect (Rectf& drect, ivec2& factor)
    {
        ivec2 tl (factor.x * drect.getUpperLeft().x, factor.y * drect.getUpperLeft().y);
        ivec2 lr (factor.x * drect.getLowerRight().x, factor.y * drect.getLowerRight().y);
        drect = Rectf(tl, lr);
    }
    
    inline void update_display_frame_rect ()
    {
        if (! m_isSet ) return;
        
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
        m_display2image = RectMapping (rawf, m_image_rect, true);
        m_current_display_frame_rect = rawf;
    }
    
    // Updates sliders
    inline void update_display_timeline_rect ()
    {
        assert (m_image_frame_size_norm.x == m_timeline_size_norm.x);
        auto timeline_size = timeline_frame_size ();
        // Get the image frame rect
        // Set width to the actual image display width
        // Space it in y at half of its expected size
        Rectf ir = display_frame_rect ();
        vec2 TL (ir.getLowerLeft().x,ir.getLowerLeft().y + timeline_size.y / 2);
        vec2 BR (TL.x + ir.getWidth() , TL.y + timeline_size.y);
        Rectf new_timeline_rect (TL, BR);
        
        // Update the other sliders.
        std::vector<Rectf> new_sliders;
        for (auto ss = 0; ss < m_slider_rects.size(); ss++)
        {
            Rectf slider = new_timeline_rect;
            slider.offset(vec2(0, (ss + m_slider_rects.size()) * slider.getHeight()));
            new_sliders.push_back(slider);
        }
        m_slider_rects = new_sliders;
        m_current_display_timeline_rect = new_timeline_rect;
        
    }
    
    // Updates Plot Rects
    //@todo adjust
    inline  void update_display_plots_rects ()
    {
        m_plot_rects.resize(m_cc);
        auto plot_tl = plots_frame_position_norm();
        auto plot_size = single_plot_size_norm ();
        vec2 plot_vertical (0.0, plot_size.y);
        ivec2 cs = canvas_size ();
        
        for (int i = 0; i < m_cc; i++)
        {
            m_plot_rects[i] = Rectf (plot_tl, vec2 (plot_tl.x + plot_size.x, plot_tl.y + plot_size.y));
            unnorm_rect(m_plot_rects[i], cs);
            plot_tl += plot_vertical;
        }
        
//        m_plots_display = Rectf(m_plot_rects[0].getUpperLeft(), m_plot_rects[m_cc-1].getLowerRight());
//        auto plt_ul = m_plots_display.getUpperLeft();
//        auto dis_ur = m_current_display_frame_rect.getUpperRight();
//        auto dis = dis_ur - plt_ul;
//        if (dis.x < 0) m_plots_display.offset(vec2(-dis.x, 0));
        
        
    }
    
    
    // Represent norm rectangles for the whole screen and image viewer
    // Real Rects are updated according to the layout and screen size
    mutable Rectf m_window_rect, m_image_rect;
    mutable Rectf m_current_display_frame_rect;
    mutable Rectf m_current_display_timeline_rect;
    
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
    mutable std::vector<Rectf> m_slider_rects;
    mutable float m_scale;
    static float aspect (const ivec2& s) { return s.x / (float) s.y; }
    mutable int m_cc;
    mutable RectMapping m_display2image;
protected:
    boost::signals2::signal<sig_plot_size_changed_cb>* m_signal_plot_size_changed;
    boost::signals2::signal<sig_window_size_changed_cb>* m_signal_window_size_changed;
    
};

#endif
