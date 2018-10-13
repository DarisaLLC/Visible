// Copyright 2002 Reify, Inc.


#include "roi_window.h"
#include "frame.h"
#include <cached_frame_buffer.h>


#include "basic_ut.hpp"


class UT_raw_frame : public rcUnitTest {
public:
    
    UT_raw_frame();
    ~UT_raw_frame();
    
    virtual uint32 run();
    
private:
    void testraw_framefer( int32 width, int32 height,rpixel d );
    void testRowPointers( int32 width, int32 height, rpixel d );
    void testPixel( int32 x, int32 y, rcFrame& buf );
};

UT_raw_frame::UT_raw_frame()
{
}

UT_raw_frame::~UT_raw_frame()
{
        printSuccessMessage( "rcFrame test", mErrors );
}

uint32
UT_raw_frame::run()
{
    {
        rcUNITTEST_ASSERT(get_bytes().count(rpixel8) == 1);
        rcUNITTEST_ASSERT(get_bytes().count(rpixel16) == 2);
        rcUNITTEST_ASSERT(get_bytes().count(rpixelFloat) == 4); 

            
        
        rcUTCheck(rc_pixel().validate((int) rpixel8));
        rcUTCheck(rc_pixel().validate((int) rpixel16));
        rcUTCheck(rc_pixel().validate((int) rpixel32S));  
        
        rcUTCheck(rc_pixel().validate(get_bytes().count(rpixel8)));
        rcUTCheck(rc_pixel().validate(get_bytes().count(rpixel16))); 
        rcUTCheck(rc_pixel().validate(get_bytes().count(rpixel32S)));       

    }
    
  
    
     // Dummy object, everything should be undefined
    {        
        rcFrame dummyBuf;

      //  rcUNITTEST_ASSERT( dummyBuf.refCount() == 0 );
     //   rcUNITTEST_ASSERT( dummyBuf.isBound()  == false);
    }

   
    {
        // Test Row Pointers
            testRowPointers( 1, 1, rpixel8);
            testRowPointers( 3, 3, rpixel8 );
            testRowPointers( 9, 10, rpixel8 );  
            testRowPointers( 16, 16, rpixel8 );  

        testRowPointers( 1, 1, rpixel16);
        testRowPointers( 3, 3, rpixel16 );
        testRowPointers( 9, 10, rpixel16 );  
        testRowPointers( 16, 16, rpixel16 );  
      
        testRowPointers( 1, 1, rpixel32S);
        testRowPointers( 3, 3, rpixel32S );
        testRowPointers( 9, 10, rpixel32S );  
        testRowPointers( 16, 16, rpixel32S );  
        
    }
    
    // Small buffers
    {
        // Test all depths
        testraw_framefer( 1, 1, rpixel8);
        testraw_framefer( 3, 3, rpixel8 );
        testraw_framefer( 9, 10, rpixel8 );  
        testraw_framefer( 16, 16, rpixel8 );  
        
        testraw_framefer( 1, 1, rpixel16);
        testraw_framefer( 3, 3, rpixel16 );
        testraw_framefer( 9, 10, rpixel16 );  
        testraw_framefer( 16, 16, rpixel16 );  
        
        testraw_framefer( 1, 1, rpixel32S);
        testraw_framefer( 3, 3, rpixel32S );
        testraw_framefer( 9, 10, rpixel32S );  
        testraw_framefer( 16, 16, rpixel32S );  
        
    }
   
    // Large buffers
    {        
        // Test all depths
        testraw_framefer( 1600, 1200, rpixel8 );
        testraw_framefer( 1152, 864, rpixel16 );        
        testraw_framefer( 640, 480, rpixel32S );        

    }

    // rcFrameRef basic tests   
    {
        // Constructor rcFrameRef( rcFrame* p )
        rcFrameRef buf( new rcFrame( 640, 480, rpixel8 ) );

        rcUNITTEST_ASSERT( buf->refCount() == 1 );

        // Constructor rcFrameRef( const rcFrameRef& p )
        rcFrameRef share1( buf );

        rcUNITTEST_ASSERT( share1.refCount() == 2 );
        {
            rcFrameRef share2( buf );
            rcUNITTEST_ASSERT( share2.refCount() == 3 );
        }
        rcUNITTEST_ASSERT( share1.refCount() == 2 );
    }

    // rcFrameRef operators
    {
        rcFrame* buf = new rcFrame( 640, 480, rpixel8 ); 
        rcFrameRef p3 = buf;
        rcUNITTEST_ASSERT( p3.refCount() == 1 );
        rcFrameRef p4 = buf;
        rcUNITTEST_ASSERT( p3.refCount() == 2 );

        // Comparison operators
        rcUNITTEST_ASSERT( p3 == p4 );
        rcUNITTEST_ASSERT( !(p3 != p4) );
        // Use const_cast to silence compiler warnings
        rcUNITTEST_ASSERT( p3 == const_cast<const rcFrame*>(buf) );
        rcUNITTEST_ASSERT( !(p3 != const_cast<const rcFrame*>(buf)) );
        rcUNITTEST_ASSERT( !(!p3) );

        // Assignment operators
        {
            rcFrameRef p  = new rcFrame( 640, 480, rpixel8 ); // sample #1
            rcFrameRef p2 = new rcFrame( 640, 480, rpixel8 ); // sample #2
            p = p2; 
            rcUNITTEST_ASSERT( p.refCount() == 2 );
        }

    }

    return mErrors;
}



