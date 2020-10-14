

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"

#include "cinder_cv/cinder_opencv.h"

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/params/Params.h"



using namespace ci;
using namespace ci::app;

class ocvWarpApp : public App {
  public:
	void setup() override;
	void draw() override;
    void update() override;

	void updateImage();

	ci::Surface			mInputImage;
	gl::TextureRef		mTexture;

	params::InterfaceGl	mParams;
	vec2				mRotationCenter;
	float				mRotationAngle, mScale;
    std::vector<vec2> m_points;
    std::vector<vec2> m_norm_points;
    cv::RotatedRect m_rrect;
};

void ocvWarpApp::setup()
{		
    mInputImage = ci::Surface8u( loadImage("/Users/arman/Pictures/affine_test.png" ) ) ;

	mRotationCenter = vec2( mInputImage.getSize() ) * 0.5f;
	mRotationAngle = 31.2f;
	mScale = 0.77f;
    vec2 sz(mInputImage.getWidth(),mInputImage.getHeight());
    
    m_points.emplace_back(218.959747/sz.x,60.3683815/sz.y);
    m_points.emplace_back(270.50528/sz.x,-37.9375763/sz.y);
    m_points.emplace_back(429.920319/sz.x,45.6497536/sz.y);
    m_points.emplace_back(378.374786/sz.x,143.955719/sz.y);


    auto get_trims = [] (const cv::Rect& box, const cv::RotatedRect& rotrect) {
        
        
        //The order is bottomLeft, topLeft, topRight, bottomRight.
        std::vector<float> trims(4);
        cv::Point2f rect_points[4];
        rotrect.points( &rect_points[0] );
        cv::Point2f xtl; xtl.x = box.tl().x; xtl.y = box.tl().y;
        cv::Point2f xbr; xbr.x = box.br().x; xbr.y = box.br().y;
        cv::Point2f dtl (xtl.x - rect_points[1].x, xtl.y - rect_points[1].y);
        cv::Point2f dbr (xbr.x - rect_points[3].x, xbr.y - rect_points[3].y);
        trims[0] = rect_points[1].x < xtl.x ? xtl.x - rect_points[1].x : 0.0f;
        trims[1] = rect_points[1].y < xtl.y ? xtl.y - rect_points[1].y : 0.0f;
        trims[2] = rect_points[3].x > xbr.x ? rect_points[3].x  - xbr.x : 0.0f;
        trims[3] = rect_points[3].x > xbr.y ? rect_points[3].y  - xbr.y : 0.0f;
        std::transform(trims.begin(), trims.end(), trims.begin(),
                       [](float d) -> float { return std::roundf(d); });
        
        return trims;
    };

    m_rrect = cv::RotatedRect(
    auto trims = get_trims(cv::Rect(0,0,mInputImage.getWidth(), mInputImage.getHeight()), m_motion_mass);
    for (auto ff : trims) std::cout << ff << std::endl;
    
    cv::Point offset(trims[0],trims[1]);
    fPair pads (trims[0]+trims[2],trims[1]+trims[3]);
    cv::Mat canvas (m_temporal_ss.rows+pads.second, m_temporal_ss.cols+pads.first, CV_8U);
    canvas.setTo(0.0);
    cv::Mat win (canvas, cv::Rect(offset.x, offset.y, m_temporal_ss.cols, m_temporal_ss.rows));
    m_temporal_ss.copyTo(win);
	mParams = params::InterfaceGl( "Parameters", ivec2( 200, 400 ) );
	mParams.addParam( "Rotation Center X", &mRotationCenter.x );
	mParams.addParam( "Rotation Center Y", &mRotationCenter.y );
	mParams.addParam( "Rotation Angle", &mRotationAngle );
	mParams.addParam( "Scale", &mScale, "step=0.01" );

	updateImage();
}

void ocvWarpApp::updateImage()
{
	cv::Mat input( toOcv( mInputImage ) );
	cv::Mat output;

	cv::Mat warpMatrix = cv::getRotationMatrix2D( toOcv( mRotationCenter ), mRotationAngle, mScale );
	cv::warpAffine( input, output, warpMatrix, toOcv( getWindowSize() ), cv::INTER_CUBIC );

	mTexture = gl::Texture::create( fromOcv( input ) );
    m_norm_points.clear ();
    vec2 ws (getWindowWidth(), getWindowHeight());
    for (auto pt : m_points){
        m_norm_points.emplace_back(pt.x * ws.x, pt.y * ws.y);
    }
}

void ocvWarpApp::update()
{
	updateImage();
}

void ocvWarpApp::draw()
{
	gl::clear( Color( 1, 1, 1 ) );
	
	gl::color( Color( 1, 1, 1 ) );
	updateImage();
    
	gl::draw( mTexture ,  getWindowBounds() );
	
	gl::color( Color( 1, 0, 0 ) );
    for (auto pt : m_norm_points){
        gl::drawSolidCircle( pt , 4.0f );
    }

	
	mParams.draw();
}


CINDER_APP( ocvWarpApp, RendererGl )

#pragma GCC diagnostic pop
