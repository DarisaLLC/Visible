
#ifndef __VF_CINDER_AUDIO2_
#define __VF_CINDER_AUDIO2_


#include "vf_utils.hpp"
#include "cinder/Filesystem.h"
#include "cinder/audio/Buffer.h"
#include "cinder/audio/Exception.h"

#include <vector>
#include <tuple>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <algorithm>

using namespace cinder;
using namespace cinder::audio;
using namespace std;
using namespace boost;
namespace fs = boost::filesystem;


    // From a Visible results file ( or a csv file with one or more columns of float values ) via a Buffer, a DataSourceBufferRef is
    // cretaed. A VisibleAudioSource is created from a DataSourceBufferRef. 
    
typedef std::shared_ptr<class VisibleAudioSource> VisibleAudioSourceRef;

    class VisibleAudioSource
    {
        
    public:
        
        VisibleAudioSource (const boost::filesystem::path &aFilePath)
        {
            const std::string& fqfn = aFilePath.string ();
            mdat.resize (0);
            bool c2v_ok = csv::csv2vectors ( fqfn, mdat, false, true, true);
            if ( ! c2v_ok )
                throw AudioFileExc( string( "Failed to validate file with error: " ), -1 );
            
            size_t columns = mdat.size ();
            size_t rows = mdat[0].size();
            // only handle case of long columns of data
            if (columns >= rows)
                throw AudioFileExc( string( "Failed to validate file with error: " ), -2 );
            
            mNumFrames  = rows;
            mReadPos = 0;
            mNumChannels = columns;
        }

        bool isFilePath () { return true; }
        bool isUrl () { return false; }
        
        virtual ~VisibleAudioSource()	{}
        
        size_t	read( audio::Buffer *buffer )  
        {
            CI_ASSERT( buffer->getNumChannels() == mNumChannels );
            CI_ASSERT( mReadPos < mNumFrames );
            
            size_t numRead;
            size_t numFramesNeeded = std::min( mNumFrames - mReadPos, std::min( mMaxFramesPerRead, buffer->getNumFrames() ) );
            numRead = performRead( buffer, 0, numFramesNeeded );
            
            mReadPos += numRead;
            return numRead;
        }

   
        
        //! Loads and returns the entire contents of this VisibleAudioSource. \return a BufferRef containing the file contents.
        cinder::audio::BufferRef getBuffer(int selected_column = 0)
        {
            cinder::audio::BufferRef result = make_shared<BufferT<float> >( mNumFrames, 1);
            const std::vector<float>& data = mdat[selected_column];
            std::memmove( result->getChannel( 0 ), & data[0], mNumFrames  * sizeof(float) );
            return result;
        }

        size_t	getNumFrames() const					{ return mNumFrames; }
        size_t	getNumChannels() const					{ return mNumChannels; }
        
    protected:

        VisibleAudioSource ();
        size_t performRead( audio::Buffer *buffer, size_t bufferFrameOffset, size_t numFramesNeeded, int selected_column = 0)
        {
            CI_ASSERT( buffer->getNumFrames() >= bufferFrameOffset + numFramesNeeded );
            const std::vector<float>& data = mdat[selected_column];
            size_t offset = bufferFrameOffset;
            memcpy( buffer->getChannel( 0 ) + offset, &data[0], numFramesNeeded * sizeof( float ) );
            mReadPos += numFramesNeeded;
            return static_cast<size_t>( numFramesNeeded );
        }
        
        std::vector<vector<float> > mdat;
        size_t mNumFrames;
        size_t mNumChannels;
        size_t mReadPos;
        size_t mMaxFramesPerRead = 4096;
        
        
    };
    
    //! Convenience method for loading a VisibleAudioSource from \a dataSource. \return VisibleAudioSourceRef. \see VisibleAudioSource::create()
    //    inline VisibleAudioSourceRef	load( const DataSourceRef &dataSource )	{ return VisibleAudioSource::create( dataSource ); }
    



#endif
