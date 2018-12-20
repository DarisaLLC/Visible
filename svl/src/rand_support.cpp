
#include "core/rand_support.h"

#include <mach/mach.h>
#include <mach/mach_time.h>

namespace svl {
	
std::mt19937 Rand::sBase( 310u );
std::uniform_real_distribution<float> Rand::sFloatGen;

void Rand::randomize()
{
#if defined(OBJC)
	sBase = std::mt19937( (uint32_t)( mach_absolute_time() & 0xFFFFFFFF ) );
#elif defined(CPP)
	sBase = std::mt19937( ::GetTickCount() );
#endif
}

void Rand::randSeed( uint32_t seed )
{
	sBase = std::mt19937( seed );
}

void Rand::seed( uint32_t seedValue )
{
	mBase = std::mt19937( seedValue );
}

} // svl
