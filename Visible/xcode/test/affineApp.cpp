

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
#include "core/stats.hpp"
#include "cinder_cv/affine_rectangle.h"
#include "vision/opencv_utils.hpp"
#include "boost/filesystem.hpp"
#include "vision/histo.h"
#include "vision/ipUtils.h"
#include "cvplot/cvplot.h"

#include <string>
#include <vector>

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



class affineApp : public AppMac {
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
 
    void generate_crop ();
    glm::vec2 screenToObject( const glm::vec2 &pt, float z ) const;
    glm::vec2 objectToScreen( const glm::vec2 &pt ) const;

    Surface8uRef  loadImageFromPath (const filesystem::path&);
    void fileDrop( FileDropEvent event ) override;
    void draw()override;
    void process ();
    std::string resultInfo ();
    std::string pixelInfo ( const ivec2 & position);
    void drawEditableRect ();
    void openFile();
    static cv::Mat affineCrop (cv::Mat& src, cv::RotatedRect& rect, float aspect);
    vec2 matStats (const cv::Mat& image);
    Surface8uRef   mImage;
    
    cv::Mat mInputMat;
    cv::Mat mPadded;
    vector<cv::Mat> mLogs;
    
    std::vector<gl::TextureRef> mTextures;
    std::vector<gl::TextureRef> mOverlays;
    gl::TextureRef* textureToDisplay;
    gl::TextureRef mOverlay;
    
    int mOption;
    params::InterfaceGlRef    mParams;
    vector<string> mNames = { "Input",  "Orientation"};
    ivec2            mMousePos;
    Font                mFont;
    Font                mScoreFont;
    gl::TextureFontRef    mTextureFont;
    gl::TextureFontRef    mTextureScoreFont;
    
    affineRectangle   mRectangle;
    affineRectangle   mRectangleInitial;
    ivec2          mMouseInitial;
    Area           mPaddedArea, mImageArea;
    cv::RotatedRect  mRotatedRect;
    
    vec2           mStringSize;
    bool           mIsOver;
    bool           mIsClicked;
    bool           mMouseIsDown;
    bool           mCtrlDown;
    
    cv::Mat mModel;
    boost::filesystem::path mFolderPath;
    std::string mExtension;
    int m_pads [4];

    
};


Surface8uRef affineApp::loadImageFromPath (const filesystem::path& path)
{
    Surface8uRef sref;
    
    if (is_anaonymous_name(path))
        sref = Surface8u::create( loadImage( path.string(), ImageSource::Options(), "jpg"));
    else
        sref = Surface8u::create( loadImage( path.string() ) );
    return sref;
    
}

vec2 affineApp::matStats (const cv::Mat& image)
{
    // compute statistics for Hue value
    cv::Scalar mean, stddev;
    cv::meanStdDev(image, mean, stddev);
    return vec2(mean[0], stddev[0]);
}
void affineApp::resize()
{
    mRectangle.resize(getWindowBounds());
    std::cout << " resize is not implemented " << std::endl;
    
}

void affineApp::openFile()
{
    try {
        mImage = loadImageFromPath(getOpenFilePath());
        if (mImage)
        {
            process();
        }
    }
    catch( std::exception &exc ) {
        CI_LOG_EXCEPTION( "failed to load image.", exc );
    }
}

void affineApp::update()
{
}


void affineApp::generate_crop (){

    auto cvDrawImage = [] (cv::Mat& image){
            auto name = "image";
            auto &view = cvplot::Window::current().view(name);
            cvplot::moveWindow(name, 0, 0);
            view.size(cvplot::Size(image.cols*3,image.rows*3));
            view.drawImage(&image);
            view.finish();
            view.flush();
        };
    
    auto cvDrawPlot = [&] (std::vector<float>& one_d, cvplot::Size& used, std::string& name){
        {
            auto name = "transparent";
            auto alpha = 200;
            cvplot::setWindowTitle(name, "transparent");
            cvplot::moveWindow(name, used.width, 0);
            cvplot::resizeWindow(name, 300+used.width, 300+used.height);
            cvplot::Window::current().view(name).frameColor(cvplot::Sky).alpha(alpha);
            auto &figure = cvplot::figure(name);
            figure.series("Projection")
            .setValue(one_d)
            .type(cvplot::Dots)
            .color(cvplot::Blue.alpha(alpha))
            .legend(false);
            figure.alpha(alpha).show(true);

        }
    };
    
    auto cp = mRectangle.rotatedRectInImage(mPaddedArea);
    cv::Mat crop = affineCrop(mPadded, cp, mRectangle.aspect());
    cv::Mat hz (1, crop.cols, CV_32F);
    cv::Mat vt (crop.rows, 1, CV_32F);
    horizontal_vertical_projections (crop, hz, vt);
    std::vector<float> hz_vec(hz.ptr<float>(0),hz.ptr<float>(0)+hz.cols );
    std::vector<float> vt_vec(crop.rows);
    for (auto ii = 0; ii < crop.rows; ii++) vt_vec[ii] = vt.at<float>(ii,0);
    cvDrawImage(crop);
    std::string hhh ("hz");
    std::string vvv ("vt");
    cvplot::Size used (crop.cols*3, crop.rows*3);
    cvDrawPlot(vt_vec, used, vvv);
}


