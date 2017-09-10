/*
 Copyright (c) 2014, The Cinder Project

 This code is intended to be used with the Cinder C++ library, http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "AudioDrawUtils.h"

#include "cinder/audio/Utilities.h"
#include "cinder/audio/Context.h"
#include "cinder/CinderMath.h"
#include "cinder/Triangulate.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Shader.h"

#include "DwtNode.h"

using namespace std;
using namespace ci;

void drawAudioBuffer( const audio::Buffer &buffer, const Rectf &bounds, bool drawFrame, const ci::ColorA &color )
{
	gl::ScopedGlslProg glslScope( getStockShader( gl::ShaderDef().color() ) );

	gl::color( color );

	const float waveHeight = bounds.getHeight() / (float)buffer.getNumChannels();
	const float xScale = bounds.getWidth() / (float)buffer.getNumFrames();

	float yOffset = bounds.y1;
	for( size_t ch = 0; ch < buffer.getNumChannels(); ch++ ) {
		PolyLine2f waveform;
		const float *channel = buffer.getChannel( ch );
		float x = bounds.x1;
		for( size_t i = 0; i < buffer.getNumFrames(); i++ ) {
			x += xScale;
			float y = ( 1 - ( channel[i] * 0.5f + 0.5f ) ) * waveHeight + yOffset;
			waveform.push_back( vec2( x, y ) );
		}

		if( ! waveform.getPoints().empty() )
			gl::draw( waveform );

		yOffset += waveHeight;
	}

	if( drawFrame ) {
		gl::color( color.r, color.g, color.b, color.a * 0.6f );
		gl::drawStrokedRect( bounds );
	}
}

// ----------------------------------------------------------------------------------------------------
// MARK: - WaveformPlot
// ----------------------------------------------------------------------------------------------------

namespace {

inline void calcMinMaxForSection( const float *buffer, size_t samplesPerSection, float &max, float &min ) {
	max = 0;
	min = 0;
	for( size_t k = 0; k < samplesPerSection; k++ ) {
		float s = buffer[k];
		max = math<float>::max( max, s );
		min = math<float>::min( min, s );
	}
}

inline void calcAverageForSection( const float *buffer, size_t samplesPerSection, float &upper, float &lower ) {
	upper = 0;
	lower = 0;
	for( size_t k = 0; k < samplesPerSection; k++ ) {
		float s = buffer[k];
		if( s > 0 ) {
			upper += s;
		} else {
			lower += s;
		}
	}
	upper /= samplesPerSection;
	lower /= samplesPerSection;
}

} // anonymouse namespace

void Waveform::load( const float *samples, size_t numSamples, const ci::ivec2 &waveSize, size_t pixelsPerVertex, CalcMode mode )
{
    float height = waveSize.y / 2.0f;
    size_t numSections = waveSize.x / pixelsPerVertex + 1;
    size_t samplesPerSection = numSamples / numSections;

	vector<vec2> &points = mOutline.getPoints();
	points.resize( numSections * 2 );

    for( size_t i = 0; i < numSections; i++ ) {
		float x = (float)i * pixelsPerVertex;
		float yUpper, yLower;
		if( mode == CalcMode::MIN_MAX ) {
			calcMinMaxForSection( &samples[i * samplesPerSection], samplesPerSection, yUpper, yLower );
		} else {
			calcAverageForSection( &samples[i * samplesPerSection], samplesPerSection, yUpper, yLower );
		}
		points[i] = vec2( x, height - height * yUpper );
		points[numSections * 2 - i - 1] = vec2( x, height - height * yLower );
    }
	mOutline.setClosed();

	mMesh = gl::VboMesh::create( Triangulator( mOutline ).calcMesh() );
}


void WaveformPlot::load( const std::vector<float> &samples, const ci::Rectf &bounds, size_t pixelsPerVertex )
{
	mBounds = bounds;
	mWaveforms.clear();

	ivec2 waveSize = bounds.getSize();
	mWaveforms.push_back( Waveform( samples, waveSize, pixelsPerVertex, Waveform::CalcMode::MIN_MAX ) );
	mWaveforms.push_back( Waveform( samples, waveSize, pixelsPerVertex, Waveform::CalcMode::AVERAGE ) );
}

void WaveformPlot::load( const audio::Buffer &buffer, const ci::Rectf &bounds, size_t pixelsPerVertex )
{
	mBounds = bounds;
	mWaveforms.clear();

	size_t numChannels = buffer.getNumChannels();
	ivec2 waveSize = bounds.getSize();
	waveSize.y /= numChannels;
	for( size_t ch = 0; ch < numChannels; ch++ ) {
		mWaveforms.push_back( Waveform( buffer.getChannel( ch ), buffer.getNumFrames(), waveSize, pixelsPerVertex, Waveform::CalcMode::MIN_MAX ) );
		mWaveforms.push_back( Waveform( buffer.getChannel( ch ), buffer.getNumFrames(), waveSize, pixelsPerVertex, Waveform::CalcMode::AVERAGE ) );
	}
}

void WaveformPlot::draw()
{
	auto &waveforms = getWaveforms();
	if( waveforms.empty() ) {
		return;
	}

	gl::ScopedGlslProg glslScope( getStockShader( gl::ShaderDef().color() ) );

	gl::color( mColorMinMax );
	gl::draw( waveforms[0].getMesh() );

	gl::color( mColorAverage );
	gl::draw( waveforms[1].getMesh() );

	if( waveforms.size() > 2 ) {
		gl::pushMatrices();
		gl::translate( 0, getBounds().getHeight() / 2 );

		gl::color( mColorMinMax );
		gl::draw( waveforms[2].getMesh() );

		gl::color( mColorAverage );
		gl::draw( waveforms[3].getMesh() );
		
		gl::popMatrices();
	}
}

// ----------------------------------------------------------------------------------------------------
// MARK: - SpectrumPlot
// ----------------------------------------------------------------------------------------------------

SpectrumPlot::SpectrumPlot()
    : mScaleDecibels(true), mBorderEnabled(true), mBorderColor(0.5f, 0.5f, 0.5f, 1), mDrawLabels(true)
{
}

void SpectrumPlot::draw( const vector<float> &magSpectrum )
{
	if( magSpectrum.empty() )
		return;

	gl::ScopedGlslProg glslScope( getStockShader( gl::ShaderDef().color() ) );

	ColorA bottomColor( 0, 0, 0.7f, 1 );

	float width = mBounds.getWidth();
	float height = mBounds.getHeight();
	size_t numBins = magSpectrum.size();
	float padding = 0;
	float binWidth = ( width - padding * ( numBins - 1 ) ) / (float)numBins;

	gl::VertBatch batch( GL_TRIANGLE_STRIP );

	size_t currVertex = 0;
	float m;
	Rectf bin( mBounds.x1, mBounds.y1, mBounds.x1 + binWidth, mBounds.y2 );
	for( size_t i = 0; i < numBins; i++ ) {
		m = magSpectrum[i];
		if( mScaleDecibels )
			m = audio::linearToDecibel( m ) / 100;

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

    if (mDrawLabels && !mTextureFont)
        mTextureFont = gl::TextureFont::create(Font(Font::getDefault().getName(), 16));

    if (mDrawLabels && mTextureFont)
    {
        auto max_freq = audio::Context::master()->deviceManager()->getDefaultInput()->getSampleRate() / 2;
        std::string fftLabel = "FFT bins 0 - " + std::to_string(max_freq) + "Hz. (intensity vs. frequency)";
        mTextureFont->drawString(fftLabel, vec2((mBounds.getLowerLeft().x + mBounds.getWidth()  *0.5f) - mTextureFont->measureString(fftLabel).x / 2, mBounds.getLowerLeft().y + mTextureFont->getFont().getSize() * 1.2f));
    }
}

// ----------------------------------------------------------------------------------------------------
// MARK: - WaveletDecompositionPlot
// ----------------------------------------------------------------------------------------------------

WaveletDecompositionPlot::WaveletDecompositionPlot(wavy::DwtNodeRef node)
    : mNode(node), mBorderColor(0.5f, 0.5f, 0.5f, 1), mDrawLabels(true), mScaleDecibels(true)
{}

void WaveletDecompositionPlot::draw()
{
    if (!mNode)
        return;

    if (mSurfaces.size() != mNode->getFormat().getDecompositionLevels())
        createSurfaces();

    updateSurfaces();
    renderSurfaces();
}

void WaveletDecompositionPlot::createSurfaces()
{
    if (!mSurfaces.empty())
        mSurfaces.clear();
    
    if (!mTextures.empty())
        mTextures.clear();

    gl::Texture::Format tex_fmt;
    tex_fmt.magFilter(GL_NEAREST);

    for (std::size_t index = 0; index < mNode->getFormat().getDecompositionLevels(); ++index)
    {
        mSurfaces.push_back(ci::Surface32f(mNode->getCoefficients(index + 1).size(), 1, false));
        mTextures.push_back(gl::Texture::create(mSurfaces.back(), tex_fmt));
    }
}

void WaveletDecompositionPlot::updateSurfaces()
{
    for (std::size_t index = 0; index < mSurfaces.size(); ++index)
    {
        for (int coeff = 0; coeff < mSurfaces[index].getWidth(); ++coeff)
        {
            float m = ci::math<float>::clamp(mNode->getCoefficients(index + 1)[coeff]);

            if (getScaleDecibels())
                m = audio::linearToDecibel(m) / 100.0f;

            mSurfaces[index].setPixel(ivec2(coeff, 0), ci::Color(m, m, m));
        }

        mTextures[index]->update(mSurfaces[index]);
    }
}

void WaveletDecompositionPlot::renderSurfaces()
{
    auto draw_height = mBounds.getHeight() / mTextures.size();
    auto draw_uppery = mBounds.getUpperLeft().y;

    if (mDrawLabels && !mTextureFont)
        mTextureFont = gl::TextureFont::create(Font(Font::getDefault().getName(), 16));

    for (std::size_t index = 0; index < mTextures.size(); ++index)
    {
        if (mTextures[index])
        {
            auto bounds = ci::Rectf(mBounds.getUpperLeft().x, draw_uppery, mBounds.getUpperRight().x, draw_uppery + draw_height);

            if (mPaletteShader)
            {
                gl::ScopedGlslProg      bind(mPaletteShader);
                gl::ScopedTextureBind   texb(mTextures[index]);
                mPaletteShader->uniform("uTex0", 0);
                gl::drawSolidRect(bounds);
            }
            else
            {
                gl::draw(mTextures[index], bounds);
            }
        }

        if (mDrawLabels && mTextureFont)
        {
            std::string levelLabel = std::to_string(index + 1);
            mTextureFont->drawString(levelLabel, vec2(mBounds.getUpperLeft().x - mTextureFont->getFont().getSize() * 1.2f, draw_uppery + draw_height * 0.5f));
        }

        draw_uppery += draw_height;
    }

    gl::color(mBorderColor);
    gl::drawStrokedRect(mBounds);

    if (mDrawLabels && mTextureFont)
    {
        std::stringstream ss;
        std::size_t precision = 6;

        ss
            << WavelettoString(mNode->getFormat().getMotherWavelet())
            << " wavelet decompositions (level vs. time). Mean: "
            << std::fixed << std::setprecision(precision) << mNode->getMean()
            << " StdDev: "
            << std::fixed << std::setprecision(precision) << mNode->getStdDeviation();

        mTextureFont->drawString(ss.str(), vec2((mBounds.getLowerLeft().x + mBounds.getWidth()  *0.5f) - mTextureFont->measureString(ss.str()).x / 2, mBounds.getLowerLeft().y + mTextureFont->getFont().getSize() * 1.2f));
    }
}
