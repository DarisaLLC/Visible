
#pragma once

#include "cinder/Vector.h"
#include "cinder/PolyLine.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/Batch.h"
#include <iterator>
#include <vector>

using namespace ci;

class OnImagePlot {
  public:
    typedef std::vector<float>::const_iterator pIter_t;
    
    OnImagePlot();
    
	void setBounds( const ci::Rectf &bounds )	{ mBounds = bounds; }
	const ci::Rectf& getBounds() const			{ return mBounds; }

	void enableBorder( bool b = true )			{ mBorderEnabled = b; }
	bool getBorderEnabled() const				{ return mBorderEnabled; }

	void setBorderColor( const ci::ColorA &color )	{ mBorderColor = color; }
	const ci::ColorA& getBorderColor() const		{ return mBorderColor; }

    const std::vector<float>& data () const { return m_copy; }
    
    void draw(const std::vector<float>& data);

    
  private:
    std::vector<float>      m_copy;
	ci::Rectf				mBounds;
	bool					mBorderEnabled;
	ci::ColorA				mBorderColor;
};
