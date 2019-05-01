

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"

#include "cinder_cv/cinder_opencv.h"
#include "cinder/app/App.h"
#include "cinder/Clipboard.h"
#include "cinder/app/Platform.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"
#include "cinder/Capture.h"
#include "cinder/params/Params.h"
#include "cinder/Log.h"
#include "cinder/gl/TextureFont.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <time.h>
#include "core/stats.hpp"
#include "vision/ss_segmenter.hpp"
#include "vision/ColorHistogram.hpp"
#include "cinder_cv/affine_rectangle.h"
#include "vision/opencv_utils.hpp"
#include "boost/filesystem.hpp"
#include "vision/histo.h"
#include "vision/ipUtils.h"
#include "vision/bit_chrome.hpp"

#include <string>
#include <vector>
#include "core/singleton.hpp"

using namespace boost;
namespace filesystem = boost::filesystem;
using namespace ci;
using namespace ci::app;
using namespace std;


namespace
{
    bool is_anaonymous_name (const filesystem::path& pp, size_t check_size = 36)
    {
        string extension = pp.extension().string();
        return extension.length() == 0 && pp.filename().string().length() == check_size;
    }
}


class woundprocess   : internal_singleton<woundprocess>
{
    
private:
    mutable int mState;
    std::vector<SurfaceRef> mViews;
    cv::Mat mTemplate;
    std::vector<cv::RotatedRect> mRects;
    
};

struct rank_output
{
    std::pair< uint8_t, int> peak;
    std::pair< uint8_t, int> goal;
    float score;
    float texture_score;
    bool valid;
};

void get_output (std::vector<std::pair <uint8_t, int> >& raw, const uint8_t  goal, const uint8_t peak, rank_output& outp)
{
    outp.valid = false;
    bool got_peak = false;
    bool got_goal = false;
    int non_count = 0;
    outp.goal.first = goal;
    got_goal = true;
    
    for (auto cc : raw)
    {
        auto val = cc.first;
        if (val == goal)
        {
            outp.goal = cc;
            got_goal = true;
        }
        else if (val == peak)
        {
            outp.peak = cc;
            got_peak = true;
        }
        else
            non_count++;
        if (got_peak && got_goal)
        {
            outp.score = 1.0f - ((std::max(peak,goal) - std::min(peak,goal))%128) / 128.0f;
            outp.valid = true;
            
            std::cout << " ======  > " << outp.score << std::endl;
            break;
        }
    }
    
}



class MainApp : public AppMac {
public:
    
    
    void setup() override;
    void update() override;
    void resize() override;
    void mouseMove( MouseEvent event ) override;
    void mouseDown( MouseEvent event ) override;
    void mouseUp( MouseEvent event ) override;
    void keyDown(KeyEvent event) override;
    void mouseDrag( MouseEvent event ) override;
    void saveImage(const cv::Mat & image, const std::string & filename);
    void RGBtoYxy(cv::Mat source, cv::Mat &destination);
    void hueMappedOrientation(cv::Mat source, cv::Mat &destination);
    
    Surface8uRef  loadImageFromPath (const filesystem::path&);
    void fileDrop( FileDropEvent event ) override;
    void draw()override;
    void process ();
    std::string pixelInfo ( const ivec2 & position);
    std::string resultInfo (const rank_output& output);
    void drawEditableRect ();
    void openFile();
    static cv::Mat affineCrop (cv::Mat& src, cv::RotatedRect& rect);
    vec2 matStats (const cv::Mat& image);
    Surface8uRef   mImage;
    Surface8uRef   mModelImage;
    Surface8uRef   mHSLImage;

    cv::Mat mInputMat;
    cv::Mat mSegmented;
    cv::Mat mEdge;
    vector<cv::Mat> mLogs;
    vector<cv::Mat> mHSV;
    vector<cv::Mat> mRGB;
    
    std::vector<gl::TextureRef> mTextures;
    std::vector<gl::TextureRef> mOverlays;
    gl::TextureRef* textureToDisplay;
    gl::TextureRef mOverlay;
    RectMapping mNormalMap;
    RectMapping mInveseMap;
    
    int mOption;
    params::InterfaceGlRef	mParams;
    vector<string> mNames = { "Input",  "Orientation"};
    ivec2			mMousePos;
    vector<vector<cv::Point> > contours;
    vector<Vec4i> hierarchy;
    Scalar mSkinHue[2];
    Font				mFont;
    Font                mScoreFont;
    gl::TextureFontRef	mTextureFont;
    gl::TextureFontRef	mTextureScoreFont;
    
