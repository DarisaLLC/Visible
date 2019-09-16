

#ifndef __VISIBLE_LAYOUT__
#define __VISIBLE_LAYOUT__

#include "cinder/Rect.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include "imgui.h"
#include "CinderImGui.h"

using namespace std;
using namespace ci;
using namespace ImGui;

// Implicit channel organization
///////////////
//    0
///////////////
//    1
/////////////// if two channels
//    2
/////////////// if three channels


class imageDisplayMapper : tiny_media_info
{
public:
    // Constructor
    imageDisplayMapper () {}
    void init (const tiny_media_info& tmi){
         *((tiny_media_info*)this) = tmi;
        m_cc = this->getNumChannels();
        m_image_rect = Rectf(0,0,this->getWidth(), this->getHeight());
        m_display_rect = m_image_rect;
        m_channel_size.first = this->getChannelWidth();
        m_channel_size.second = this->getChannelHeight();
        update_maps();
    }
    
    // Accessors
    inline int& channelCount () const { return m_cc; }
    
    inline const Rectf& display_frame_rect ()
    {
        return m_display_rect;
    }
    
    inline const Rectf& image_frame_rect ()
    {
        return m_image_rect;
    }
    
    
    inline const ivec2 display2image (const ivec2& display_pixel_loc)
    {
        return m_display2image.map(display_pixel_loc);
    }
    
    inline const int display2channel (const ivec2& display_pixel_loc)
    {
        auto iloc = m_display2image.map(display_pixel_loc);
        return ((int)iloc.y) / m_channel_size.second;
    }
    
    inline const ImVec2 image2display (const ivec2& image_pixel_loc, int channel = -1, bool bottom_left_origin = false){
        assert(channel < m_cc);
        ivec2 all_loc = image_pixel_loc;
        if(channel >= 0){
            auto x = image_pixel_loc.x % m_channel_size.first;
            auto y =image_pixel_loc.y % m_channel_size.second;
            int yoffset = channel*m_channel_size.second;
            all_loc.x = x;
            all_loc.y = y+yoffset;
        }
        if(m_image_rect.contains(all_loc))
            return m_image2display.map(all_loc);
        return image_pixel_loc;
    }
  
    // Mutators
    
    inline void update_display_rect (const ImVec2& tl, const ImVec2& br){
        Rectf ns (tl.x, tl.y, br.x, br.y);
        m_display_rect = ns;
        update_maps ();
    }
    

  
    
private:
  
    inline void update_maps ()
    {
        m_image2display = RectMapping (m_image_rect, m_display_rect, false);
        m_display2image = RectMapping (m_display_rect, m_image_rect, false);
    }
    
  
  
    
    // Represent norm rectangles for the whole screen and image viewer
    // Real Rects are updated according to the layout and screen size
    mutable Rectf m_image_rect;
    mutable Rectf m_display_rect;
    mutable int m_cc;
    std::pair<int,int> m_channel_size;
    mutable RectMapping m_display2image;
    mutable RectMapping m_image2display;
    
};

#endif
