/*
 * Code Copyright 2011 Robert Hodgin ( http://roberthodgin.com )
 * Used with permission for the Cinder Project ( http://libcinder.org )
 */

#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "Item.h"

using namespace ci;
using namespace std;

Font Item::sSmallFont;
Font Item::sBigFont;

Item::Item( int index, vec2 pos, const string &title, const string &desc, Surface palette )
	: mIndex(index), mTitle( title ), mDesc( desc ), mPalette(palette)
{
	// TODO: can you reuse layouts? If so, how do you clear the text?
	// textures
	TextLayout layout;
	layout.clear( ColorA( 1, 1, 1, 0 ) );
	layout.setFont( sSmallFont );
	layout.setColor( Color::white() );
	layout.addLine( mTitle );
	mTitleTex = gl::Texture::create( layout.render( true ) );
	
	TextLayout bigLayout;
	bigLayout.clear( ColorA( 1, 1, 1, 0 ) );
	bigLayout.setFont( sBigFont );
	bigLayout.setColor( Color::white() );
	bigLayout.addLine( mTitle );
	mTitleBigTex = gl::Texture::create( bigLayout.render( true ) );
	
	// title
	mTitleStart		= pos;
	mTitleDest1		= vec2( pos.x - 25.0f, pos.y );
	mTitleFinish	= vec2( pos.x - 4.0f, 120.0f );
	mTitleDest2		= vec2( mTitleFinish.x - 25.0f, mTitleFinish.y );
	mMouseOverDest	= mTitleStart + vec2( 7.0f, 0.0f );
	mTitlePos		= mTitleStart;
	mTitleColor		= Color( 0.7f, 0.7f, 0.7f );
	mTitleAlpha		= 1.0f;
	mTitleWidth		= mTitleTex->getWidth();
	mTitleHeight	= mTitleTex->getHeight();
	
	// desc
	mDescStart		= vec2( mTitleStart.x + 25.0f, mTitleFinish.y + mTitleBigTex->getHeight() + 5.0f );
	mDescDest		= vec2( mTitleStart.x + 35.0f, mDescStart.y );
	mDescPos		= mDescStart;
	mDescAlpha		= 0.0f;
	TextBox tbox	= TextBox().alignment( TextBox::LEFT ).font( sSmallFont ).size( ivec2( 650.0f, TextBox::GROW ) ).text( mDesc );
	mDescTex		= gl::Texture::create( tbox.render() );
	
	// bar
	mBarPos			= pos - vec2( 4.0f, 1.0f );
	mBarWidth		= 0.0f;
	mBarHeight		= mTitleHeight + 2.0f;
	mMaxBarWidth	= mTitleWidth + 22.0f;
	mBarColor		= Color::white();
	mBarAlpha		= 0.3f;
	
	mFadeFloat		= 1.0f;
	mIsSelected		= false;
	mIsBeingSelected = false;

	setColors();
}

void Item::setColors()
{
	Surface::Iter iter = mPalette.getIter();
	while( iter.line() ) {
		while( iter.pixel() ) {
			int index = iter.x();
			Color col( iter.r()/255.0f, iter.g()/255.0f, iter.b()/255.0f );
			vec2 pos = vec2( index%4, floor(index/4.0f) ) * 5.0f - vec2( 12.0f, 8.0f );
			Rectf rect( -2.0f, -2.0f, 2.0f, 2.0f );
			Swatch swatch( col, mTitleStart + pos + vec2( -10.0f, 10.0f ), rect );
			mSwatches.push_back( swatch );
		}
	}
}

bool Item::isPointIn( const vec2 &pt ) const
{
	return mTitleArea.contains( pt );
}

void Item::mouseOver( Timeline &timeline )
{
	if( !mIsBeingSelected ){
		float dur = 0.2f;
		
		// title
		timeline.apply( &mTitlePos, mMouseOverDest, dur, EaseOutAtan( 10 ) );
		timeline.apply( &mTitleColor, Color( 1, 1, 1 ), 0.05f, EaseOutAtan( 10 ) );
		
		// bar
		timeline.apply( &mBarColor, Color( 0, 0, 0 ), dur, EaseOutAtan( 10 ) );
		timeline.apply( &mBarAlpha, 0.4f, dur, EaseOutAtan( 10 ) );
		timeline.apply( &mBarWidth, mMaxBarWidth, dur, EaseOutAtan( 10 ) );

		for( std::vector<Swatch>::iterator swatchIt = mSwatches.begin(); swatchIt != mSwatches.end(); ++swatchIt ) {
			swatchIt->mouseOver( timeline );
		}
	}
}

