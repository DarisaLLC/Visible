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
    

    
    
    static gl::TextureFontRef getWidgetTexFont() {
        static gl::TextureFontRef sWidgetTexFont;
        if( ! sWidgetTexFont )
            sWidgetTexFont = gl::TextureFont::create( Font( Font::getDefault().getName(), 22 ) );
        return sWidgetTexFont;
    }
    
    static std::vector<struct TextInput *> sTextInputs;
    
    struct Widget {
        Widget() : mHidden( false ), mPadding( 10.0f ) {}
        
        virtual void draw() {}
        
        Rectf mBounds;
        ColorA mBackgroundColor;
        
        gl::TextureFontRef mTexFont;
        bool mHidden;
        float mPadding;
    };

    typedef std::shared_ptr<Widget> WidgetRef;
    
    inline void drawWidgets( const std::vector<Widget *> &widgets )
    {
        for( auto w : widgets )
            w->draw();
    }
    
    struct Button : public Widget {
        Button( bool isToggle = false, const std::string& titleNormal = "", const std::string& titleEnabled = "" )
        : Widget(), mIsToggle( isToggle ), mTitleNormal( titleNormal ), mTitleEnabled( titleEnabled )
        {
            mTextColor = Color::white();
            mNormalColor = Color( "SlateGray" );
            mEnabledColor = Color( "RoyalBlue" );
            setEnabled( false );
            mTimeout = 30;
            mFadeFrames = 0;
        }
        
        void setEnabled( bool b )
        {
            if( b ) {
                mBackgroundColor = mEnabledColor;
            } else {
                mBackgroundColor = mNormalColor;
            }
            mEnabled = b;
        }
        
        bool hitTest( const vec2 &pos )
        {
            if( mHidden )
                return false;
            
            bool b = mBounds.contains( pos );
            if( b ) {
                if( mIsToggle )
                    setEnabled( ! mEnabled );
                else {
                    setEnabled( true );
                    mFadeFrames = mTimeout;
                }
            }
            
            return b;
        }
        
        void draw()
        {
            if( mHidden )
                return;
            if( ! mTexFont )
                mTexFont = tinyUi::getWidgetTexFont();
            
            if( mIsToggle || ! mFadeFrames )
                gl::color( mBackgroundColor );
            else {
                mFadeFrames--;
                setEnabled( false );
                gl::color( lerp( mNormalColor, mEnabledColor, (float)mFadeFrames / (float)mTimeout ) );
            }
            
            gl::drawSolidRoundedRect( mBounds, 4 );
            
            std::string& title = mEnabled ? mTitleEnabled : mTitleNormal;
            
            gl::color( mTextColor );
            mTexFont->drawString( title, vec2( mBounds.x1 + mPadding, mBounds.getCenter().y + mTexFont->getFont().getDescent() ) );
        }
        
        ColorA mTextColor;
        std::string mTitleNormal, mTitleEnabled;
        ColorA mNormalColor, mEnabledColor;
        bool mEnabled, mIsToggle;
        size_t mTimeout, mFadeFrames;
    };
    
    struct HSlider : public Widget {
        HSlider() : Widget()
        {
            mValue = mValueScaled = 0.0f;
            mMin = 0.0f;
            mMax = 1.0f;
            mBackgroundColor = ColorA( "DarkGreen", 0.75f );
            mValueColor = ColorA( "SpringGreen", 0.75f );
            mTextColor = Color::white();
        }
        
        void set( float val ) {
            mValueScaled = val;
            mValue = ( mValueScaled - mMin ) / ( mMax - mMin );
        }
        
        bool hitTest( const vec2 &pos )
        {
            if( mHidden )
                return false;
            
            bool b = mBounds.contains( pos );
            if( b ) {
                mValue = ( pos.x - mBounds.x1 ) / mBounds.getWidth();
                mValueScaled = (mMax - mMin) * mValue + mMin;
            }
            return b;
        }
        
        void draw()
        {
            if( mHidden )
                return;
            if( ! mTexFont )
                mTexFont = tinyUi::getWidgetTexFont();
            
            gl::color( mBackgroundColor );
            gl::drawSolidRect( mBounds );
            
            auto valFormatted = boost::format( "%0.3f" ) % mValueScaled;
            
            std::string str = mTitle + ": " + valFormatted.str();
            gl::color( mTextColor );
            mTexFont->drawString( str, vec2( mBounds.x1 + mPadding, mBounds.getCenter().y + mTexFont->getFont().getDescent() ) );
            
            gl::color( mValueColor );
            gl::drawStrokedRect( mBounds );
            
            float offset = mBounds.x1 + mBounds.getWidth() * mValue;
            float w = 2.0f;
            Rectf valRect( offset - w, mBounds.y1, offset + w, mBounds.y2 );
            gl::drawSolidRoundedRect( valRect, w );
        }
        
        float mValue, mValueScaled, mMin, mMax;
        ColorA mTextColor, mValueColor;
        std::string mTitle;
    };
    
    
    /*
     *  TimeLineSlider
     *
     */
    
    struct TimeLineSlider : public Widget {

        TimeLineSlider() : Widget()
        {
            init ();
        }
        
        TimeLineSlider(Rectf& bounds) : Widget()
        {
            init ();
            mBounds = bounds;
        }
        
        void init ()
        {
            mValue = mValueScaled = 0.0f;
            mMin = 0.0f;
            mMax = 1.0f;
            mBackgroundColor = ColorA( 0.67, 0.67, 0.67, 0.75f );
            mValueColor = ColorA( "SpringGreen", 0.75f );
            mTextColor = Color::white();
        }
        
        void set( float val ) {
            mValueScaled = val;
            mValue = ( mValueScaled - mMin ) / ( mMax - mMin );
        }
        
        bool hitTest( const vec2 &pos )
        {
            if( mHidden )
                return false;
            
            bool b = mBounds.contains( pos );
            if( b ) {
                mValue = ( pos.x - mBounds.x1 ) / mBounds.getWidth();
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
            std::cout << t.current_frame() << "/" << t.count() << std::endl;
            set (t.norm_index());
        }
        
        void draw()
        {
            if( mHidden )
                return;
            if( ! mTexFont )
                mTexFont = tinyUi::getWidgetTexFont();
            
            {
                {
                    gl::ScopedColor A ( mBackgroundColor );
                    gl::drawSolidRect( mBounds );
                }
                auto valFormatted = boost::format( "%0.3f" ) % mValueScaled;
                
                std::string str = mTitle + ": " + valFormatted.str();
                {
                    gl::ScopedColor B ( mTextColor );
                    mTexFont->drawString( str, vec2( mBounds.x1 + mPadding, mBounds.getCenter().y + mTexFont->getFont().getDescent() ) );
                }
            }
            
            {
                gl::ScopedColor A ( ColorA(0.25f, 0.5f, 1, 1 ) );
                gl::drawStrokedRect( mBounds );
                
                float offset = mBounds.x1 + mBounds.getWidth() * mValue;
                float w = 3.0f;
                Rectf valRect( offset - w, mBounds.y1, offset + w, mBounds.y2 );
                gl::drawSolidRoundedRect( valRect, w );
            }
        }
        
        float mValue, mValueScaled, mMin, mMax;
        ColorA mTextColor, mValueColor;
        std::map<int, duration_marker_t> mMarker;
        std::string mTitle;
    };
    
    
  
    
    
    
    
    struct VSelector : public Widget {
        VSelector() : Widget()
        {
            mCurrentSectionIndex = 0;
            mBackgroundColor = ColorA( "MidnightBlue", 0.75f );
            mSelectedColor = ColorA( "SpringGreen", 0.95f );
            mUnselectedColor = ColorA::gray( 0.5 );
            mTitleColor = ColorA::gray( 0.75f, 0.5f );
        }
        
        bool hitTest( const vec2 &pos )
        {
            if( mHidden )
                return false;
            
            bool b = mBounds.contains( pos );
            if( b ) {
                int offset = pos.y - (int)mBounds.y1;
                int sectionHeight = (int)mBounds.getHeight() / mSegments.size();
                mCurrentSectionIndex = std::min<size_t>( offset / sectionHeight, mSegments.size() - 1 );
            }
            return b;
        }
        
        std::string currentSection() const
        {
            if( mSegments.empty() )
                return "";
            
            return mSegments[mCurrentSectionIndex];
        }
        
        void draw()
        {
            if( mHidden )
                return;
            
            if( ! mTexFont )
                mTexFont = tinyUi::getWidgetTexFont();
            
            gl::color( mBackgroundColor );
            gl::drawSolidRect( mBounds );
            
            float sectionHeight = mBounds.getHeight() / mSegments.size();
            Rectf section( mBounds.x1, mBounds.y1, mBounds.x2, mBounds.y1 + sectionHeight );
            gl::color( mUnselectedColor );
            for( size_t i = 0; i < mSegments.size(); i++ ) {
                if( i != mCurrentSectionIndex ) {
                    gl::drawStrokedRect( section );
                    gl::color( mUnselectedColor );
                    mTexFont->drawString( mSegments[i], vec2( section.x1 + mPadding, section.getCenter().y + mTexFont->getFont().getDescent() ) );
                }
                section += vec2( 0.0f, sectionHeight );
            }
            
            gl::color( mSelectedColor );
            
            section.y1 = mBounds.y1 + mCurrentSectionIndex * sectionHeight;
            section.y2 = section.y1 + sectionHeight;
            gl::drawStrokedRect( section );
            
            if( ! mSegments.empty() ) {
                gl::color( mSelectedColor );
                mTexFont->drawString( mSegments[mCurrentSectionIndex], vec2( section.x1 + mPadding, section.getCenter().y + mTexFont->getFont().getDescent() ) );
            }
            
            if( ! mTitle.empty() ) {
                gl::color( mTitleColor );
                mTexFont->drawString( mTitle, vec2( mBounds.x1 + mPadding, mBounds.y1 - mTexFont->getFont().getDescent() ) );
            }
        }
        
        std::vector<std::string> mSegments;
        ColorA mSelectedColor, mUnselectedColor, mTitleColor;
        size_t mCurrentSectionIndex;
        std::string mTitle;
    };
    
    struct TextInput : public Widget {
        enum Format { NUMERICAL, ALL };
        
        TextInput( Format format = Format::NUMERICAL, const std::string& title = "" )
        : Widget(), mFormat( format ), mTitle( title )
        {
            mBackgroundColor = ColorA( "MidnightBlue", 0.65f );
            mTitleColor = ColorA::gray( 0.75f, 0.5f );
            mNormalColor = Color( "SlateGray" );
            mEnabledColor = ColorA( "SpringGreen", 0.95f );
            setSelected( false );
            
            tinyUi::sTextInputs.push_back( this );
        }
        
        void setSelected( bool b )
        {
            disableAll();
            mSelected = b;
        }
        
        bool hitTest( const vec2 &pos )
        {
            if( mHidden )
                return false;
            
            bool b = mBounds.contains( pos );
            if( b ) {
                setSelected( true );
            }
            
            return b;
        }
        
        void setValue( int value )
        {
            mInputString = std::to_string( value );
        }
        
        int getValue()
        {
            if( mInputString.empty() )
                return 0;
            
            return stoi( mInputString );
        }
        
        void processChar( char c )
        {
            if( mFormat == Format::NUMERICAL && ! isdigit( c ) )
                return;
            
            mInputString.push_back( c );
        }
        
        void processBackspace()
        {
            if( ! mInputString.empty() )
                mInputString.pop_back();
        }
        
        static void disableAll()
        {
            for( TextInput *t : tinyUi::sTextInputs )
                t->mSelected = false;
        }
        
        static TextInput *getCurrentSelected()
        {
            for( TextInput *t : tinyUi::sTextInputs ) {
                if( t->mSelected )
                    return t;
            }
            
            return nullptr;
        }
        
        void draw()
        {
            if( mHidden )
                return;
            if( ! mTexFont )
                mTexFont = tinyUi::getWidgetTexFont();
            
            gl::color( mBackgroundColor );
            gl::drawSolidRect( mBounds );
            
            vec2 titleOffset = vec2( mBounds.x1 + mPadding, mBounds.y1 - mTexFont->getFont().getDescent() );
            gl::color( mTitleColor );
            mTexFont->drawString( mTitle, titleOffset );
            
            vec2 textOffset = vec2( mBounds.x1 + mPadding, mBounds.getCenter().y + mTexFont->getFont().getDescent() );
            
            gl::color( ( mSelected ? mEnabledColor : mNormalColor ) );
            
            gl::drawStrokedRect( mBounds );
            mTexFont->drawString( mInputString, textOffset );
        }
        
        Format	mFormat;
        std::string mTitle, mInputString;
        ColorA mNormalColor, mEnabledColor, mTitleColor;
        bool mSelected;
        
        
    };
    
}



#endif
