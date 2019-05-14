

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
#include "cinder_cv/editableRectangle.hpp"
#include "vision/opencv_utils.hpp"
#include "boost/filesystem.hpp"
#include "vision/histo.h"
#include "vision/ipUtils.h"
#include "cvplot/cvplot.h"

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
 
   // void generate_crop ();
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
    static cv::Mat affineCrop (cv::Mat& src, cv::RotatedRect& rect);
    vec2 matStats (const cv::Mat& image);
    Surface8uRef   mImage;
    Surface8uRef   mModelImage;
    Surface8uRef   mHSLImage;
    
    cv::Mat mInputMat;
    cv::Mat mPadded;
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
    params::InterfaceGlRef    mParams;
    vector<string> mNames = { "Input",  "Orientation"};
    ivec2            mMousePos;
    vector<vector<cv::Point> > contours;
    vector<Vec4i> hierarchy;
    Font                mFont;
    Font                mScoreFont;
    gl::TextureFontRef    mTextureFont;
    gl::TextureFontRef    mTextureScoreFont;
    
    affineRectangle   mRectangle;
    affineRectangle   mRectangleInitial;
    Rectf          mWorkingRect;
    ivec2          mMouseInitial;
    
    vec2           mStringSize;
    bool           mIsOver;
    bool           mIsClicked;
    bool           mMouseIsDown;
    
    cv::Mat mModel;
    cv::Mat mConvertedTmp;
    boost::filesystem::path mFolderPath;
    std::string mExtension;
    std::string mToken;
    int m_pads [4];

    
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
//    setWindowSize(getWindowSize());
 //   mRectangle.resize(getWindowWidth(),getWindowHeight());
//    vec2 newsize = mRectangle.scale_map(getWindowSize());
//    mRectangle = affineRectangle (Rectf(0,0,newsize[0], newsize[1]));
//    mRectangle.position (vec2(newsize[0]/2, newsize[1]/2));
}