void Item::mouseOff( Timeline &timeline )
{
	if( !mIsBeingSelected ){
		float dur = 0.4f;
		
		// title
		timeline.apply( &mTitlePos, mTitleStart, dur, EaseOutBounce( 1.0f ) );
		timeline.apply( &mTitleColor, Color( 0.7f, 0.7f, 0.7f ), 0.05f, EaseOutAtan( 10 ) );
		
		// bar
		timeline.apply( &mBarColor, Color( 0, 0, 0 ), 0.2f, EaseOutAtan( 10 ) );
		timeline.apply( &mBarAlpha, 0.0f, 0.2f, EaseOutAtan( 10 ) );
		timeline.apply( &mBarWidth, 0.0f, 0.2f, EaseInAtan( 10 ) );

		for( std::vector<Swatch>::iterator swatchIt = mSwatches.begin(); swatchIt != mSwatches.end(); ++swatchIt ) {
			swatchIt->mouseOff( timeline );
		}
	}
}

void Item::select( Timeline &timeline, float leftBorder )
{
	if( !mIsSelected ){
		mIsBeingSelected = true;
		
		mFadeFloat = 0.0f;
		
		float dur1 = 0.2f;
		float dur2 = 0.2f;
		
		// title position
		// set selected after initial animation
		timeline.apply( &mTitlePos, mTitleDest1, dur1, EaseInAtan( 10 ) ).finishFn( bind( &Item::setSelected, this ) );
		timeline.appendTo( &mTitlePos, mTitleDest2 - vec2( 25.0f, 0.0f ), 0.0001f, EaseNone() );
		timeline.appendTo( &mTitlePos, mTitleFinish, dur2, EaseOutAtan( 10 ) );
		// title color
		timeline.apply( &mTitleColor, Color( 1, 1, 1 ), dur1, EaseInQuad() );
		// title alpha
		timeline.apply( &mTitleAlpha, 0.0f, dur1, EaseInAtan( 10 ) );
		timeline.appendTo( &mTitleAlpha, 1.0f, dur2, EaseOutAtan( 10 ) );
		
		// desc position
		timeline.apply( &mDescPos, mDescDest, dur2, EaseOutAtan( 10 ) ).timelineEnd( -dur1 );
		// desc alpha
		timeline.apply( &mDescAlpha, 1.0f, dur2, EaseOutQuad() ).timelineEnd( -dur1*0.5f );
		
		// fadeFloat (used to adjust dropshadow offset)
		timeline.apply( &mFadeFloat, 0.0f, dur1, EaseInAtan( 10 ) );
		timeline.appendTo( &mFadeFloat, 0.0f, 0.25f, EaseNone() );
		timeline.appendTo( &mFadeFloat, 1.0f, dur2, EaseOutQuad() );
		
		// bar width
		timeline.apply( &mBarWidth, 0.0f, dur1, EaseInOutAtan( 10 ) );

		for( std::vector<Swatch>::iterator swatchIt = mSwatches.begin(); swatchIt != mSwatches.end(); ++swatchIt ) {
			swatchIt->scatter( timeline, math<float>::max( mTitleBigTex->getWidth(), 500.0f ), mTitleFinish.y + 50.0f );
		}
	}
}

void Item::deselect( Timeline &timeline )
{
	mIsBeingSelected = false;
	
	float dur1 = 0.2f;
	float dur2 = 0.2f;
	
	// set title position
	// set deselected after initial animation
	timeline.apply( &mTitlePos, mTitleDest2, dur1, EaseInAtan( 10 ) ).finishFn( bind( &Item::setDeselected, this ) );
	timeline.appendTo( &mTitlePos, mTitleDest1, 0.0001f, EaseNone() );
	timeline.appendTo( &mTitlePos, mTitleStart, dur2, EaseOutElastic( 2.0f, 0.5f ) );
	
	// set title color
	timeline.apply( &mTitleColor, Color( 1, 1, 1 ), dur1, EaseInAtan( 10 ) );
	timeline.appendTo( &mTitleColor, Color( 0.7f, 0.7f, 0.7f ), 0.05f, EaseOutAtan( 10 ) );
	
	// set title alpha
	timeline.apply( &mTitleAlpha, 0.0f, dur1, EaseInAtan( 10 ) );
	timeline.appendTo( &mTitleAlpha, 1.0f, dur2, EaseOutAtan( 10 ) );
	
	// set desc position
	timeline.apply( &mDescPos, mDescStart, dur1, EaseInAtan( 10 ) ).timelineEnd();
	
	// set desc alpha
	timeline.apply( &mDescAlpha, 0.0f, dur1, EaseInQuad() ).timelineEnd( -dur1 );
	
	// set bar
	timeline.apply( &mBarWidth, 0.0f, dur1, EaseInAtan( 10 ) );
	mBarAlpha = 0.3f;
	
	mIsSelected = false;
	
	for( std::vector<Swatch>::iterator swatchIt = mSwatches.begin(); swatchIt != mSwatches.end(); ++swatchIt ){
		swatchIt->assemble( timeline );
	}
}

