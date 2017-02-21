#pragma once

#include "cinder/Color.h"
#include "cinder/ImageIo.h"
#include "cinder/Surface.h"

#include <vector>
#include <array>
#include <future>

namespace col {

	enum ColorChannel {
		RED = 0,
		GREEN = 1,
		BLUE = 2
	};
	
	// Testing out for performance!
	class RgbCube {
	public:
		RgbCube( const std::vector<glm::ivec3>& colors );
		
		//! Get the longest cube channel/axis
		ColorChannel getLongestAxis() const;
		
		//! Get the channel cube dimension
		int getSize( ColorChannel channel ) const;
		
		//! Get the channel cube bounds
		std::pair<uint8_t, uint8_t> getBounds( ColorChannel channel ) const;
		
		//! Calculate the mean color (centroid) of the cube
		ci::Color8u calcMeanColor() const;
		
		//! WARNING! This method is non-const: it sorts the color std::vector
		std::pair<RgbCube, RgbCube> splitAtMedian();
	private:
		//! pair of min-max for each color channel (R-G-B in order)
		std::array< std::pair<uint8_t, uint8_t>, 3 > mBounds;
		
		//! Collection of all colors referenced by cube
		std::vector<glm::ivec3> mColorVecs;
	};

	class PaletteGenerator  {
	public:
		PaletteGenerator( ci::Surface8u surface );

		ci::Color8u					randomSample() const;
		std::vector<ci::Color8u>	randomPalette( size_t num, float luminanceThreshold = 0.0f ) const;
		std::vector<ci::Color8u>	medianCutPalette( size_t num, bool randomize = false, float luminanceThreshold = -1 ) const;
		std::vector<ci::Color8u>	kmeansPalette( size_t num ) const;
	private:
		std::vector<glm::ivec3> mColorsVecs;
	};
	
	typedef std::future<std::vector<ci::Color8u>> AsyncPalette;
}