void MainApp::openFile()
{
    try {
        mImage = loadImageFromPath(getOpenFilePath());
        if (mImage)
        {
            mRectangle = affineRectangle (Area(0.0,0.0,mImage->getWidth(), mImage->getHeight()), getWindowSize() / 2);
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
    vec3 object = mRectangle.mouseToWorld(mMousePos);
    mIsOver = mRectangle.area().contains( vec2( object ) ) && mTextures[mOption] != nullptr;
}

#if 0
void MainApp::generate_crop (){

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
//            cvplot::setMouseCallback(name, mouse_callback);
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
    
    auto cp = mRectangle.rotated_rect();
    vec2 scale (getWindowWidth() / (float) mPadded.cols, getWindowHeight()/ (float) mPadded.rows);
    cp.center.x = cp.center.x / scale.x;
    cp.center.y = cp.center.y / scale.y;
    cp.size.width =     cp.size.width / scale.x;
    cp.size.height =     cp.size.height / scale.y;
    RotatedRect sp (cp.center, cp.size, -cp.angle);
    cv::Mat crop = affineCrop(mPadded, sp);
    cv::Mat hz (1, crop.cols, CV_32F);
    cv::Mat vt (crop.rows, 1, CV_32F);
    horizontal_vertical_projections (crop, hz, vt);
    std::vector<float> hz_vec(hz.ptr<float>(0),hz.ptr<float>(0)+hz.cols );
    std::vector<float> vt_vec(crop.rows);
    for (auto ii = 0; ii < crop.rows; ii++) vt_vec[ii] = vt.at<float>(ii,0);
    std::cout << " = " << crop.cols << "," << crop.rows << std::endl;
    cvDrawImage(crop);
    std::string hhh ("hz");
    std::string vvv ("vt");
    cvplot::Size used (crop.cols*3, crop.rows*3);
    cvDrawPlot(vt_vec, used, vvv);
    
    
}
#endif

void MainApp::setup()
{
    mFolderPath = Platform::get()->getHomeDirectory();
    mExtension = ".png";
    mToken = "bina-model";
    
    assert(exists(mFolderPath));
    
    cv::namedWindow(" Bina ");

    openFile();
    
    // Setup the parameters
    mParams = params::InterfaceGl::create ( getWindow(), "Parameters", vec2( 200, 150 ) );
    
    // Add an enum (list) selector.
    mOption = 0;
    
    
    mParams->addParam( "Display", mNames, &mOption )
    .updateFn( [this] { textureToDisplay = &mTextures[mOption]; console() << "display updated: " << mNames[mOption] << endl; } );
    
    
    mParams->addButton( "Add image ",
                       std::bind( &MainApp::openFile, this ) );
    
 //   mParams->addButton(" Crop Affine ", std::bind(&MainApp::generate_crop, this));

    
    
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
    mRectangle.mouseDown(event);
    
    mIsClicked = mRectangle.isClicked();
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
    ivec2    pixel( (int)( position.x * scaleX ), (int)( (position.y * scaleY ) ) );
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

std::string MainApp::resultInfo ()
{
    ostringstream oss( ostringstream::ate );
    oss << " Affine Prototype ";
    return oss.str();
    
}

void MainApp::mouseMove( MouseEvent event )
{
    mMousePos = event.getPos();
}


void MainApp::mouseDrag( MouseEvent event )
{
    mRectangle.mouseDrag(event);
    // Keep track of mouse position.
    mMousePos = event.getPos();
    mIsClicked = mRectangle.isClicked();
    
}


void MainApp::fileDrop( FileDropEvent event )
{
    try {
        mImage = loadImageFromPath (event.getFile( 0 ));
        if (mImage)
        {
            mRectangle = affineRectangle (Area(0.0,0.0,mImage->getWidth(), mImage->getHeight()), getWindowSize() / 2);
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
    vec2 one (0.05, 0.05);
    
    
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
            mRectangle.resize(vec2(-one.x,0));
            break;
            
        case KeyEvent::KEY_LEFTBRACKET:
            mRectangle.resize(vec2(one.x,0));
            break;
            
        case KeyEvent::KEY_SEMICOLON:
            mRectangle.resize(vec2(0,-one.y));
            break;
            
        case KeyEvent::KEY_QUOTE:
            mRectangle.resize(vec2(0,one.y));
            break;
            
        case KeyEvent::KEY_c:
            if (event.isMetaDown() )
                Clipboard::setImage( copyWindowSurface() );
            
            break;
            
        case KeyEvent::KEY_v:
            if (event.isMetaDown() && Clipboard::hasImage())
                
                try {
                    mImage = Surface::create( Clipboard::getImage() );
                    
                   mRectangle = affineRectangle (Area(0.0,0.0,mImage->getWidth(), mImage->getHeight()), getWindowSize() / 2);
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
    mTextures.resize(mNames.size());
    mOverlays.resize(mNames.size());
    textureToDisplay = &mTextures[0];
    
    cv::Mat hsv;
    cv::Mat mask;
    cv::Mat xyz;
    
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
    
    setWindowSize(mPadded.cols, mPadded.rows);
}



void MainApp::draw()
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
    
    // Not using the affine rect
    mRectangle.draw(getWindowSize());
//    drawEditableRect();
    
    
    // Draw the interface
    if(mParams)
        mParams->draw();
    
}

#if 0
void MainApp::drawEditableRect()
{
   // const std::vector<Point2f>& cvcorners =  mRectangle.windowCorners();
    Area outputArea = app::getWindowBounds();
    /*
     Area::proportionalFit( mTextureOrig.getBounds(),
     app::getWindowBounds(), true, true );
     */
    Rectf captureDrawRect = Rectf( outputArea );

    RectMapping n2SMapping( Rectf( 0, 0, 1, 1 ), captureDrawRect );
    
    // Either use setMatricesWindow() or setMatricesWindowPersp() to enable 2D rendering.
    gl::setMatricesWindow( getWindowSize(), true );
    
    // Draw the transformed rectangle.
    gl::pushModelMatrix();
//    gl::multModelMatrix( mRectangle.getWorldTransform() );
    
 
    auto norm_area = n2SMapping.map(mRectangle.norm_area());
    auto norm_position = n2SMapping.map(mRectangle.norm_position());
                                        
    // Draw a stroked rect in magenta (if mouse inside) or white (if mouse outside).
    {
        ColorA cl (1.0, 1.0, 1.0,0.8f);
        if (mIsOver) cl = ColorA (1.0, 0.0, 1.0,0.8f);
        gl::ScopedColor color (cl);
        gl::drawStrokedRect(norm_area);
    }
    
    
    gl::drawStrokedEllipse(norm_position, norm_area.getWidth()/2, norm_area.getHeight()/2);
    {
        gl::ScopedColor color (ColorA(1.0,0.0,0.0,0.8f));
        vec2 scale (1.0,1.0);
        {
            vec2 window = norm_position;
            gl::ScopedColor color (ColorA (0.0, 1.0, 0.0,0.8f));
            gl::drawLine( vec2( window.x, window.y - 5 * scale.y ), vec2( window.x, window.y + 5 * scale.y ) );
            gl::drawLine( vec2( window.x - 5 * scale.x, window.y ), vec2( window.x + 5 * scale.x, window.y ) );
        }
        
    }

    // Draw the 4 corners as green crosses.
    int i = 0;
    float dsize = 5.0f;
    for( const auto &corner : mRectangle.worldCorners()  ) {
        vec2 window = n2SMapping.map(corner);
        vec2 scale (1.0,1.0);
        {
            gl::ScopedColor color (ColorA (0.0, 1.0, 0.0,0.8f));
            gl::drawLine( vec2( window.x, window.y - 5 * scale.y ), vec2( window.x, window.y + 5 * scale.y ) );
            gl::drawLine( vec2( window.x - 5 * scale.x, window.y ), vec2( window.x + 5 * scale.x, window.y ) );
        }
        ColorA cl (1.0, 0.0, 0.0,0.8f);
        if (i==0 || i == 1)
            cl = ColorA (0.0, 0.0, 1.0,0.8f);
        {
            gl::ScopedColor color (cl);
            gl::drawStrokedCircle(window, dsize, dsize+5.0f);
        }
        dsize+=5.0f;
        i++;
    }
    
    std::cout << "affine Rect : " << mRectangle.degrees() << std::endl;

    gl::popModelMatrix();
    
}



glm::vec2 MainApp::screenToObject( const glm::vec2 &pt, float z ) const
{
    // Build the viewport (x, y, width, height).
    glm::vec2 offset = gl::getViewport().first;
    glm::vec2 size = gl::getViewport().second;
    glm::vec4 viewport = glm::vec4( offset.x, offset.y, size.x, size.y );
    
    // Calculate the view-projection matrix.
    glm::mat4 model = mRectangle.getWorldTransform();
    glm::mat4 viewProjection = gl::getProjectionMatrix() * gl::getViewMatrix();
    
    // Calculate the intersection of the mouse ray with the near (z=0) and far (z=1) planes.
    glm::vec3 near = glm::unProject( glm::vec3( pt.x, size.y - pt.y - 1, 0 ), model, viewProjection, viewport );
    glm::vec3 far = glm::unProject( glm::vec3( pt.x, size.y - pt.y - 1, 1 ), model, viewProjection, viewport );
    
    // Calculate world position.
    return glm::vec2( ci::lerp( near, far, ( z - near.z ) / ( far.z - near.z ) ) );
}


glm::vec2 MainApp::objectToScreen( const glm::vec2 &pt ) const
{
    // Build the viewport (x, y, width, height).
    glm::vec2 offset = gl::getViewport().first;
    glm::vec2 size = gl::getViewport().second;
    glm::vec4 viewport = glm::vec4( offset.x, offset.y, size.x, size.y );
    
    // Calculate the view-projection matrix.
    glm::mat4 model = mRectangle.getWorldTransform();
    glm::mat4 viewProjection = gl::getProjectionMatrix() * gl::getViewMatrix();
    
    glm::vec2 p = glm::vec2( glm::project( glm::vec3( pt, 0 ), model, viewProjection, viewport ) );
    p.y = size.y - 1 - p.y;
    
    return p;
}
#endif

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