    affineRectangle   mRectangle;
    Rectf          mWorkingRect;
    
    ivec2          mMouseInitial;
    affineRectangle   mRectangleInitial;
    RotatedRect    mAffineRect;
    Point2f          mAffineCenter;
    
    rank_output     m_rank_score;
    
    vec2           mStringSize;
    bool           mIsOver;
    bool           mIsClicked;
    bool           mMouseIsDown;
    
    cv::Mat mModel;
    cv::Mat mConvertedTmp;
    boost::filesystem::path mFolderPath;
    std::string mExtension;
    std::string mToken;
    
    
    std::string get_timestamp_filename ()
    {
#define LOGNAME_FORMAT "bina-model%Y%m%d_%H%M%S"
#define LOGNAME_SIZE 20
        
        static char name[LOGNAME_SIZE];
        time_t now = time(0);
        strftime(name, sizeof(name), LOGNAME_FORMAT, localtime(&now));
        return std::string(name);
    }
    
};


Surface8uRef MainApp::loadImageFromPath (const filesystem::path& path)
{
    Surface8uRef sref;
    
    if (is_anaonymous_name(path))
        sref = Surface8u::create( loadImage( path.string(), ImageSource::Options(), "jpg"));
    else
        sref = Surface8u::create( loadImage( path.string() ) );
    return sref;
    
}

vec2 MainApp::matStats (const cv::Mat& image)
{
    // compute statistics for Hue value
    cv::Scalar mean, stddev;
    cv::meanStdDev(image, mean, stddev);
    return vec2(mean[0], stddev[0]);
}
void MainApp::resize()
{
    setWindowSize(getWindowSize());
    vec2 newsize = mRectangle.scale_map(getWindowSize());
    mRectangle = affineRectangle (Rectf(0,0,newsize[0], newsize[1]));
    mRectangle.position (vec2(newsize[0]/2, newsize[1]/2));
}




void MainApp::openFile()
{
    try {
        mImage = loadImageFromPath(getOpenFilePath());
        if (mImage)
        {
            resize();
            process();
        }
    }
    catch( std::exception &exc ) {
        CI_LOG_EXCEPTION( "failed to load image.", exc );
    }
}

void MainApp::update()
{
    // Check if mouse is inside rectangle, by converting the mouse coordinates to object space.
    vec3 object = gl::windowToObjectCoord( mRectangle.matrix(), mMousePos );
    mIsOver = mRectangle.area().contains( vec2( object ) ) && mTextures[mOption] != nullptr;
}

void MainApp::setup()
{
    mFolderPath = Platform::get()->getHomeDirectory();
    mExtension = ".png";
    mToken = "bina-model";
    
    assert(exists(mFolderPath));
    
    cv::namedWindow(" Bina ");
    mOption = 1;
    mSkinHue[0] = Scalar(3, 40, 90);
    mSkinHue[1] = Scalar(33, 255, 255);
    
    openFile();
    
    // Setup the parameters
    mParams = params::InterfaceGl::create ( getWindow(), "Parameters", vec2( 200, 150 ) );
    
    // Add an enum (list) selector.
    mOption = 0;
    
    
    mParams->addParam( "Display", mNames, &mOption )
    .updateFn( [this] { textureToDisplay = &mTextures[mOption]; console() << "display updated: " << mNames[mOption] << endl; } );
    
    
    mParams->addButton( "Add image ",
                       std::bind( &MainApp::openFile, this ) );
    mParams->addSeparator();
    
    
#if defined( CINDER_COCOA_TOUCH )
    mFont = Font( "Cochin-Italic", 14);
    mScoreFont = Font( "Cochin-Italic", 18);
#elif defined( CINDER_COCOA )
    mFont = Font( "Menlo", 14 );
    mScoreFont = Font( "Menlo", 18 );
#else
    mFont = Font( "Times New Roman", 14 );
    mScoreFont = Font( "Times New Roman", 32 );
#endif
    mTextureFont = gl::TextureFont::create( mFont );
    mTextureScoreFont = gl::TextureFont::create( mScoreFont );
    
    gl::enableVerticalSync( false );
    
    
    mIsClicked = false;
    mIsOver = false;
    mMouseIsDown = false;
    
}

void MainApp::mouseDown( MouseEvent event )
{
    mMouseIsDown = true;
    // Check if mouse is inside rectangle, by converting the mouse coordinates
    // to world space and then to object space. [ Excercising gl::windowToWorldCoord() ].
    vec3 world = gl::windowToWorldCoord( event.getPos() );
    vec4 object = glm::inverse( mRectangle.matrix() ) * vec4( world, 1 );
    
    // Now we can simply use Area::contains() to find out if the mouse is inside.
    mIsClicked = mRectangle.area().contains( vec2( object ) );
    if( mIsClicked ) {
        mMouseInitial = event.getPos();
        mRectangleInitial = mRectangle;
    }
}

