#pragma once

#include "cinder/Color.h"
#include "cinder/ImageIo.h"
#include "cinder/Surface.h"

using namespace ci;

#include <vector>
#include <array>
#include <utility>
#include <future>
#include <tuple>

namespace col {

    typedef std::pair<std::vector<float>,std::vector<float>> log_chrom_t;
    typedef std::tuple<std::vector<ci::Color8u>, log_chrom_t, std::pair<float,float>, std::pair<float,float> > result_pair_t;
    typedef std::future<result_pair_t>  AsyncPalette;

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

    
        
		PaletteGenerator( ci::Surface8u surface, uint32_t bins = 256);

		ci::Color8u					randomSample() const;
		std::vector<ci::Color8u>	randomPalette( size_t num, float luminanceThreshold = 0.0f ) const;
		std::vector<ci::Color8u>	medianCutPalette( size_t num, bool randomize = false, float luminanceThreshold = -1 ) const;
        std::vector<ci::Color8u> colorList (int);
        const log_chrom_t logChrom () { return mLogChrom; }
        const std::pair<float,float>& MinMaxX () { return mmx; }
        const std::pair<float,float>& MinMaxY () { return mmy; }
        
	private:
        std::vector<glm::ivec3> mColorsVecs;
        log_chrom_t mLogChrom;
        std::pair<float,float> mmx;
        std::pair<float,float> mmy;

	};
	
  }
