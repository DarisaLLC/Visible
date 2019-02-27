
#pragma once

#include "cinder/Vector.h"
#include "cinder/PolyLine.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/Batch.h"
#include <iterator>
#include <vector>

using namespace ci;

class TimeSeriesPlot {
  public:
    typedef std::vector<float>::const_iterator pIter_t;
    
    TimeSeriesPlot();
    
    void update(const std::vector<float> &);
    
	void setBounds( const ci::Rectf &bounds )	{ mBounds = bounds; }
	const ci::Rectf& getBounds() const			{ return mBounds; }

	void enableBorder( bool b = true )			{ mBorderEnabled = b; }
	bool getBorderEnabled() const				{ return mBorderEnabled; }

	void setBorderColor( const ci::ColorA &color )	{ mBorderColor = color; }
	const ci::ColorA& getBorderColor() const		{ return mBorderColor; }

    const std::vector<float>& data () const { return m_copy; }
    
    void draw(const ci::Rectf &bounds, const pIter_t& start, const pIter_t& past_last);

    void drawAll(const ci::Rectf &bounds);
    
  private:
    std::vector<float>      m_copy;
	ci::Rectf				mBounds;
	bool					mBorderEnabled;
	ci::ColorA				mBorderColor;
};
