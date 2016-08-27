
#ifndef _iclip_H_
#define _iclip_H_


#include "cinder/gl/gl.h"
#include "cinder/Color.h"
#include "cinder/Rect.h"
#include "cinder/Function.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/TextureFont.h"
#include "InteractiveObject.h"


using namespace std;
using namespace ci;
using namespace ci::app;
class graph1D;

typedef std::shared_ptr<graph1D> Graph1DRef;

class graph1D:  public InteractiveObject
{
public:
    
	graph1D( std::string name, const ci::Rectf& display_box) : InteractiveObject(display_box)
	{
		// create label
		TextLayout text; text.clear( cinder::Color::white() ); text.setColor( Color(0.5f, 0.5f, 0.5f) );
		try { text.setFont( Font( "Futura-CondensedMedium", 18 ) ); } catch( ... ) { text.setFont( Font( "Arial", 18 ) ); }
		text.addLine( name );
        mLabelTex = cinder::gl::Texture::create(text.render( true ) );
        mContentScale = vec2(0.9, 0.8);
	}
    
	void load_vector (const std::vector<float>& buffer)
    {
        std::unique_lock<std::mutex> lock (mutex_);
        mBuffer.clear ();
        std::vector<float>::const_iterator reader = buffer.begin ();
        while (reader != buffer.end())
        {
            mBuffer.push_back (*reader++);
        }
        
        mFn = bind (&graph1D::get, std::placeholders::_1, std::placeholders::_2);
        lock.unlock();
        cond_.notify_one ();
    }
    
    //  bool is_valid () const { return (mFn != std::function<float (float)> () ); }
    
    float get (float tnormed) const
    {
        const std::vector<float>& buf = buffer();
        if (empty()) return -1.0;
        
        // NN
        int32_t x = floor (tnormed * (buf.size()-1));
        if (x >= 0 && x < buf.size())
            return buf[x];
        else
            return -1.0f;
    }
	void draw( float t ) const
	{
        Rectf content = rect.scaledCentered(mContentScale.x);
        
		// draw box and frame
		cinder::gl::color( ci::ColorA( 0.5f, 0.5f, 0.5f, 0.5f) );
		cinder::gl::drawSolidRect( content );
		cinder::gl::color( ci::Color( 0.0f, 0.0f, 0.0f ) );
        ci::gl::drawStrokedRect( content, 3.0f);
        ci::gl::color( ci::Color::white() );
        ci::gl::draw( mLabelTex, vec2(content.getCenter()[0] - mLabelTex->getSize()[0] / 2, rect.getHeight() - mLabelTex->getSize()[1] ));
        
		// draw graph
		gl::color( ColorA( 0.0f, 0.0f, 1.0f, 1.0f ) );
        gl::begin( GL_LINE_STRIP );
		for( float x = 0; x < content.getWidth(); x += 0.25f ) {
			float y = 1.0f - mFn ( this, x / content.getWidth() );
            ci::gl::vertex(vec2( x, y * content.getHeight() ) + content.getUpperLeft() );
		}
        gl::end();
		
		// draw animating circle
		gl::color( Color( 0.75f, 0.5f, 0.25f ) );
		//gl::drawSolidCircle( rect.getUpperLeft() + mFn ( this, t ) * rect.getSize(), 5.0f );
        glLineWidth(25.f);
        float px = norm_pos().x * rect.getWidth();
        vec2 mid (px, rect.getHeight()/2.0);
        if (content.contains(mid))
            ci::gl::drawLine (vec2(px, 0.f), vec2(px, rect.getHeight()));
        
    }
    
    const std::vector<float>&       buffer () const { return mBuffer; }
    bool empty () const { return mBuffer.empty (); }
    vec2 mContentScale;
	
    std::vector<float>                   mBuffer;
	cinder::gl::TextureRef						mLabelTex;
    std::function<float (const graph1D*, float)> mFn;
    std::condition_variable cond_;
    mutable std::mutex mutex_;
    
};



class Slider : public InteractiveObject {
    
public:
    Slider( ) : InteractiveObject( Rectf(0,0, 100,10) )
    {
        mValue = 0.f;
    }
    
    vec2   getPosition() { return rect.getUpperLeft(); }
    void    setPosition(vec2 position) { rect.offset(position); }
    void    setPosition(float x, float y) { setPosition(vec2(x,y)); }
    
    float   getWidth() { return getSize().x; }
    float   getHeight() { return getSize().y; }
    vec2   getSize() { return rect.getSize(); }
    void    setSize(vec2 size) { rect.x2 = rect.x1+size.x; rect.y2 = rect.y1+size.y; }
    void    setSize(float width, float height) { setSize(vec2(width,height)); }
    
    
    virtual float getValue()
    {
        return mValue;
    }
    
    virtual void setValue(float value)
    {
        if(mValue != value) {
            mValue = ci::math<float>::clamp(value);
            changed();
            mEvents.call( InteractiveObjectEvent( this, InteractiveObjectEvent::Changed ) );
        }
    }
    
    virtual void pressed()
    {
        InteractiveObject::pressed();
        dragged();
    }
    
    virtual void dragged()
    {
        InteractiveObject::dragged();
        
      vec2 mousePos = App::get()->getMousePos();
        setValue( (mousePos.x - rect.x1) / rect.getWidth() );
    }
    
    virtual void changed()
    {
        console() << "Slider::changed" << endl;
    }
    
    virtual void draw()
    {
        gl::color(Color::gray(0.7f));
        gl::drawSolidRect(rect);
        
        gl::color(Color::black());
        Rectf fillRect = Rectf(rect);
        fillRect.x2 = fillRect.x1 + fillRect.getWidth() * mValue;
        gl::drawSolidRect( fillRect );
    }
    
protected:
    float mValue;
};


class Button: public InteractiveObject{
public:
    Button( const ci::Rectf& rect, ci::gl::TextureRef idleTex, ci::gl::TextureRef overTex, ci::gl::TextureRef pressTex ):InteractiveObject( rect )
    {
        mIdleTex = idleTex;
        mOverTex = overTex;
        mPressTex = pressTex;
    }
    
    virtual void draw(){
        if( mPressed ){
            ci::gl::draw( mPressTex, rect );
        } else if( mOver ){
            ci::gl::draw( mOverTex, rect );
        } else {
            ci::gl::draw( mPressTex, rect );
        }
    }
    
private:
    ci::gl::TextureRef mIdleTex, mOverTex, mPressTex;
};

#endif // _iclip_H_
