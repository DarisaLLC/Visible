#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Batch.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class GeometryShaderIntroApp : public App {
  public:
	void setup() override;
    void mouseDrag( MouseEvent event ) override;
    void mouseDown( MouseEvent event ) override;
    void keyDown( KeyEvent event ) override;
	void draw() override;
    void resize() override;
    void update() override;
    
    void setUniforms();
    
    CameraPersp mCam;
    
    ci::gl::UboRef				mUboDataWindow;
    std::vector<float>         mDataWindow;
    
	gl::BatchRef	mBatch;
	gl::GlslProgRef		mGlsl;
	int32_t					mDataIndex;
	float				mDataValue;
    
    float              mGlobalTime;
    vec3                mResolution;
    vec4                mMouse;
    
    mat4 calcViewportMatrix()
    {
        auto curViewport = cinder::gl::getViewport();
        
        const float a = ( curViewport.second.x - curViewport.first.x ) / 2.0f;
        const float b = ( curViewport.second.y - curViewport.first.y ) / 2.0f;
        const float c = 1.0f / 2.0f;
        
        const float tx = ( curViewport.second.x + curViewport.first.x ) / 2.0f;
        const float ty = ( curViewport.second.y + curViewport.second.y ) / 2.0f;
        const float tz = 1.0f / 2.0f;
        
        return mat4(
                    a, 0, 0, 0,
                    0, b, 0, 0,
                    0, 0, c, 0,
                    tx, ty, tz, 1
                    );
    }
    
    
    
};


struct syn_signal_generator
{
    syn_signal_generator (float start_time = 0.0, float step_time = 0.1)
    {
        m_start_time = start_time;
        m_step_time = step_time;
        m_current_time = m_step_time;
    }
    
    float operator ()(float x)
    {
        float tmp = m_current_time;
        m_current_time += m_step_time;
        return 0.3 * sin(x * 9.0 + tmp * 9.0) / (0.41 *(0.1 + abs(x))) * 1.0;
    }
    
    float m_start_time;
    float m_step_time;
    float m_current_time;
};


void GeometryShaderIntroApp::setup()
{
    syn_signal_generator sg (0.0, 10.0);
    
    mDataWindow.resize (1024);
    
    for (auto idx = 0; idx < mDataWindow.size(); idx++)
        mDataWindow[idx] = sg((float) (idx+1));
    

    mUboDataWindow = gl::Ubo::create (sizeof(float) * mDataWindow.size(), mDataWindow.data());
    mUboDataWindow->bindBufferBase(0);
    
    mCam.setPerspective( 60, getWindowAspectRatio(), 1, 10000 );
    mCam.lookAt( vec3( 0, 0, -1 ), vec3( 0, 0, 1 ) );
    
	// setup shader
	try {
		mGlsl = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "basic.vert"))
                                     .fragment( loadAsset( "oneplot.frag")));
	}
	catch( gl::GlslProgCompileExc ex ) {
		cout << ex.what() << endl;
		quit();
	}

  //  mGlsl->uniformBlock("DataWindow", 0);
  //  mBatch = gl::Batch::create( geom::Rect(), mGlsl );

    
    mDataIndex  = 0;
    mDataValue = mDataWindow[mDataIndex];
    
   
    gl::enableDepthWrite();
    gl::enableDepthRead();
}

void GeometryShaderIntroApp::mouseDown( MouseEvent event )
{
    mMouse.x = (float)event.getPos().x;
    mMouse.y = (float)event.getPos().y;
    mMouse.z = (float)event.getPos().x;
    mMouse.w = (float)event.getPos().y;
}

void GeometryShaderIntroApp::mouseDrag( MouseEvent event )
{
    mMouse.x = (float)event.getPos().x;
    mMouse.y = (float)event.getPos().y;
}

void GeometryShaderIntroApp::keyDown( KeyEvent event )
{
    switch( event.getCode() ) {
        case KeyEvent::KEY_ESCAPE:
            quit();
            break;
        case KeyEvent::KEY_SPACE:
            random();
            break;
    }
}

void GeometryShaderIntroApp::setUniforms()
{
    auto shader = gl::context()->getGlslProg();
    if( !shader )
        return;
    
    // Calculate shader parameters.
    vec3  iResolution( vec2( getWindowSize() ), 1 );
    float iGlobalTime = (float)getElapsedSeconds();
    
    time_t now = time( 0 );
    tm*    t = gmtime( &now );
    vec4   iDate( float( t->tm_year + 1900 ),
                 float( t->tm_mon + 1 ),
                 float( t->tm_mday ),
                 float( t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec ) );
    
    // Set shader uniforms.
    shader->uniform( "iResolution", iResolution );
    shader->uniform( "iGlobalTime", iGlobalTime );
    shader->uniform( "iMouse", mMouse );
//    shader->uniform( "iDate", iDate );
    
//    const int32_t di = mDataIndex;
//    const float df = mDataValue;
//    shader->uniform( "uDataValue", df);
//    shader->uniform( "uDataIndex", di);

}

void GeometryShaderIntroApp::resize()
{
    mCam.setPerspective(60, getWindowAspectRatio(), 1, 1000);
    gl::setMatrices(mCam);
}
void GeometryShaderIntroApp::update()
{
//    mDataIndex ++;
//    mDataIndex = mDataIndex % mDataWindow.size();
//    mDataValue = mDataWindow[mDataIndex];
}



void GeometryShaderIntroApp::draw()
{
	gl::ScopedGlslProg glslProg( mGlsl );
    setUniforms();
    gl::clear( Color( 0, 0, 0 ) );
    gl::drawSolidRect( Rectf( 0, (float)getWindowHeight(), (float)getWindowWidth(), 0 ) );
//    gl::setMatrices(mCam);
//	mBatch->draw();
}

CINDER_APP( GeometryShaderIntroApp, RendererGl ( RendererGl::Options().version( 3, 3 ) ))
