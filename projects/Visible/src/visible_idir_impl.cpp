#include <stdio.h>

#include "VisibleApp.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Timeline.h"
#include "cinder/Timer.h"
#include "cinder/Camera.h"
#include "cinder/qtime/Quicktime.h"
#include "cinder/params/Params.h"
#include "cinder/ImageIo.h"
#include "ui_contexts.h"
#include "stl_util.hpp"
#include <stdlib.h>

using namespace ci;
using namespace ci::app;
using namespace std;

static boost::filesystem::path browseToFolder ()
{
    return Platform::get()->getFolderPath (Platform::get()->getHomeDirectory());
}

void imageDirContext::loadImageDirectory (const filesystem::path& directory)

{
    m_valid = false;
    
    // clear the list
    mImageFiles.clear();
    
    if( directory.empty() || !filesystem::is_directory( directory ) )
        return;
    
    // make a list of all audio files in the directory
    filesystem::directory_iterator end_itr;
    for( filesystem::directory_iterator i( directory ); i != end_itr; ++i )
    {
        // skip if not a file
        if( !filesystem::is_regular_file( i->status() ) ) continue;
        
        // skip if extension does not match
        string extension = i->path().extension().string();
        extension.erase( 0, 1 );
        if( std::find( mSupportedExtensions.begin(), mSupportedExtensions.end(), extension ) == mSupportedExtensions.end() )
            continue;
        
        // file matches
        mImageFiles.push_back( i->path() );
    }
    
    std::sort (mImageFiles.begin(), mImageFiles.end(), stl_utils::naturalPathComparator());
    m_valid = mImageFiles.size() > 0;
    
 }

imageDirContext::imageDirContext( WindowRef& ww, const boost::filesystem::path& dp)
        : uContext (ww), mFolderPath (dp)
{
    m_valid = false;
    m_type = Type::image_dir_viewer;
    
    // make a list of valid audio file extensions and initialize audio variables
    const char* extensions[] = { "jpg", "png", "JPG" };
    mSupportedExtensions = vector<string>( extensions, extensions + 3 );

    setup ();
    if (is_valid())
    {
        mWindow->setTitle( mFolderPath.filename().string() );
    }
}

bool imageDirContext::is_valid ()
{
    return m_valid && is_context_type(uContext::image_dir_viewer);
}

void imageDirContext::setup()
{
     // Browse for the image folder
    if (mFolderPath == boost::filesystem::path ())
    {
        mFolderPath = browseToFolder ();
    }
    
    loadImageDirectory(mFolderPath);

    if ( mImageFiles.empty () ) return;

    mTotalItems = mImageFiles.size();
 
    float xPos = 0;

    // @to_improve
    // Load textures at full size to get the size information ( could have used sips cli in OS X )
    std::map<std::pair<int,int>,int> size_hist;
    
    std::vector<filesystem::path>::const_iterator pItr = mImageFiles.begin();
    for (; pItr < mImageFiles.end(); pItr++)
    {
        mSurfaces.push_back(Surface8u::create( loadImage( pItr->string())) );

        
        const std::pair<int,int> sz (mTextures.back()->getSize().x, mTextures.back()->getSize().y);
        int check = stl_utils::safe_get (size_hist, sz, int(-1));
        if (check < 0)
            size_hist[sz] = 1; // new entry
        else
        {
            size_hist[sz] = check+1;
        }
    }

    // find the largest image. Again: expectation is that they are equal
    std::vector<int> areas;
    std::vector<std::pair<int,int> > sizes = stl_utils::keys_as_vector(size_hist);
    for (std::pair<int,int>& hh : sizes)areas.push_back(hh.first * hh.second);
    auto maxi = std::distance (areas.begin(), std::max_element(areas.begin(), areas.end()));
    mLarge = sizes[maxi];

    App::get()->setWindowSize(mLarge.first/2, mLarge.second/2);
    
    mItemExpandedWidth = mLarge.first/4;
    mItemHeight = mLarge.second;
    mItemRelaxedWidth = mLarge.first/2 / mTotalItems;


    
    pItr = mImageFiles.begin();
    std::vector<gl::TextureRef>::const_iterator tItr = mTextures.begin();
    
    for (; pItr < mImageFiles.end(); pItr++, tItr++)
    {
        mItems.push_back( AccordionItem( timeline(),
                                        xPos,
                                        0,
                                        mItemHeight,
                                        mItemRelaxedWidth,
                                        mItemExpandedWidth,
                                        *tItr,
                                        pItr->parent_path().string(),
                                        pItr->filename().string()));
        xPos += mItemRelaxedWidth;
    }

    
    // similar to mCurrentSelection = null;
    mCurrentSelection = mItems.end();
}


void imageDirContext::mouseMove( MouseEvent event )
{
    list<AccordionItem>::iterator mNewSelection = mItems.end();
    
    for( list<AccordionItem>::iterator itemIt = mItems.begin(); itemIt != mItems.end(); ++itemIt ) {
        if( itemIt->isPointIn( event.getPos() ) ) {
            mNewSelection = itemIt;
            break;
        }
    }
    
    if( mNewSelection != mCurrentSelection) {
        float xPos = 0;
        float contractedWidth = (mTotalItems*mItemRelaxedWidth - mItemExpandedWidth)/float(mTotalItems - 1);
        mCurrentSelection = mNewSelection;
        
        if (mCurrentSelection == mItems.end()) {
            for( list<AccordionItem>::iterator itemIt = mItems.begin(); itemIt != mItems.end(); ++itemIt ) {
                itemIt->animTo(xPos, mItemRelaxedWidth);
                xPos += mItemRelaxedWidth;
            }
        } else {
            for( list<AccordionItem>::iterator itemIt = mItems.begin(); itemIt != mItems.end(); ++itemIt ) {
                if( itemIt == mCurrentSelection ) {
                    itemIt->animTo(xPos, mItemExpandedWidth, true);
                    xPos += mItemExpandedWidth;
                } else {
                    itemIt->animTo(xPos, contractedWidth);
                    xPos += contractedWidth;
                }
            }
        }
    }
}

void imageDirContext::resize ()
{
//    mItemExpandedWidth = getWindowWidth();
//    float xyscale = mItemExpandedWidth / ((float)mLarge.first);
//    mItemHeight = xyscale * mLarge.second;
//    mItemRelaxedWidth = (getWindowWidth() * 1.333 ) / mTotalItems;
//    
//    
//    std::vector<filesystem::path>::const_iterator pItr = mImageFiles.begin();
//    std::vector<gl::TextureRef>::const_iterator tItr = mTextures.begin();
//    mItems.clear ();
//       float xPos = 0;
//    
//    for (; pItr < mImageFiles.end(); pItr++, tItr++)
//    {
//        mItems.push_back( AccordionItem( timeline(),
//                                        xPos,
//                                        0,
//                                        mItemHeight,
//                                        mItemRelaxedWidth,
//                                        mItemExpandedWidth,
//                                        *tItr,
//                                        pItr->parent_path().string(),
//                                        pItr->filename().string()));
//        xPos += mItemRelaxedWidth;
//    }
//    
    

}
void imageDirContext::update()
{
    for( list<AccordionItem>::iterator itemIt = mItems.begin(); itemIt != mItems.end(); ++itemIt ) {
        itemIt->update();
    }
}

void imageDirContext::draw()
{	
    gl::clear( Color( 1, 1, 1 ) );
    gl::enableAlphaBlending();
    
    for( list<AccordionItem>::iterator itemIt = mItems.begin(); itemIt != mItems.end(); ++itemIt ) {
        itemIt->draw();
    }
}
