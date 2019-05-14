#include "cinder/cinder.h"
#include "cinder_opencv.h"
#include "cinder/Quaternion.h"

#undef near
#undef far

using namespace ci;
using namespace ci::app;
using namespace std;


//! A rectangle that can be transformed.
struct EditableRect {
    Area  area;
    vec2  position;
    vec2  scale;
    quat  rotation;
    
    EditableRect() : scale( 1 ) {}
    ~EditableRect() {}
    
    //! Returns the rectangle's model matrix.
    mat4  matrix()
    {
        mat4 m = glm::translate( vec3( position, 0 ) );
        m *= glm::toMat4( rotation );
        m *= glm::scale( vec3( scale, 1 ) );
        m *= glm::translate( vec3( -area.getSize() / 2, 0 ) );
        
        return m;
    }
};


class affineRectangle {
public:
    affineRectangle () {}
    affineRectangle (const Area& bounds, const ivec2& screen_position);
    void draw (const ivec2& window_size);
    
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void resize( const vec2 change );
    
    vec3 mouseToWorld( const ivec2 &mouse, float z = 0 );
    
    Area area () const { return mRectangle.area; }
    bool isClicked () const { return mIsClicked; }
    
    const vec2&   position () const { return mRectangle.position; }
    void   position (const vec2& np) { mRectangle.position = np; }
    float  degrees () const
    {
        return toDegrees(2*std::acos(mRectangle.rotation.w));
    }
    
    float  radians () const
    {
        return 2*std::acos(mRectangle.rotation.w);
    }

//    const vec2&   scale () const;
//    const quat   rotation ()const;
//    float degrees () const;
//    float radians () const;
    
private:
    EditableRect   mRectangle;
    
    ivec2          mMouseInitial;
    EditableRect   mRectangleInitial;
    
    bool           mIsClicked;
};

affineRectangle::affineRectangle (const ci::Area& bounds, const ivec2& screen_position)
{
    mRectangle.area = bounds;
    mRectangle.position = screen_position;
    
    mIsClicked = false;
}

void affineRectangle::draw(const ivec2& window_size)
{
    // Either use setMatricesWindow() or setMatricesWindowPersp() to enable 2D rendering.
    gl::setMatricesWindow( window_size, true );

    // Draw the transformed rectangle.
    gl::pushModelMatrix();
    gl::multModelMatrix( mRectangle.matrix() );
    gl::drawStrokedRect( mRectangle.area );
    gl::popModelMatrix();
}

void affineRectangle::mouseDown( MouseEvent event )
{
    // Check if mouse is inside rectangle, by converting the mouse coordinates
    // to world space and then to object space.
    vec3 world = mouseToWorld( event.getPos() );
    vec4 object = glm::inverse( mRectangle.matrix() ) * vec4( world, 1 );
    
    // Now we can simply use Area::contains() to find out if the mouse is inside.
    mIsClicked = mRectangle.area.contains( vec2( object ) );
    if( mIsClicked ) {
        mMouseInitial = event.getPos();
        mRectangleInitial = mRectangle;
    }
}

void affineRectangle::resize( const vec2 change )
{
    mRectangle.scale += change;
}

void affineRectangle::mouseDrag( MouseEvent event )
{
    // Scale and rotate the rectangle.
    if( mIsClicked ) {
        // Calculate the initial click position and the current mouse position, in world coordinates relative to the rectangle's center.
        vec3 initial = mouseToWorld( mMouseInitial ) - vec3( mRectangle.position, 0 );
        vec3 current = mouseToWorld( event.getPos() ) - vec3( mRectangle.position, 0 );
        
        // Calculate scale by using the distance to the center of the rectangle.
        float d0 = glm::length( initial );
        float d1 = glm::length( current );
        mRectangle.scale = vec2( mRectangleInitial.scale * ( d1 / d0 ) );
        
        // Calculate rotation by taking the angle with the X-axis for both positions and calculating the difference.
        float a0 = math<float>::atan2( initial.y, initial.x );
        float a1 = math<float>::atan2( current.y, current.x );
        mRectangle.rotation = mRectangleInitial.rotation * glm::angleAxis( a1 - a0, vec3( 0, 0, 1 ) );
    }
}

vec3 affineRectangle::mouseToWorld( const ivec2 &mouse, float z )
{
    // Build the viewport (x, y, width, height).
    vec2 offset = gl::getViewport().first;
    vec2 size = gl::getViewport().second;
    vec4 viewport = vec4( offset.x, offset.y, size.x, size.y );
    
    // Calculate the view-projection matrix.
    mat4 transform = gl::getProjectionMatrix() * gl::getViewMatrix();
    
    // Calculate the intersection of the mouse ray with the near (z=0) and far (z=1) planes.
    vec3 near = glm::unProject( vec3( mouse.x, size.y - mouse.y, 0 ), mat4(), transform, viewport );
    vec3 far = glm::unProject( vec3( mouse.x, size.y - mouse.y, 1 ), mat4(), transform, viewport );
    
    // Calculate world position.
    return ci::lerp( near, far, ( z - near.z ) / ( far.z - near.z ) );
}
