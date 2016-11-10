
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


  }

/////////////// Null Viewer ///////////////////////

const std::string & uContext::name() const
{
    static std::string sNullName = "nullViewer";
    return sNullName;
}
void uContext::name (const std::string& ) {}

void uContext::draw () {}
void uContext::update () {}
//    virtual void resize () = 0;
void uContext::setup () {}
bool uContext::is_valid () { return m_valid; }

uContext::uContext (ci::app::WindowRef window) : mWindow (window)
{
    mSelected = false;
    mWindow->setUserData(this);
    size_t m_unique_id = getNumWindows();
    mWindow->getSignalClose().connect([m_unique_id,this] { std::cout << "You closed window #" << m_unique_id << std::endl; });
    m_uniqueId = m_unique_id;
    
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
    
    // Browse for the result file
    mPath = getOpenFilePath();
    if (mPath.string().empty() || ! exists(mPath) )
    {
        std::cout << mPath.string() << " Does not exist Or User cancelled " << std::endl;
    }
    
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


void matContext::onMarked (marker_info& t)
{
    std::cout << " clip <- " << t.norm_time << std::endl;
}

bool matContext::is_valid ()
{
    return m_valid;
}




////////////////// ClipContext  ///////////////////


clipContext::clipContext(ci::app::WindowRef window)
{
    setup ();
    if (is_valid())
    {
        uContext(window);
        mCbMouseDown = mWindow->getSignalMouseDown().connect( std::bind( &clipContext::mouseDown, this, std::placeholders::_1 ) );
        mCbMouseDrag = mWindow->getSignalMouseDrag().connect( std::bind( &clipContext::mouseDrag, this, std::placeholders::_1 ) );
        mCbMouseUp = mWindow->getSignalMouseUp().connect( std::bind( &clipContext::mouseUp, this, std::placeholders::_1 ) );
        mCbMouseDrag = mWindow->getSignalMouseMove().connect( std::bind( &clipContext::mouseMove, this, std::placeholders::_1 ) );
        mCbKeyDown = mWindow->getSignalKeyDown().connect( std::bind( &clipContext::keyDown, this, std::placeholders::_1 ) );
        getWindow()->setTitle( mPath.filename().string() );
    }
    
}

void clipContext::draw ()
{
    mGraph1D->rect = getWindowBounds();
    if (mGraph1D) mGraph1D->draw ( );
}

void clipContext::onMarked ( marker_info_t& t)
{
    std::cout << " clip <- " << t.norm_time << std::endl;
}

void clipContext::update ()
{
}

bool clipContext::is_valid () { return m_valid; }

void clipContext::setup()
{
    mType = Type::clip_viewer;
    m_valid = false;
    normalize(true);
    
    // Browse for the result file
    mPath = getOpenFilePath();
    if (mPath.string().empty() || ! exists(mPath) )
    {
        std::cout << mPath.string() << " Does not exist Or User cancelled " << std::endl;
        return;
    }
    

    const std::string& fqfn = mPath.string ();
    m_columns = m_rows = 0;
    mdat.clear ();
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
    
    if (m_valid)
        loadAll(mdat);

}

void clipContext::loadAll (const  std::vector<vector<float> > & src)
{
    // Assumptions: Col 0 is X axis, Col 1,... are separate graphs
    // Normalize to 0 - 1

    mGraph1D = Graph1DRef (new graph1D ("signature", getWindowBounds() ) );
    mGraph1D->addListener( this, &clipContext::receivedEvent );
    
    for (unsigned ii = 1; ii < src.size(); ii++)
    {
        const std::vector<float>& coi = src[ii];
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
        }
        
        if (m_valid)
        {
            if (!normalize_option())
                mGraph1D->load_vector(coi);
            else if (coi.size() == out.size())
                mGraph1D->load_vector(out);
        }
    
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