void Item::update()
{
	mTitleArea	= Area( mTitlePos().x - 50.0f, mTitlePos().y - 2.0f, mTitlePos().x + mTitleWidth + 50.0f, mTitlePos().y + mTitleHeight + 2.0f );
	mBarRect	= Rectf( mBarPos.x, mBarPos.y, mBarPos.x + mBarWidth, mBarPos.y + mBarHeight );
}

void Item::drawSwatches() const
{	
	for( std::vector<Swatch>::const_iterator swatchIt = mSwatches.begin(); swatchIt != mSwatches.end(); ++swatchIt ){
		swatchIt->draw();
	}
}

void Item::drawBgBar() const
{
	gl::color( ColorA( mBarColor, mBarAlpha ) );
	gl::drawSolidRect( mBarRect );
}

void Item::drawText() const
{
	if( isBelowTextThreshold() ){
		// title shadow
		gl::color( ColorA( 0, 0, 0, mTitleAlpha() * 0.1f ) );
		gl::draw( mTitleBigTex, mTitlePos() + mFadeFloat() * vec2( -3.5f, 3.5f ) );
		gl::color( ColorA( 0, 0, 0, mTitleAlpha() * 0.2f ) );
		gl::draw( mTitleBigTex, mTitlePos() + mFadeFloat() * vec2( -2.5f, 2.5f ) );
		gl::color( ColorA( 0, 0, 0, mTitleAlpha() * 0.6f ) );
		gl::draw( mTitleBigTex, mTitlePos() + mFadeFloat() * vec2( -1.0f, 1.0f ) );
		
		// title text
		gl::color( ColorA( mTitleColor(), mTitleAlpha() ) );
		gl::draw( mTitleBigTex, mTitlePos() );
		
		// desc shadow
		gl::color( ColorA( 0, 0, 0, mDescAlpha() ) );
		gl::draw( mDescTex, mDescPos() + vec2( -1.0f, 1.0f ) );

		// desc text
		gl::color( ColorA( 1, 1, 1, mDescAlpha() ) );
		gl::draw( mDescTex, mDescPos() );
	} else {
		// drop shadow
		gl::color( ColorA( 0, 0, 0, mTitleAlpha() ) );
		gl::draw( mTitleTex, mTitlePos() + vec2( -1.0f, 1.0f ) );
		
		// white text
		gl::color( ColorA( mTitleColor(), mTitleAlpha() ) );
		gl::draw( mTitleTex, mTitlePos() );
	}
}

bool Item::isBelowTextThreshold() const
{
	if( mTitlePos().y <= mTitleFinish.y + 1.0f ){
		return true;
	}
	return false;
}

void Item::setFonts( const Font &smallFont, const Font &bigFont )
{
	sSmallFont	= smallFont;
	sBigFont	= bigFont;
}

const ci::Font& Item::small_font () { return sSmallFont; }
const ci::Font& Item::big_font () { return sBigFont; }



float quickRound( float n )
{
    return floor(n+0.5f);
}

AccordionItem::AccordionItem( Timeline &timeline, float x, float y, float height, float contractedWidth, float expandedWidth, gl::TextureRef image, string title, string subtitle )
    : mTimeline(timeline), mX(x), mY(y), mWidth(contractedWidth), mHeight(height),  mImage(image), mTitle(title), mSubtitle(subtitle)
{
#if defined( CINDER_COCOA )
    std::string normalFont( "Arial" );
    std::string boldFont( "Arial-BoldMT" );
#else
    std::string normalFont( "Arial" );
    std::string boldFont( "ArialBold" );
#endif
    
    mAnimEase = EaseOutAtan(25);
    mAnimDuration = 0.7f;
    
    mTextAlpha = 0.0f;
    
    TextLayout layout;
    layout.clear( ColorA( 0.6f, 0.6f, 0.6f, 0.0f ) );
    layout.setFont( Font( boldFont, 26 ) );
    layout.setColor( Color( 1, 1, 1 ) );
    layout.addLine( mTitle );
    layout.setFont( Font( normalFont, 16 ) );
    layout.addLine( mSubtitle );
    layout.setBorder(11, 6);
    mText = gl::Texture::create( layout.render( true ) );
    
    update();
}

