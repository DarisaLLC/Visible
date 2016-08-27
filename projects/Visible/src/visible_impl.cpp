
#include "ui_contexts.h"
#include "stl_util.hpp"
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Timeline.h"
#include "cinder/Timer.h"
#include "cinder/Camera.h"
#include "cinder/qtime/Quicktime.h"
#include "cinder/params/Params.h"
#include "cinder/ImageIo.h"
#include <boost/algorithm/minmax.hpp>
#include <boost/algorithm/minmax_element.hpp>


using namespace boost;


namespace
{

    std::ostream& ci_console ()
    {
        return AppBase::get()->console();
    }
    
    double				ci_getElapsedSeconds()
    {
        return AppBase::get()->getElapsedSeconds();
    }
    
    
#if 0
    void copy_to_vector (const ci::audio2::BufferRef &buffer, std::vector<float>& mBuffer)
    {
        mBuffer.clear ();
        const float *reader = buffer->getChannel( 0 );
        for( size_t i = 0; i < buffer->getNumFrames(); i++ )
            mBuffer.push_back (*reader++);
    }
    size_t Wave2Index (const Rectf& box, const size_t& pos, const Waveform& wave)
    {
        size_t xScaled = (pos * wave.sections()) / box.getWidth();
        xScaled *= wave.section_size ();
        xScaled = math<size_t>::clamp( xScaled, 0, wave.samples () );
    }
    
#endif

  }


///////////////  Matrix Viewer ////////////////////

void matContext::mouseDrag( MouseEvent event )
{
    mCamUi.mouseDrag(event);
}


void matContext::mouseDown( MouseEvent event )
{
    mCamUi.mouseDown(event);
}


void matContext::setup ()
{
    gl::enableAlphaBlending();
    gl::enableDepthRead();
    gl::enableDepthWrite();
    
    mCamUi = CameraUi( &mCam, getWindow() );
    mCam.setNearClip( 10 );
    mCam.setFarClip( 2000 );
    
    // Browse for a matrix file
    mPath = getOpenFilePath();
    if(! mPath.empty() ) internal_setupmat_from_file(mPath);
}

void matContext::internal_setupmat_from_file (const boost::filesystem::path & fp)
{
    csv::matf_t mat;
    csv::csv2vectors(fp.string(), mat, false, false, true);
    if (csv::validate_matf (mat) ) internal_setupmat_from_mat (mat);
}

void matContext::internal_setupmat_from_mat (const csv::matf_t & mat)
{
    m_valid = false;
    size_t dL = mat.size ();
    int numPixels = dL * dL;

    mPointCloud = gl::VboMesh::create( numPixels, GL_POINTS, { gl::VboMesh::Layout().usage(GL_STATIC_DRAW).attrib(geom::POSITION, 3).attrib(geom::COLOR, 3) } );
    
    mPointsBatch = gl::Batch::create( mPointCloud, gl::getStockShader( gl::ShaderDef().color() ) );
    
    auto vertPosIter = mPointCloud->mapAttrib3f( geom::POSITION );
    auto vertColorIter = mPointCloud->mapAttrib3f( geom::COLOR );
    
    size_t my = 0;
    while (my < dL)
    {
        const csv::rowf_t& rowf = mat[my];
        const float* elm_ptr = &rowf[0];
        size_t mx = 0;
        while (mx < dL)
        {
            float val = *elm_ptr++;
            Color color( val, 0, (1 - val) );
            *vertPosIter++ = vec3(static_cast<float>(mx),static_cast<float>(dL - my), val * 100 );
            *vertColorIter++ = vec3( color.r, color.g, color.b );
            mx++;
        }
        my++;
    }

    mCam.lookAt( vec3( dL / 2, 50, dL / 2 ), vec3( 0 ) );
    m_valid = true;
}

void matContext::draw()
{
    if (! m_valid ) return;
    gl::clear();
    
    gl::setMatrices( mCam );
    if( mPointsBatch )
        mPointsBatch->draw();
}


