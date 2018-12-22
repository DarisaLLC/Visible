
#include "TimeSeriesPlotUtils.h"

#include "cinder/CinderMath.h"
#include "cinder/Triangulate.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Shader.h"

using namespace std;
using namespace ci;


// ----------------------------------------------------------------------------------------------------
// MARK: - TimeSeriesPlot
// ----------------------------------------------------------------------------------------------------

TimeSeriesPlot::TimeSeriesPlot(const std::vector<float> & data, const ci::Rectf &bounds)
	: m_copy(data), mBorderEnabled( true ), mBorderColor( 0.5f, 0.5f, 0.5f, 1 ), mBounds(bounds)
{
}

void TimeSeriesPlot::draw(  const pIter_t& start, const pIter_t& past_last )
{
	if( m_copy.empty() )
		return;

	gl::ScopedGlslProg glslScope( getStockShader( gl::ShaderDef().color() ) );

	ColorA bottomColor( 0, 0, 0.7f, 1 );

	float width = mBounds.getWidth();
	float height = mBounds.getHeight();
    
    size_t numBins = past_last - start;
	float padding = 0;
	float binWidth = ( width - padding * ( numBins - 1 ) ) / (float)numBins;

	gl::VertBatch batch( GL_TRIANGLE_STRIP );

	size_t currVertex = 0;
	float m;
	Rectf bin( mBounds.x1, mBounds.y1, mBounds.x1 + binWidth, mBounds.y2 );
    for (auto be = start; be < past_last; be++)
	for( size_t i = 0; i < numBins; i++ ) {
        m = *be;
		bin.y1 = bin.y2 - m * height;

		batch.color( bottomColor );
		batch.vertex( bin.getLowerLeft() );
		batch.color( 0, m, 0.7f );
		batch.vertex( bin.getUpperLeft() );

		bin += vec2( binWidth + padding, 0 );
		currVertex += 2;
	}

	batch.color( bottomColor );
	batch.vertex( bin.getLowerLeft() );
	batch.color( 0, m, 0.7f );
	batch.vertex( bin.getUpperLeft() );

	gl::color( 0, 0.9f, 0 );

	batch.draw();

	if( mBorderEnabled ) {
		gl::color( mBorderColor );
		gl::drawStrokedRect( mBounds );
	}
}