void MainApp::mouseUp( MouseEvent event )
{
    mMouseIsDown = false;
}

std::string MainApp::pixelInfo( const ivec2 &position )
{
    // first, specify a small region around the current cursor position
    float scaleX = mTextures[mOption]->getWidth() / (float)getWindowWidth();
    float scaleY = mTextures[mOption]->getHeight() / (float)getWindowHeight();
    ivec2	pixel( (int)( position.x * scaleX ), (int)( (position.y * scaleY ) ) );
    if (! mImage ) return " No Image Available ";
    auto val = mImage->getPixel(pixel);
    ostringstream oss( ostringstream::ate );
    oss << "[" << pixel.x << "," << pixel.y << "] ";
    oss << "rgb " << to_string((int)val.r) << "," << to_string((int)val.g) << "," << to_string((int)val.b);
    if (mHSLImage)
    {
        float hsv = mHSV[0].at<uint8_t>(pixel.y, pixel.x);
        
        oss << " hue " << to_string(hsv);
    }
    // Calculate string length for this font and this size. add 5 percent for paddint
    mStringSize = mTextureFont->measureString(oss.str());
    mStringSize /= vec2(getWindowSize());
    mStringSize *= vec2(1.05f,1.05f);
    return oss.str();
    
}

std::string MainApp::resultInfo (const rank_output& output)
{
    ostringstream oss( ostringstream::ate );
    oss << "Redness: " << to_string(m_rank_score.score) << "   " << "Texture: " << to_string(m_rank_score.texture_score);
    return oss.str();
    
}

void MainApp::mouseMove( MouseEvent event )
{
    mMousePos = event.getPos();
}


void MainApp::mouseDrag( MouseEvent event )
{
    // Keep track of mouse position.
    mMousePos = event.getPos();
    
    // Scale and rotate the rectangle.
    if( mIsClicked )
    {
        // Calculate the initial click position and the current mouse position, in world coordinates relative to the rectangle's center.
        vec3 initial = gl::windowToWorldCoord( mMouseInitial ) - vec3( mRectangle.position(), 0 );
        vec3 current = gl::windowToWorldCoord( event.getPos() ) - vec3( mRectangle.position(), 0 );
        
        // Calculate scale by using the distance to the center of the rectangle.
        float d0 = glm::length( initial );
        float d1 = glm::length( current );
        mRectangle.scale (vec2( mRectangleInitial.scale() * ( d1 / d0 ) ) );
        
        
        // Calculate rotation by taking the angle with the X-axis for both positions and calculating the difference.
        float a0 = ci::math<float>::atan2( initial.y, initial.x );
        float a1 = ci::math<float>::atan2( current.y, current.x );
        mRectangle.rotation (mRectangleInitial.rotation() * glm::angleAxis( a1 - a0, vec3( 0, 0, 1 ) ) );
    }
    
}


void MainApp::fileDrop( FileDropEvent event )
{
    try {
        mImage = loadImageFromPath (event.getFile( 0 ));
        if (mImage)
        {
            mWorkingRect = Rectf (0, 0, mImage->getWidth(), mImage->getHeight());
            process();
        }
    }
    catch( std::exception &exc ) {
        CI_LOG_EXCEPTION( "failed to load image: " << event.getFile( 0 ), exc );
    }
}

void MainApp::keyDown( KeyEvent event )
{
    auto position = mRectangle.position();
    
    switch( event.getCode() )
    {
        case KeyEvent::KEY_UP:
            position.y++;
            break;
        case KeyEvent::KEY_DOWN:
            position.y--;
            break;
            
        case KeyEvent::KEY_LEFT:
            position.x--;
            break;
        case KeyEvent::KEY_RIGHT:
            position.x++;
            break;
        case KeyEvent::KEY_RIGHTBRACKET:
            mRectangle.area().inflate(vec2(1,0));
            break;
            
        case KeyEvent::KEY_LEFTBRACKET:
            mRectangle.area().inflate(vec2(-1,0));
            break;
            
        case KeyEvent::KEY_SEMICOLON:
            mRectangle.area().inflate(vec2(0,1));
            break;
            
        case KeyEvent::KEY_QUOTE:
            mRectangle.area().inflate(vec2(0,-1));
            break;
            
        case KeyEvent::KEY_c:
            if (event.isMetaDown() )
                Clipboard::setImage( copyWindowSurface() );
            
            break;
            
        case KeyEvent::KEY_v:
            if (event.isMetaDown() && Clipboard::hasImage())
                
                try {
                    mImage = Surface::create( Clipboard::getImage() );
                    
                    mWorkingRect = Rectf (0, 0, mImage->getWidth(), mImage->getHeight());
                    process();
                }
            catch( std::exception &exc ) {
                CI_LOG_EXCEPTION( "failed to paste image: ", exc );
            }
            
            
            break;
            
        case KeyEvent::KEY_SPACE:
            mOption = (mOption + 1) % mNames.size();
            break;
        case KeyEvent::KEY_f:
            setFullScreen (! isFullScreen () );
            break;
        case KeyEvent::KEY_r:
            position = getWindowSize() / 2;
            break;
        case KeyEvent::KEY_ESCAPE:
            if (isFullScreen())
                setFullScreen(false);
            else
                quit();
    }
    mRectangle.position(position);
    
}