void UT_raw_frame::testPixel( int32 x, int32 y, rcFrame& buf )
{
	uint32 pixelValue = 0;
    rcTimestamp now = rcTimestamp::now();

    // Timestamp mutator
    buf.setTimestamp( now );
    rcUNITTEST_ASSERT( buf.timestamp() == now );
    
	if ( buf.depth() >= rpixel8 && buf.depth() <= rpixel32S) {
		// Verify that accessors and mutators work
		buf.setPixel( x, y, pixelValue );
		rcUNITTEST_ASSERT( buf.getPixel( x, y ) == pixelValue );
       
		pixelValue = 7;
		buf.setPixel( x, y, pixelValue );
		rcUNITTEST_ASSERT( buf.getPixel( x, y ) == pixelValue );
        if ( buf.getPixel( x, y ) != pixelValue )
            cerr << buf.width() << "x" << buf.height() << "x" << buf.depth()*8
                 << " getPixel(" << x << "," << y << ") returned " <<  buf.getPixel( x, y )
                 << ", expected " << pixelValue << endl;
   
		pixelValue = rcUINT8_MAX;
		buf.setPixel( x, y, pixelValue );
		rcUNITTEST_ASSERT( buf.getPixel( x, y ) == pixelValue );
        if ( buf.getPixel( x, y ) != pixelValue )
            cerr << buf.width() << "x" << buf.height() << "x" << buf.depth()*8
                 << " getPixel(" << x << "," << y << ") returned " <<  buf.getPixel( x, y )
                 << ", expected " << pixelValue << endl;
   		// Verify that value is returned
		rcUNITTEST_ASSERT( buf.setPixel( x, y, pixelValue-1 ) == (pixelValue - 1) );
	}

	if ( buf.depth() >= rpixel16 && buf.depth() <= rpixel32S) {
		// Verify that accessors and mutators work
		pixelValue = rcUINT8_MAX + 7;
		buf.setPixel( x, y, pixelValue );
		rcUNITTEST_ASSERT( buf.getPixel( x, y ) == pixelValue );
         if ( buf.getPixel( x, y ) != pixelValue )
            cerr << buf.width() << "x" << buf.height() << "x" << buf.depth()*8
                 << " getPixel(" << x << "," << y << ") returned " <<  buf.getPixel( x, y )
                 << ", expected " << pixelValue << endl;         
		pixelValue = rcUINT16_MAX;
		buf.setPixel( x, y, pixelValue );
		rcUNITTEST_ASSERT( buf.getPixel( x, y ) == pixelValue );
        if ( buf.getPixel( x, y ) != pixelValue )
            cerr << buf.width() << "x" << buf.height() << "x" << buf.depth()*8
                 << " getPixel(" << x << "," << y << ") returned " <<  buf.getPixel( x, y )
                 << ", expected " << pixelValue << endl;
 
		// Verify that value is returned
		rcUNITTEST_ASSERT( buf.setPixel( x, y, pixelValue-1 ) == pixelValue - 1);
	}

	if ( buf.depth() >= rpixel32S && buf.depth() <= rpixel32S) {
		// Verify that accessors and mutators work
		pixelValue = rcUINT24_MAX + 7;
		buf.setPixel( x, y, pixelValue );
		rcUNITTEST_ASSERT( buf.getPixel( x, y ) == pixelValue );
		pixelValue = rcUINT32_MAX;
		buf.setPixel( x, y, pixelValue );
		rcUNITTEST_ASSERT( buf.getPixel( x, y ) == pixelValue );

		// Verify that value is returned
		rcUNITTEST_ASSERT( buf.setPixel( x, y, pixelValue-1 ) == pixelValue - 1);
	}
}

void UT_raw_frame::testraw_framefer( int32 width, int32 height, rpixel depth )
{
    rcFrame* buf = new rcFrame( width, height, depth );

	// Accessor tests
    rcUNITTEST_ASSERT( buf->refCount() == 0 );
    rcUNITTEST_ASSERT( buf->height() == height );
    rcUNITTEST_ASSERT( buf->width() == width );
    rcUNITTEST_ASSERT( buf->depth() == depth );
    rcUNITTEST_ASSERT( buf->rawData() != 0 );
    rcUNITTEST_ASSERT( buf->timestamp() == rcTimestamp() );

  
    int32 alignment = (buf->width() * buf->bytes()) % rcFrame::ROW_ALIGNMENT_MODULUS;
    if ( alignment )
        alignment = rcFrame::ROW_ALIGNMENT_MODULUS - alignment;
    rcUNITTEST_ASSERT( alignment == buf->rowPad() );
    if ( alignment != buf->rowPad() )
        cerr << buf->width() << "x" << buf->height() << "x" << buf->bytes()*8
             << " rowPad " << buf->rowPad() << ", expected " << alignment << endl;

    
	int32 row, column;

	// Pixel mutator tests
	for ( row = 0; row < buf->height(); row++ ) {
		for ( column = 0; column < buf->width(); column++ ) {
			testPixel( column, row, *buf );
		}
	}
    delete buf;
}


void UT_raw_frame::testRowPointers( int32 width, int32 height, rpixel depth )
{
    rcFrameRef buf (new rcFrame( width, height, depth ) );

   // Accessor tests
   rcUNITTEST_ASSERT( buf->refCount() == 1 );
   rcUNITTEST_ASSERT( buf->height() == height );
   rcUNITTEST_ASSERT( buf->width() == width );
   rcUNITTEST_ASSERT( buf->depth() == depth );
   rcUNITTEST_ASSERT( buf->rawData() != 0 );


   // RowPointer Tests
   for ( int32 row = 0; row < buf->height(); row++ )
      {
         rcUNITTEST_ASSERT (!((uint64) (buf->rowPointer (row)) % buf->alignment ()));
         if (!row) continue;
         rcUNITTEST_ASSERT ((uint32)(buf->rowPointer (row) - buf->rowPointer (row - 1)) ==
                             buf->rowUpdate ());
      }

}
