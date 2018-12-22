
#pragma once

#include "cinder/Vector.h"
#include "cinder/PolyLine.h"
#include "cinder/gl/VboMesh.h"
#include <iterator>
#include <vector>


class TimeSeriesPlot {
  public:
    typedef std::vector<float>::const_iterator pIter_t;
    
    TimeSeriesPlot(const std::vector<float> &, const ci::Rectf &bounds);
    
	void setBounds( const ci::Rectf &bounds )	{ mBounds = bounds; }
	const ci::Rectf& getBounds() const			{ return mBounds; }

	void enableBorder( bool b = true )			{ mBorderEnabled = b; }
	bool getBorderEnabled() const				{ return mBorderEnabled; }

	void setBorderColor( const ci::ColorA &color )	{ mBorderColor = color; }
	const ci::ColorA& getBorderColor() const		{ return mBorderColor; }

    void draw( const pIter_t& start, const pIter_t& past_last);

  private:
    std::vector<float>      m_copy;
	ci::Rectf				mBounds;
	bool					mBorderEnabled;
	ci::ColorA				mBorderColor;
};