void MainApp::hueMappedOrientation(cv::Mat source, cv::Mat &destination){
   
    cv::Mat gray, mag, ang, hsv;
    std::vector<cv::Mat> channels;
    channels.resize(source.channels());
    
    cv::cvtColor(source, gray, CV_RGB2GRAY);
    
    svl::sobel_opencv(gray,mag, ang);

    // Map to Hue Hue is 180 degrees
    // Scales, calculates absolute values, and converts the result to 8-bit.
    ang.convertTo(ang,CV_8U,256.0/180.0);
    cv::convertScaleAbs(ang,ang);
    cv::multiply(mag, cv::Scalar(2.0), mag);
    mag.convertTo(mag,CV_8U);

    
    hsv = source.clone();
    channels[0] = ang; // set orientation as hue channel
    channels[1] = mag;
    channels[2] = mag;
    
    cv::merge(channels, hsv);
    cv::cvtColor(hsv, destination, CV_HSV2RGB);
}

void MainApp::RGBtoYxy(cv::Mat source, cv::Mat &destination){
    
    cv::Mat scaled;
    source.copyTo(scaled);
    scaled.convertTo(scaled, CV_64F);
    
    cv::Mat sRGBtoXYZ=(cv::Mat_<double>(3, 3) << 0.4124564,  0.3575761,  0.1804375, 0.2126729,  0.7151522,  0.0721750, 0.0193339, 0.1191920, 0.9503041);
    int rows=source.rows;
    int cols=source.cols;
    
    cv::Mat reshaped=scaled.reshape(1, rows*cols);
    cv::transpose(reshaped, reshaped);
    cv::Mat xyz=sRGBtoXYZ*reshaped;
    
    cv::transpose(xyz, xyz);
    xyz=xyz.reshape(3, rows);
    
    double eps=2.2204e-16;
    std::vector<cv::Mat> channels;
    cv::split(xyz, channels);
    cv::Mat sum;
    channels[0].copyTo(sum);
    for (int i=1;i<3;++i){
        sum+=channels[i];
    }
    sum+=eps;
    
    cv::Mat x;
    cv::divide(channels[0], sum, x);
    cv::Mat y;
    cv::divide(channels[1], sum, y);
    
    std::vector<cv::Mat> resultChannels;
    resultChannels.push_back(channels[1]);
    resultChannels.push_back(x);
    resultChannels.push_back(y);
    
    cv::Mat result;
    cv::merge(resultChannels, result);
    
    result.copyTo(destination);
}

void MainApp::process ()
{
    
    cv::Mat inputMatF;
    mTextures.resize(mNames.size());
    mOverlays.resize(mNames.size());
    textureToDisplay = &mTextures[0];
    
    cv::Mat hsv;
    cv::Mat mask;
    cv::Mat xyz;

    mInputMat = cv::Mat ( toOcv ( *mImage) );
    
    // resize it with aspect in tact
 //   cv::resize(mInputMat, mInputMat, cv::Size(mInputMat.cols * 2 , mInputMat.rows * 2 ));
    cv::Mat clear = mInputMat.clone();
    clear = 0;
    
    
    cv::Mat none = mInputMat.clone();
    mTextures[0] = gl::Texture::create(fromOcv(none));
    mOverlays[0] = gl::Texture::create(fromOcv(clear));
    

    cv::Mat tmpi = mInputMat.clone();
    mTextures[1] = gl::Texture::create(fromOcv(tmpi));
    mOverlays[1] = gl::Texture::create(fromOcv(clear));
    
   // hueMappedOrientation(tmpi, xyz);
    
 
    
    // convert to hsv and show hue
    // get the vector of Mats for processing
    // Surface for display



    
    setWindowSize(mInputMat.cols * 2, mInputMat.rows * 2);
}



