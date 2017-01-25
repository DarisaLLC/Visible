
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
#include "CinderOpenCV.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <time.h>
#include "core/stats.hpp"
#include "vision/ss_segmenter.hpp"
#include "vision/ColorHistogram.hpp"
#include "vision/histogram.h"
#include "cinder/affine_rectangle.h"
#include "vision/opencv_utils.hpp"
#include "boost/filesystem.hpp"
#include "vision/histo.h"
#include "vision/ipUtils.h"
#include "vision/bit_chrome.hpp"

#include <string>
#include <vector>
#include "core/singleton.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;


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
    void saveModel ();
    void loadModel ();
    void saveImage(const cv::Mat & image, const std::string & filename);
    cv::Mat create_stitch_mask (const cv::Mat & image);

    void get_all(const fs::path& root, const string& ext, const std::string& prefix, vector<fs::path>& ret);
    void fileDrop( FileDropEvent event ) override;
    void draw()override;
    void process ();
    std::string pixelInfo ( const ivec2 & position);
    std::string resultInfo (const rank_output& output);
    void drawEditableRect ();
    void openFile();
    void trainWoundTemplate ();
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
    
    std::vector<gl::TextureRef> mTextures;
    std::vector<gl::TextureRef> mOverlays;
    gl::TextureRef* textureToDisplay;
    gl::TextureRef mOverlay;
    RectMapping mNormalMap;
    RectMapping mInveseMap;
    
    int mOption;
    params::InterfaceGlRef	mParams;
    vector<string> mNames = { "Input", "Redness"};
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
    
    void compute_col (const cv::Mat& mat, vec2 center, float& direction)
    {

//        assert( CV_MAT_CN( mat.type() ) == 1 );
//        
//        CvMoments moments = cv::moments(mat);
//        
//        // Use rule for specular reflection
//        double inv_m00 = moments.inv_sqrt_m00*moments.inv_sqrt_m00;
//        Point2f com (( moments.m10 * inv_m00), moments.m01 * inv_m00 );
//        std::cout << com << std::endl;
//       // Histogram1D ocvhist;
//      //  ocvhist.getHistogram(mat);
//      //  histoStats hh (mat);
//        
//        uint8_t average = (moments.m00 / (float)(mat.rows*mat.cols));
//        std::cout << "Average: " << average << std::endl;
//        
//        float r2 = 0.0f;
//        for (int h = 0; h < mat.rows; h++)
//            for (int w = 0; w < mat.cols; w++)
//            {
//                if (mat.at<uint8_t>(h,w) < average) continue;
//                r2 = std::max(r2, (h-com.y)*(h-com.y)+(w-com.x)*(w-com.x));
//            }
//        float r (std::sqrt(r2));
//        float dx = com.x - mat.cols/2.0f;
//        float dy = mat.rows/2.0f - com.y;
//        float prod = r2 - dx*dx - dy*dy;
//        Mat N (Vec3f(dx/r, dy/r, std::sqrt(prod)/r));
//        std::cout << N << std::endl;
//        Mat R (Vec3f(0.0f,0.0f,1.0f));
//        cv::Mat L = N.dot(R)*N - R;
//        //L = (cosτ ·sinσ, sinσ ·sinτ, cosσ) is the unit vector towards the light source;
//        //σ and τ are the illuminant vector’s slant and tilt angles
//        std::cout << L << std::endl;
//        float slant = acos(L.at<float>(0,2));
//        float tilt = asin(L.at<float>(0,1)/sin(slant));
//        std::cout << "Slant: " << toDegrees(slant) << "Tilt: " << toDegrees(tilt) << std::endl;
//        
        
        
    }
    
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
void MainApp::saveModel()
{
    std::ostringstream ss;
    std::string filename = get_timestamp_filename();
    ss << mFolderPath.string()  << "/" << filename + ".png";
    const string dirAndFilename = ss.str();
    cout << "Saving " << dirAndFilename << endl;
    cv::imwrite(dirAndFilename, mModel);
}

void MainApp::loadModel()
{
    std::vector<fs::path> models;
    get_all(mFolderPath, mExtension, mToken, models);
    if(models.size())
    {
        auto ir = loadImage( mFolderPath.string() + "/" + models[0].string() );
        mModelImage = Surface::create( ir );
        mModel = cv::Mat ( toOcv ( *mModelImage) );
    }
}

