
#ifndef __BASIC_UT__
#define __BASIC_UT__


#include "vf_utils.hpp"
#include "timestamp.h"

#define UT_SOURCELINE() (__FILE__, __LINE__)

#define rcUNITTEST_ASSERT(condition)  \
( rcUnitTest::Assert( (condition), \
(#condition),       \
__FILE__,           \
__LINE__ ) )

#define rcUTCheck(condition)  \
( rcUnitTest::Assert( (condition), \
(#condition),       \
__FILE__,           \
__LINE__ ) )

#define rcUTCheckEqual(A, B)				\
( rcUnitTest::Assert(((A) == (B)),			\
(#A),       \
__FILE__,           \
__LINE__ ) )

#define rcUTCheckRealEq(A, B)				\
( rcUnitTest::Assert( (real_equal((A), (B))),		\
(#A),       \
__FILE__,           \
__LINE__ ) )

#define rcUTCheckRealDelta(A, B, C)				\
( rcUnitTest::Assert( (real_equal((A), (B), (C))),		\
(#A),       \
__FILE__,           \
__LINE__ ) )

// Unit test base class

class rcUnitTest {
public:
    rcUnitTest() : mErrors( 0 ) { };
    // Virtual dtor is required
    virtual ~rcUnitTest() { };
    
    // Run tests
    virtual uint32_t run() = 0;
    
    // Print success/failure message
    void printSuccessMessage( const char* msg, uint32_t errors ) {}
#if 0
    {
        if ( errors > 0 ) {
            fprintf( stderr, "%-38s Failed, %d errors\n", msg, errors );
        }
        else {
            fprintf( stderr, "%-38s OK\n", msg );
        }
    }
#endif
    // Assert which accumulates the error count
    uint32_t Assert( uint32_t expr, const char* exprString, const char* file, int lineNumber ) {
        if ( ! expr ) {
            ++mErrors;
            fprintf( stderr, "Assertion %s failed in %s at line %i\n",
                    exprString, file, lineNumber );
        }
        
        return expr;
    }
    
    void randomSeed ()
    {
        // Set a seed from two clocks
        time_spec_t now = time_spec_t::get_system_time ();
        uint32_t seed = now.get_frac_secs ();
        srandom (seed);
    }
    
    // Construct a temporary file name
    static std::string makeTmpName(const char *pathFormat, const char* baseName )
    {
        char buf[2048];
        static const char *defaultFormat = "/tmp/um%d_%s";
        
        // Use current time in seconds as a pseudo-unique prefix
        double secs = time_spec_t::get_system_time ().get_full_secs ();
        snprintf( buf, sizeof(buf)/sizeof(char), pathFormat ? pathFormat : defaultFormat,
                 uint32_t(secs), baseName );
        return std::string( buf );
    }
    
    // Construct a temporary file name
    static std::string makeTmpName(const char* baseName )
    {
        return makeTmpName (0, baseName);
    }
    
protected:
    uint32_t   mErrors;
    std::string   mFileName;
};

#define utClassDeclare(x) \
class UT_##x: public rcUnitTest {\
public:\
UT_##x ();\
~UT_##x ();\
virtual uint32_t run();\
private:\
void test ();\
};


#define utClassDefine(x)\
UT_##x ::UT_##x ()\
{}\
uint32_t UT_##x ::run ()\
{\
test();\
return mErrors;\
}\
UT_##x ::~UT_##x ()\
{\
printSuccessMessage( #x, mErrors );\
}

#endif

