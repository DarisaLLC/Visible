
#include "guiContext.h"
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
#include "VisibleApp.h"
#include "app_utils.hpp"


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
    
}

guiContext::guiContext (ci::app::WindowRef& ww)
: mWindow (ww), m_valid (false), m_type (null_viewer)
{
    if (ww)
        ww->setUserData (this);
}

guiContextRef guiContext::getRef ()
{
    return shared_from_this();
}

guiContext::~guiContext ()
{
   // std::cout << " guiContext Dtor called " << std::endl;
}

// u implementation does nothing
void guiContext::mouseDown( MouseEvent event ) {}
void guiContext::mouseMove( MouseEvent event ) {}
void guiContext::mouseUp( MouseEvent event ) {}
void guiContext::mouseDrag( MouseEvent event ) {}
void guiContext::keyDown( KeyEvent event ) {}

void guiContext::normalize_point (vec2& pos, const ivec2& size)
{
    pos.x = pos.x / size.x;
    pos.y = pos.y / size.y;
}


const std::string & guiContext::getName() const { return mName; }
void guiContext::setName (const std::string& name) { mName = name; }
guiContext::Type guiContext::context_type () const { return m_type; }
bool guiContext::is_context_type (const Type t) const { return m_type == t; }
bool guiContext::is_valid () const { return m_valid; }


///////////////  Main Viewer ////////////////////

mainContext::mainContext(ci::app::WindowRef& ww, const boost::filesystem::path& dp)
: guiContext (ww)
{
    m_valid = false;
    m_type = Type::null_viewer;
    
}


////////////////////////////////////////////////////



///////////////  Matrix Viewer ////////////////////

matContext::matContext(ci::app::WindowRef& ww, const boost::filesystem::path& dp)
: guiContext (ww), mPath (dp)
{
    m_valid = false;
    m_type = Type::matrix_viewer;
    
    setup ();
    if (is_valid())
    {
        mWindow->setTitle (matContext::caption ());
    }
}

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
    if (! mPath.empty ())
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

void matContext::resize()
{
    
}


void matContext::onMarked (marker_info& t)
{
    std::cout << " clip <- " << t.norm_time << std::endl;
}

bool matContext::is_valid ()
{
    return m_valid && is_context_type (guiContext::Type::matrix_viewer);
}




////////////////// ClipContext  ///////////////////


clipContext::clipContext(ci::app::WindowRef& ww, const boost::filesystem::path& dp)
: guiContext (ww), mPath (dp)
{
    m_valid = false;
    m_type = Type::clip_viewer;
    
    if (mPath.string().empty())
        mPath = getOpenFilePath();
    
    m_valid = ! mPath.string().empty() && exists(mPath);
    
    setup ();
    if (is_valid())
    {
        mWindow->setTitle( mPath.filename().string() );
    }
    
}

void clipContext::resize ()
{
    mGraph1D->rect = getWindowBounds();
}
void clipContext::draw ()
{
    if (mGraph1D) mGraph1D->draw ( );
}

void clipContext::onMarked ( marker_info_t& t)
{
    if (mGraph1D)
    {
        mGraph1D->set_marker_position ( t );
        mGraph1D->draw ( );
    }
}

void clipContext::update ()
{
}

bool clipContext::is_valid () { return m_valid && is_context_type(guiContext::clip_viewer); }

void clipContext::setup()
{
    m_type = Type::clip_viewer;
    m_valid = false;
    normalize(true);
    
    const std::string& fqfn = mPath.string ();
    m_columns = m_rows = 0;
    mdat.clear ();
    bool c2v_ok = csv::csv2vectors ( fqfn, mdat, false, true, true);
    if ( c2v_ok )
    {
        m_columns = mdat.size ();
        m_rows = mdat[0].size();
        m_frames = m_file_frames = m_rows;
        m_read_pos = 0;
        m_valid = true;
    }
    
    if (m_valid)
        loadAll(mdat);
    
}

// Only creates on 1D graph
void clipContext::loadAll (const  std::vector<vector<float> > & src)
{
    if (! m_valid) return;
    
    // Assumptions: Col 0 is X axis, Col 1,... are separate graphs
    // Normalize to 0 - 1
    // Create the first column
    mGraph1D = Graph1DRef (new graph1D ("signature", getWindowBounds() ) );
    mGraph1D->addListener( this, &clipContext::receivedEvent );
    
    const std::vector<float>& coi = src[0];
    std::vector<float> out;
    
    if (normalize_option ())
    {
        std::pair<std::vector<float>::const_iterator, std::vector<float>::const_iterator> p =
        boost::minmax_element(coi.begin(), coi.end());
        std::pair<float,float> minmax_val (*p.first, *p.second);
        out.resize (coi.size());
        
        std::vector<float>::const_iterator di = coi.begin();
        std::vector<float>::iterator dout = out.begin();
        float scale = minmax_val.second - minmax_val.first;
        for (; di < coi.end(); di++, dout++)
        {
            *dout = (*di - minmax_val.first) / scale;
        }
        mGraph1D->setup(out);
    }
    else
    {
        mGraph1D->setup(coi);
    }
    
    
    //    mClipParams = params::InterfaceGl (" Clip ", vec2( 200, 400) );
    
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
    if (mGraph1D)
    {
        // Call base mouse move to update
        mGraph1D->mouseMove( event );
        marker_info tt;
        mGraph1D->get_marker_position(tt);
        tt.et = marker_info::event_type::move;
        signalMarker.emit(tt);
        
    }
}


void clipContext::mouseDrag( MouseEvent event )
{
    if (mGraph1D) mGraph1D->mouseDrag( event );
}


void clipContext::mouseDown( MouseEvent event )
{
    if (mGraph1D)
    {
        mGraph1D->mouseDown( event );
        marker_info tt;
        mGraph1D->get_marker_position(tt);
        tt.et = marker_info::event_type::down;
        signalMarker.emit(tt);
    }
}


void clipContext::mouseUp( MouseEvent event )
{
    if (mGraph1D) mGraph1D->mouseUp( event );
}