double getPSNR(const Mat& I1, const Mat& I2)
{
    Mat s1;
    absdiff(I1, I2, s1);       // |I1 - I2|
    s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
    s1 = s1.mul(s1);           // |I1 - I2|^2
    
    Scalar s = sum(s1);         // sum elements per channel
    
    double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels
    
    if( sse <= 1e-10) // for small values return zero
        return 0;
    else
    {
        double  mse =sse /(double)(I1.channels() * I1.total());
        double psnr = 10.0*log10((255*255)/mse);
        return psnr;
    }
}

// return the filenames of all files that have the specified extension
// in the specified directory and all subdirectories
void MainApp::get_all(const fs::path& root, const string& ext, const std::string& prefix, vector<fs::path>& ret)
{
    if(!fs::exists(root) || !fs::is_directory(root)) return;
    
    fs::recursive_directory_iterator it(root);
    fs::recursive_directory_iterator endit;
    
    while(it != endit)
    {
        if(fs::is_regular_file(*it) &&
           it->path().extension() == ext &&
           it->path().filename().string().substr(0, prefix.size()) == prefix)
            ret.push_back(it->path().filename());
        ++it;
        
    }
}



void MainApp::openFile()
{
    try {
        fs::path path = getOpenFilePath( "", ImageIo::getLoadExtensions() );
        if( ! path.empty() )
        {
            auto ir = loadImage( path );
            mImage = Surface::create( ir );
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
    
    cv::namedWindow("Main");
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
    mParams->addButton( "Grab First Look",
                       std::bind( &MainApp::trainWoundTemplate, this ) );
    mParams->addButton( "Save First Look ",
                       std::bind( &MainApp::saveModel, this ) );
    mParams->addSeparator();
    mParams->addButton( "Load First Look ",
                       std::bind( &MainApp::loadModel, this ) );
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
        mImage = Surface::create( loadImage( loadFile( event.getFile( 0 ) ) ) );

        mWorkingRect = Rectf (0, 0, mImage->getWidth(), mImage->getHeight());
        process();
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

void MainApp::process ()
{
    
    cv::Mat inputMatF;
    mTextures.resize(mNames.size());
    mOverlays.resize(mNames.size());
    textureToDisplay = &mTextures[0];
    
    cv::Mat hsv;
    cv::Mat mask;
   
    mInputMat = cv::Mat ( toOcv ( *mImage) );
    

    
    // resize it with aspect in tact
    int size = std::sqrt( 1 + (mInputMat.rows * mInputMat.cols) / 500000.);
    cv::resize(mInputMat, mInputMat, cv::Size(mInputMat.cols / size , mInputMat.rows / size ));
    cv::Mat clear = mInputMat.clone();
    clear = 0;
    cv::Mat tmpi = mInputMat.clone();
    mTextures[0] = gl::Texture::create(fromOcv(tmpi));
    mOverlays[0] = gl::Texture::create(fromOcv(clear));
    
    // convert to hsv and show hue
    // get the vector of Mats for processing
    // Surface for display
    cvtColor(mInputMat, hsv, cv::COLOR_RGB2HSV_FULL);
    split(hsv, mHSV);
    mHSLImage = Surface8u::create (fromOcv(hsv));

    roiWindow<P8U> hueroi (mHSV[0].cols, mHSV[0].rows);
    cv::Mat original = mHSV[0].clone();
    cv::GaussianBlur(mHSV[0], mHSV[0], cv::Size(21,21), 0.0f, 0.0f);

    NewFromOCV(mHSV[0], hueroi);
    std::pair<uint8_t,std::vector<std::pair< uint8_t, int> > > segres;
    segres = segmentationByInvaraintStructure(hueroi);
    std::cout << (int) segres.first << std::endl;
    m_rank_score.valid = false;
    get_output(segres.second, 180, segres.first, m_rank_score);
    m_rank_score.texture_score = 1.0f / (1 + (float) getPSNR(mHSV[0], original));
    
    cv::inRange(hsv,mSkinHue[0], mSkinHue[1], mask);
    mOverlays[1] = gl::Texture::create(fromOcv(clear));
    mTextures[1] = mTextures[0];

     
    setWindowSize(mInputMat.cols, mInputMat.rows);
}



void MainApp::draw()
{
    
    
    {
        //    gl::ScopedViewport vp(getWindowPos(), getWindowSize());
        gl::enableAlphaBlending();
        gl::draw(*textureToDisplay, getWindowBounds());
        
        
        
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
    //  drawEditableRect();
    
    
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
    if( mIsOver )
        gl::color( 1, 0, 1 );
    else
        gl::color( 1, 1, 1 );
    gl::drawStrokedRect( mRectangle.area());
    
    // Draw the 4 corners as yellow crosses.
    float size = 5.0f;
    int i = 0;
    for( const vec2 &corner : mRectangle.worldCorners() )
    {
        gl::drawLine( vec2( corner.x, corner.y - 5 ), vec2( corner.x, corner.y + 5 ) );
        gl::drawLine( vec2( corner.x - 5, corner.y ), vec2( corner.x + 5, corner.y ) );
        if (i==0 || i == 1)
            gl::color( 0, 0, 1 );
        else
            gl::color( 1, 0, 0 );
        gl::drawStrokedCircle(corner, size, size+=5.0f);
        i++;
    }
    
    //    gl::drawStrokedEllipse(mRectangle.position(), mRectangle.area().getWidth()/2, mRectangle.area().getHeight()/2);
    
    gl::popModelMatrix();
    
    mAffineRect = mRectangle.get_rotated_rect();
    mAffineCenter = mAffineRect.center;
    gl::ScopedColor( 1, 0, 1 );
    std::vector<Point2f> cvcorners;
    compute2DRectCorners(mAffineRect, cvcorners);
    affineRectangle::draw_oriented_rect(cvcorners);
    
}

cv::Mat MainApp::create_stitch_mask (const cv::Mat& image)
{
    
    Mat mhFiltered; // resulting (MHat-filtered image)
    Mat gaussianBlurredImage;
    Mat in_1, diff16;
    cvtColor(image, mhFiltered, COLOR_BGR2GRAY );
    mhFiltered.convertTo(in_1, CV_16SC1);
    cv::GaussianBlur(mhFiltered, gaussianBlurredImage, cv::Size(19,19), 5.00);
    gaussianBlurredImage.convertTo(diff16, CV_16SC1);
    cv::subtract(in_1, diff16, diff16);
 
    // find minimum and maximum values:
    double min, max;
    cv::minMaxLoc(diff16, &min, &max);
    std::cout << "min pixel value: " << max - min << std::endl;
    // scale the pixel values so that the smalles value is 0 and the largest one is 255
    diff16.convertTo(mConvertedTmp,CV_8UC1, 255.0/(max-min), -min);
    //    // histogram of 16 bins
    Histogram1D h;
    h.setNBins(256);
//    cv::namedWindow("Reference Histogram");
//    cv::imshow("Reference Histogram",h.getHistogramImage(mConvertedTmp));
    std::cout << h.getHistogram(mConvertedTmp) << std::endl;
    cv::threshold(mConvertedTmp, mConvertedTmp, 50, 255, cv::THRESH_BINARY);
    cv::Mat copy = image.clone();
    for (int h = 0; h < mConvertedTmp.rows; h++)
        for (int v = 0; v < mConvertedTmp.cols; v++)
        {
            if(mConvertedTmp.at<uint8_t>(h,v) == 0)
                copy.at<Vec3b>(h,v)[2] = 255;
        }
    
    return copy;
    
  
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


void MainApp::trainWoundTemplate ()
{
    cv::Mat canvas = cv::Mat ( toOcv ( *mImage) );
    mModel = MainApp::affineCrop(canvas, mAffineRect);
    {
        Scalar color = Scalar( 255, 128, 0 );
        
        cv_drawCross(canvas, mAffineRect.center, Scalar (0,128,192), 30);
        cv_drawCross(canvas, Point2i(10, 10), Scalar (0,255,0), 30);
        
        Point2f rect_points[4]; mAffineRect.points( rect_points );
        
        for( int j = 0; j < 4; j++ )
            line( canvas, rect_points[j], rect_points[(j+1)%4], color, 1, 8 );
        
        cv::Mat masked = create_stitch_mask(mModel);
        cv::imshow("Main", masked);
        
        cv::Mat modelcpy = mModel.clone();
        cv::rectangle(modelcpy, cv::Point(0,0), cv::Point(mModel.cols-1, mModel.rows-1), Scalar(1,0,0));
        cv::namedWindow("Affine Crop", CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);
        cv::imshow("Affine Crop", modelcpy);
        cv::resizeWindow("Affine Crop", mModel.cols, mModel.rows);
    }
    
    
}


CINDER_APP ( MainApp, RendererGl );