void MainApp::draw()
{
        gl::enableAlphaBlending();
    
    {
        //    gl::ScopedViewport vp(getWindowPos(), getWindowSize());

        gl::draw(mTextures[mOption], getWindowBounds());
        
        
        
        if (mOverlays[mOption])
        {
            gl::ScopedColor color (ColorA (1.0,0.0,1.0,0.25f));
            gl::draw(mOverlays[mOption], getWindowBounds());
        }
        
        if (mIsOver)
        {
            gl::pushModelView();
            
            gl::ScopedColor color (ColorA (0.33, 0.33, 1.0,1.0f));
            mTextureFont->drawString( pixelInfo(mMousePos), ivec2(20, (1.02f - mStringSize.y)*getWindowHeight()));
            gl::popModelView();
        }
        
        if (m_rank_score.valid)
        {
            gl::pushModelView();
            
            Rectf frac (0.30f, 0.02f, 0.95f, 0.15f);
            Rectf box (frac.x1*getWindowWidth(),frac.y1*getWindowHeight(),
                       frac.x2*getWindowWidth(),frac.y2*getWindowHeight());
            {
                gl::ScopedColor color (ColorA (0.0, 0.0, 0.0,0.33f));
                gl::drawSolidRoundedRect (box, 0.5f);
            }
            
            gl::ScopedColor color (ColorA (1.0, 1.0, 1.0,0.8f));
            mTextureScoreFont->drawString( resultInfo (m_rank_score),
                                          ivec2( box.x1+10, (box.y1+box.y2)/2));
            gl::popModelView();
            
        }
    }
    
    // Not using the affine rect
    drawEditableRect();
    
    
    // Draw the interface
    if(mParams)
        mParams->draw();
}

void MainApp::drawEditableRect()
{
    
    // Either use setMatricesWindow() or setMatricesWindowPersp() to enable 2D rendering.
    gl::setMatricesWindow( getWindowSize(), true );
    
    // Draw the transformed rectangle.
    gl::pushModelMatrix();
    gl::multModelMatrix( mRectangle.matrix() );
    
    
    // Draw the same rect as line segments.
    affineRectangle::draw_oriented_rect (mRectangle.area());
    
    
    
    // Draw a stroked rect in magenta (if mouse inside) or white (if mouse outside).
    ColorA cl (1.0, 1.0, 1.0,0.8f);
    if (mIsOver) cl = ColorA (1.0, 0.0, 1.0,0.8f);
    
    gl::ScopedColor color (cl);
    
    gl::drawStrokedRect( mRectangle.area());
    
    // Draw the 4 corners as yellow crosses.
    float dsize = 5.0f;
    int i = 0;
    for( const vec2 &corner : mRectangle.worldCorners() )
    {
        gl::drawLine( vec2( corner.x, corner.y - 5 ), vec2( corner.x, corner.y + 5 ) );
        gl::drawLine( vec2( corner.x - 5, corner.y ), vec2( corner.x + 5, corner.y ) );
        ColorA cl (1.0, 0.0, 0.0,0.8f);
        if (i==0 || i == 1)
            cl = ColorA (0.0, 0.0, 1.0,0.8f);
        gl::drawStrokedCircle(corner, dsize, dsize+5.0f);
        dsize+=5.0f;
        i++;
    }
    
    //    gl::drawStrokedEllipse(mRectangle.position(), mRectangle.area().getWidth()/2, mRectangle.area().getHeight()/2);
    
    gl::popModelMatrix();
    
    mAffineRect = mRectangle.get_rotated_rect();
    mAffineCenter = mAffineRect.center;
    
    std::vector<Point2f> cvcorners;
    compute2DRectCorners(mAffineRect, cvcorners);
    affineRectangle::draw_oriented_rect(cvcorners);
    
}


cv::Mat MainApp::affineCrop (cv::Mat& src, cv::RotatedRect& rect)
{
    Mat M, rotated, cropped;
    float angle = rect.angle;
    auto rect_size = rect.size;
    
    
    // get the rotation matrix
    M = getRotationMatrix2D(rect.center, angle, 1.0);
    // perform the affine transformation
    warpAffine(src, rotated, M, src.size(), INTER_CUBIC, BORDER_REPLICATE);
    // crop the resulting image
    getRectSubPix(rotated, rect_size, rect.center, cropped);
    return cropped;
}


CINDER_APP ( MainApp, RendererGl );


#pragma GCC diagnostic pop