bool AccordionItem::isPointIn( const vec2 &pt )
{
    return mImageArea.contains( pt );
}

void AccordionItem::animTo( float newX, float newWidth, bool revealText )
{
    mTimeline.apply( &mX, newX, mAnimDuration, mAnimEase );
    mTimeline.apply( &mWidth, newWidth, mAnimDuration, mAnimEase );
    
    if (revealText)
        mTimeline.apply( &mTextAlpha, 1.0f, mAnimDuration*0.3f, EaseNone() );
    else
        mTimeline.apply( &mTextAlpha, 0.0f, mAnimDuration*0.3f, EaseNone() );
}

void AccordionItem::update()
{
    // sample area of image texture to render
    mImageArea = Area(quickRound(mX), quickRound(mY), quickRound(mX + mWidth), quickRound(mY + mHeight));
    
    // rectangle to render text texture
    mTextRect = Rectf(quickRound(mX), quickRound(mY), quickRound( mX + math<float>::min( mWidth, mText->getWidth() ) ), quickRound( mY + math<float>::min( mHeight, mText->getHeight() ) ) );
    // sample area of text texture to render
    mTextArea = Area(0, 0, mTextRect.getWidth(), mTextRect.getHeight() );
}

void AccordionItem::draw()
{
    gl::color( Color(1,1,1) );
    gl::draw( mImage, mImageArea, Rectf(mImageArea) );
    gl::color( ColorA(1,1,1,mTextAlpha) );
    gl::draw( mText, mTextArea, mTextRect );
}

#include <boost/foreach.hpp>



Swatch::Swatch( Color color, vec2 pos, Rectf rect )
  : mColor( color ), mPos( pos ), mAnchorPos( pos ), mRect( rect ), mScale( 1.0f )
{
    mAlpha = 1.0f;
    mIsSelected = false;
}

void Swatch::mouseOver( Timeline &timeline )
{
    timeline.apply( &mScale, 1.0f, 0.2f, EaseOutAtan( 10 ) );
}

void Swatch::mouseOff( Timeline &timeline )
{
    timeline.apply( &mScale, 1.0f, 0.2f, EaseOutAtan( 10 ) );
}

void Swatch::scatter( Timeline &timeline, float width, float height )
{
    vec2 pos1( 0.0f, mPos().y );
    vec2 pos2( 0.0f, height );
    vec2 pos3 = vec2( Rand::randFloat( width ), Rand::randFloat( height - 50.0f, height + 50.0f ) );//pos2 + rVec;
    
    float dur = Rand::randFloat( 0.25f, 0.5f );
    
    // pos
    timeline.apply( &mPos, mPos(), 0.2f, EaseInAtan( 10 ) ).finishFn( bind( &Swatch::setSelected, this ) );
    timeline.appendTo( &mPos, pos2, 0.0001f, EaseNone() );
    timeline.appendTo( &mPos, pos3, dur, EaseOutAtan( 20 ) );
    
    // alpha
    timeline.apply( &mAlpha, 0.0f, 0.2f, EaseInAtan( 10 ) );
    timeline.appendTo( &mAlpha, 1.0f, dur, EaseOutAtan( 10 ) );
    
    // scale
    float s = (float)Rand::randInt( 3, 7 );
    timeline.apply( &mScale, 1.0f, 0.2f, EaseInAtan( 10 ) );
    timeline.appendTo( &mScale, s*s, dur, EaseOutAtan( 20 ) );
}

void Swatch::assemble( Timeline &timeline )
{
    float dur = 0.2f;
//    mPos = mAnchorPos + vec2( 15.0f, 0.0f );
    timeline.apply( &mPos, mAnchorPos, dur, EaseOutAtan( 10 ) ).finishFn( bind( &Swatch::setDeselected, this ) );
    timeline.apply( &mScale, 1.0f, dur, EaseOutAtan( 10 ) );
}

void Swatch::draw() const
{
    Rectf scaledRect = mRect * mScale;
    if( !mIsSelected ){
        gl::color( ColorA( Color::black(), mAlpha ) );
        gl::drawSolidRect( scaledRect.inflated( vec2( 1.0f, 1.0f ) ) + mPos );
    }
    gl::color( ColorA( mColor, mAlpha ) );
    gl::drawSolidRect( scaledRect + mPos );
}