void matContext::update()
{
    
}


bool matContext::is_valid ()
{
    return m_valid;
}




////////////////// ClipContext  ///////////////////



void clipContext::draw ()
{
    mGraph1D->rect = getWindowBounds();
    // time cycles every 1 / TWEEN_SPEED seconds, with a 50% pause at the end
    float time = svl::math<float>::clamp( fmod( ci_getElapsedSeconds() * 0.5, 1 ) * 1.5f, 0, 1 );
    if (mGraph1D) mGraph1D->draw ( time );
}



void clipContext::update ()
{
}

bool clipContext::is_valid () { return m_valid; }

void clipContext::setup()
{
    // Browse for the result file
    mPath = getOpenFilePath();
    m_valid = false;
  	getWindow()->setTitle( mPath.filename().string() );
    const std::string& fqfn = mPath.string ();
    m_columns = m_rows = 0;
    mdat.resize (0);
    bool c2v_ok = csv::csv2vectors ( fqfn, mdat, false, true, true);
    if ( c2v_ok )
        m_columns = mdat.size ();
    
      m_rows = mdat[0].size();
    // only handle case of long columns of data
    m_column_select = 1;
    if (m_columns < m_rows)
    {
        m_column_select = (m_column_select >= 0 && m_column_select < m_columns) ? m_column_select : 0;
        m_frames = m_file_frames = m_rows;
        m_read_pos = 0;
        m_valid = true;
    }
    
    loadAll(mdat[0]);

}

void clipContext::loadAll (const std::vector<float>& src)
{
    // Normalize to 0 - 1
    std::pair<std::vector<float>::const_iterator, std::vector<float>::const_iterator> p =
    boost::minmax_element(src.begin(), src.end());
    std::pair<float,float> minmax_val (*p.first, *p.second);
    std::vector<float> out (src.size());
    
    std::vector<float>::const_iterator di = src.begin();
    std::vector<float>::iterator dout = out.begin();
    float scale = minmax_val.second - minmax_val.first;
    for (; di < src.end(); di++, dout++)
    {
        *dout = (*di - minmax_val.first) / scale;
    }
    
    if (m_valid)
    {
        mGraph1D = Graph1DRef (new graph1D ("signature", getWindowBounds() ) );
        mGraph1D->addListener( this, &clipContext::receivedEvent );
        mGraph1D->load_vector(out);

    }
    
    
    mClipParams = params::InterfaceGl (" Clip ", vec2( 200, 400) );

    //        string max = ci::toString( m_movie.getDuration() );
    //        mMovieParams.addParam( "Column", &m_column_select, "min=0 max=" + columns );
}


void clipContext::receivedEvent( InteractiveObjectEvent event )
{
    string text;
    switch( event.type )
    {
        case InteractiveObjectEvent::Pressed:
            text = "Pressed event";
            break;
        case InteractiveObjectEvent::PressedOutside:
            text = "Pressed outside event";
            break;
        case InteractiveObjectEvent::Released:
            text = "Released event";
            break;
        case InteractiveObjectEvent::ReleasedOutside:
            text = "Released outside event";
            break;
        case InteractiveObjectEvent::RolledOver:
            text = "RolledOver event";
            break;
        case InteractiveObjectEvent::RolledOut:
            text = "RolledOut event";
            break;
        case InteractiveObjectEvent::Dragged:
            text = "Dragged event";
            break;
        default:
            text = "Unknown event";
    }
    ci_console() << "Received " + text << endl;
}


void clipContext::mouseMove( MouseEvent event )
{
   if (mGraph1D) mGraph1D->mouseMove( event );
}


void clipContext::mouseDrag( MouseEvent event )
{
   if (mGraph1D) mGraph1D->mouseDrag( event );
}


void clipContext::mouseDown( MouseEvent event )
{
   if (mGraph1D) mGraph1D->mouseDown( event );
}


void clipContext::mouseUp( MouseEvent event )
{
   if (mGraph1D) mGraph1D->mouseUp( event );
}