void affineApp::setup()
{
    mFolderPath = Platform::get()->getHomeDirectory();
    mExtension = ".png";
    assert(exists(mFolderPath));
    cv::namedWindow(" Bina ");
    openFile();
    
    // Setup the parameters
    mParams = params::InterfaceGl::create ( getWindow(), "Parameters", vec2( 200, 150 ) );
    mOption = 0;
    mParams->addParam( "Display", mNames, &mOption )
    .updateFn( [this] { textureToDisplay = &mTextures[mOption]; console() << "display updated: " << mNames[mOption] << endl; } );
    mParams->addButton( "Add image ",
                       std::bind( &affineApp::openFile, this ) );
    
    mParams->addButton(" Crop Affine ", std::bind(&affineApp::generate_crop, this));
    
#if defined( CINDER_COCOA_TOUCH )
    mFont = Font( "Cochin-Italic", 14);
    mScoreFont = Font( "Cochin-Italic", 18);
#elif defined( CINDER_COCOA )
    mFont = Font( "Menlo", 18 );
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

void affineApp::mouseDown( MouseEvent event )
{
    mMouseIsDown = true;
    mRectangle.mouseDown(event.getPos());
    
    mIsClicked = mRectangle.isClicked();
    if( mIsClicked ) {
        mMouseInitial = event.getPos();
        mRectangleInitial = mRectangle;
    }
}

void affineApp::mouseUp( MouseEvent event )
{
    mMouseIsDown = false;
    mRectangle.mouseUp();
}

std::string affineApp::pixelInfo( const ivec2 &position )
{
    // first, specify a small region around the current cursor position
    float scaleX = mTextures[mOption]->getWidth() / (float)getWindowWidth();
    float scaleY = mTextures[mOption]->getHeight() / (float)getWindowHeight();
    ivec2    pixel( (int)( position.x * scaleX ), (int)( (position.y * scaleY ) ) );
    if (! mImage ) return " No Image Available ";
    auto val = mImage->getPixel(pixel);
    ostringstream oss( ostringstream::ate );
    oss << "[" << pixel.x << "," << pixel.y << "] ";
    oss << "rgb " << to_string((int)val.r) << "," << to_string((int)val.g) << "," << to_string((int)val.b);

    // Calculate string length for this font and this size. add 5 percent for paddint
    mStringSize = mTextureFont->measureString(oss.str());
    mStringSize /= vec2(getWindowSize());
    mStringSize *= vec2(1.05f,1.05f);
    return oss.str();
    
}

std::string affineApp::resultInfo ()
{
    ostringstream oss( ostringstream::ate );
    oss << " Affine Prototype ";
    return oss.str();
    
}

void affineApp::mouseMove( MouseEvent event )
{
    mMousePos = event.getPos();
    mRectangle.mouseMove(mMousePos);
    mIsOver = mRectangle.isOver();
}


void affineApp::mouseDrag( MouseEvent event )
{
    mRectangle.mouseDrag(event.getPos());
    // Keep track of mouse position.
    mMousePos = event.getPos();
    mIsClicked = mRectangle.isClicked();
    
}


void affineApp::fileDrop( FileDropEvent event )
{
    try {
        mImage = loadImageFromPath (event.getFile( 0 ));
        if (mImage)
        {
            process();
        }
    }
    catch( std::exception &exc ) {
        CI_LOG_EXCEPTION( "failed to load image: " << event.getFile( 0 ), exc );
    }
}

void affineApp::keyDown( KeyEvent event )
{
    static float trans = (event.isControlDown()) ? 2.0f : 1.0f;
    
    vec2 one (0.05, 0.05);
    float step = svl::constants::pi / 180.;
    
    switch( event.getCode() )
    {
        case KeyEvent::KEY_UP:
            mRectangle.translate(vec2(0,-trans));
            break;
        case KeyEvent::KEY_DOWN:
            mRectangle.translate(vec2(0,trans));
            break;
        case KeyEvent::KEY_LEFT:
            mRectangle.translate(vec2(-trans,0));
            break;
        case KeyEvent::KEY_RIGHT:
            mRectangle.translate(vec2(trans,0));
            break;
        case KeyEvent::KEY_RIGHTBRACKET:
            mRectangle.scale(vec2(-one.x,0));
            break;
        case KeyEvent::KEY_LEFTBRACKET:
            mRectangle.scale(vec2(one.x,0));
            break;
        case KeyEvent::KEY_SEMICOLON:
            mRectangle.scale(vec2(0,-one.y));
            break;
            
        case KeyEvent::KEY_QUOTE:
            mRectangle.scale(vec2(0,one.y));
            break;
        case KeyEvent::KEY_BACKSLASH:
            mRectangle.rotate(-step);
            break;
        case KeyEvent::KEY_SLASH:
            mRectangle.rotate(step);
            break;
            
        case KeyEvent::KEY_c:
            if (event.isMetaDown() )
                Clipboard::setImage( copyWindowSurface() );
            
            break;
            
        case KeyEvent::KEY_v:
            if (event.isMetaDown() && Clipboard::hasImage())
                
                try {
                    mImage = Surface::create( Clipboard::getImage() );
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
            std::cout << " Key r is not implemented " << std::endl;
            
            break;
        case KeyEvent::KEY_ESCAPE:
            if (isFullScreen())
                setFullScreen(false);
            else
                quit();
    }
}




void affineApp::process ()
{
    mTextures.resize(mNames.size());
    mOverlays.resize(mNames.size());
    textureToDisplay = &mTextures[0];
    
    cv::Mat mask;
    
    mInputMat = cv::Mat ( toOcv ( *mImage) );
    m_pads[0] = mInputMat.rows;
    m_pads[1] = mInputMat.rows;
    m_pads[2] = m_pads[3] = 0;
    
    auto istats = matStats(mInputMat);
    cv::copyMakeBorder(mInputMat, mPadded, m_pads[0], m_pads[1],m_pads[2], m_pads[3], cv:: BORDER_CONSTANT , cv::Scalar::all(istats[0]));
    cv::Mat clear = mPadded.clone();
    clear = 0;
    cv::Mat none = mPadded.clone();
    
    mTextures[0] = gl::Texture::create(fromOcv(none));
    mOverlays[0] = gl::Texture::create(fromOcv(clear));
    cv::Mat tmpi = mPadded.clone();
    mTextures[1] = gl::Texture::create(fromOcv(tmpi));
    mOverlays[1] = gl::Texture::create(fromOcv(clear));
    
    mPaddedArea = Area (0,0,mPadded.cols, mPadded.rows);
    mImageArea = Area(0,0,mInputMat.cols, mInputMat.rows);
    setWindowSize(mPadded.cols, mPadded.rows);
    
    mRotatedRect.angle = toDegrees(svl::constants::pi / 6.0);
   mRotatedRect.center = toOcv(mImageArea.getCenter());
   mRotatedRect.size.width = 50;
   mRotatedRect.size.height = 100;
    mRectangle.init(getWindowBounds(), mImageArea,mRotatedRect,mPaddedArea);
    
}



void affineApp::draw()
{
    gl::enableAlphaBlending();
    {
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
    }
    
    mRectangle.draw(getWindowBounds());
    
    // Draw the interface
    if(mParams)
        mParams->draw();
    
}

cv::Mat affineApp::affineCrop (cv::Mat& src, cv::RotatedRect& in_rect, float aspect)
{
    auto rect = in_rect;
    Mat M, rotated, cropped;
    float angle = rect.angle;
    auto rect_size = rect.size;
    float in_aspect = (float)rect.size.width / (float) rect.size.height;
    if ((in_aspect > 1.0) != (aspect > 1.0)){
        std::swap(rect.size.width,rect.size.height);
    }
    
    std::string rs = to_string(rect);
    std::cout << std::endl << rs << std::endl;
    
    // get the rotation matrix
    M = getRotationMatrix2D(rect.center, angle, 1.0);
    // perform the affine transformation
    warpAffine(src, rotated, M, src.size(), INTER_CUBIC, BORDER_REPLICATE);
    // crop the resulting image
    getRectSubPix(rotated, rect_size, rect.center, cropped);
    return cropped;
}


CINDER_APP ( affineApp, RendererGl );


#pragma GCC diagnostic pop

