#ifndef __GUI__WIDGETS__
#define __GUI__WIDGETS__

#include "cinder/gl/gl.h"
#include "cinder/gl/TextureFont.h"
#include <boost/format.hpp>

#include <vector>
#include <cctype>
#include "InteractiveObject.h"
#include "async_producer.h"
#include "core/core.hpp"
#include "timeMarker.h"

using namespace ci;

namespace tinyUi
{
    typedef float marker_t;
    typedef std::pair<marker_t,marker_t> duration_t;
    typedef std::pair<ColorA,ColorA> marker_colors_t;
    typedef std::pair<duration_t,marker_colors_t> duration_marker_t;
    typedef std::pair<marker_t,ColorA> timepoint_marker_t;
    
    
    static gl::TextureFontRef getWidgetTexFont() {
        static gl::TextureFontRef sWidgetTexFont;
        if( ! sWidgetTexFont )
            sWidgetTexFont = gl::TextureFont::create( Font( Font::getDefault().getName(), 22 ) );
        return sWidgetTexFont;
    }
    
    static std::vector<struct TextInput *> sTextInputs;


    
    /*
     *  TimeLineSlider
     *
     */
    
    class TimeLineSlider : public Rectf
    {
    public:
        TimeLineSlider()
        {
            init ();
        }
        
        TimeLineSlider(Rectf& bounds) : Rectf(bounds)
        {
            init ();
        }
        
        void init ()
        {
            mHidden = false;
            mValue = mValueScaled = 0.0f;
            mMin = 0.0f;
            mMax = 1.0f;
            mBackgroundColor = ColorA( 0.67, 0.67, 0.67, 0.75f );
            mValueColor = ColorA( "SpringGreen", 0.75f );
            mTextColor = Color::white();
        }
        
        void setBounds (const Rectf& bounds)
        {
            static_cast<Rectf*>(this)->set(bounds.getX1(), bounds.getY1(), bounds.getX2(), bounds.getY2());
        }
        Rectf getBounds ()
        {
            return Rectf(getX1(), getY1(), getX2(), getY2());
        }
        
        void setTitle ( const std::string& title)
        {
            mTitle = title;
        }

        std::string& getTitle () const
        {
            return mTitle;
        }
        
        void set( float val ) {
            mValueScaled = val;
            mValue = ( mValueScaled - mMin ) / ( mMax - mMin );
        }
        
        bool hitTest( const vec2 &pos )
        {
            if( mHidden )
                return false;
            
            bool b = contains( pos );
            if( b ) {
                mValue = ( pos.x - x1 ) / getWidth();
                mValueScaled = (mMax - mMin) * mValue + mMin;
            }
            return b;
        }
        
        void get_marker_position (marker_info& t) const
        {
            t.from_norm (mValue);
        }
        
        
        void set_marker_position (marker_info t)
        {
//            std::cout << t.current_frame() << "/" << t.count() << std::endl;
            set (t.norm_index());
        }
        
        void clear_timepoint_markers ()
        {
            mTimePoints.clear();
            std::queue<uint32_t> empty;
            std::swap(m_available_timepoint_indicies, empty);
        }
        void clear_timepoint_marker (const uint32_t& map_index)
        {
            map<uint32_t, timepoint_marker_t>::iterator iter = mTimePoints.find(map_index) ;
            if( iter != mTimePoints.end() )
            {
                mTimePoints.erase( iter );
                m_available_timepoint_indicies.push(map_index);
            }
        }
        uint32_t add_timepoint_marker (const timepoint_marker_t& tm)
        {
            // Find the next available map point
            auto count = mTimePoints.size();
            if (! m_available_timepoint_indicies.empty())
            {
                count = m_available_timepoint_indicies.front();
                m_available_timepoint_indicies.pop();
            }

            mTimePoints[(uint32_t)count] = tm;
            return (uint32_t)count;
        }
        
        const float & value () const { return mValue; }
        const float & valueScaled () const { return mValueScaled; }
        
        void draw()
        {
            if( mHidden )
                return;
            if( ! mTexFont )
                mTexFont = tinyUi::getWidgetTexFont();
            
            {
                {
                    gl::ScopedColor A ( mBackgroundColor );
                    gl::drawSolidRect( *this );
                }
                auto valFormatted = boost::format( "%0.3f" ) % mValueScaled;
                
                std::string str = mTitle + ": " + valFormatted.str();
                {
                    gl::ScopedColor B ( mTextColor );
                    mTexFont->drawString( str, vec2( x1 + mPadding, getCenter().y + mTexFont->getFont().getDescent() ) );
                }
            }
            
            {
                gl::ScopedColor A ( ColorA(0.25f, 0.5f, 1, 1 ) );
                gl::drawStrokedRect( *this );
                
                float offset = x1 + getWidth() * mValue;
                float w = 3.0f;
                Rectf valRect( offset - w, y1, offset + w, y2 );
                gl::drawSolidRoundedRect( valRect, w );
            }

            for (const auto& kv : mTimePoints) {
                gl::ScopedColor A (kv.second.second);
                float offset = x1 + getWidth() * kv.second.first;
                float w = 3.0f;
                Rectf valRect( offset - w, y1, offset + w, y2 );
                gl::drawSolidRoundedRect( valRect, w );
            }
        }
        
    private:
        ColorA mBackgroundColor;
        gl::TextureFontRef mTexFont;
        bool mHidden;
        float mPadding;
        
        float mValue, mValueScaled, mMin, mMax;
        ColorA mTextColor, mValueColor;
        std::map<uint32_t, duration_marker_t> mDurations;
        std::map<uint32_t, timepoint_marker_t> mTimePoints;
        std::queue<uint32_t> m_available_timepoint_indicies;
        
        
        mutable std::string mTitle;
    };

    typedef TimeLineSlider Widget;
    typedef std::shared_ptr<Widget> WidgetRef;
    
    inline void drawWidgets( const std::vector<Widget *> &widgets )
    {
        for( auto w : widgets )
            w->draw();
    }
    
    
 
}



#endif
