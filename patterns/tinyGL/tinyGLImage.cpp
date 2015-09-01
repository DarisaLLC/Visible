#include "tinyGLImage.h"
#include <cstdio>
#include <mutex>
#include <memory>
#include <iostream>
#include "vision/roiWindow.h"
#include "drawUtils.hpp"
using namespace std;
using namespace tinyGL;
using namespace DS;


static void crossproduct(
                         double ax, double ay, double az,
                         double bx, double by, double bz,
                         double *rx, double *ry, double *rz )
{
    *rx = ay*bz - az*by;
    *ry = az*bx - ax*bz;
    *rz = ax*by - ay*bx;
}

static void crossproduct_v(
                           double const * const a,
                           double const * const b,
                           double * const c )
{
    crossproduct(
                 a[0], a[1], a[2],
                 b[0], b[1], b[2],
                 c, c+1, c+2 );
}

static double scalarproduct(
                            double ax, double ay, double az,
                            double bx, double by, double bz )
{
    return ax*bx + ay*by + az*bz;
}

static double scalarproduct_v(
                              double const * const a,
                              double const * const b )
{
    return scalarproduct(
                         a[0], a[1], a[2],
                         b[0], b[1], b[2] );
}

static double length(
                     double ax, double ay, double az )
{
    return sqrt(
                scalarproduct(
                              ax, ay, az,
                              ax, ay, az ) );
}

static double length_v( double const * const a )
{
    return sqrt( scalarproduct_v(a, a) );
}

static void normalize(
                      double *x, double *y, double *z)
{
    double const k = 1./length(*x, *y, *z);
    
    *x *= k;
    *y *= k;
    *z *= k;
}

static void normalize_v( double *a )
{
    double const k = 1./length_v(a);
    a[0] *= k;
    a[1] *= k;
    a[2] *= k;
}

/* == annotation drawing functions == */

void draw_strokestring(void *font, float const size, char const *string)
{
    glPushMatrix();
    float const scale = size * 0.01; /* GLUT character base size is 100 units */
    glScalef(scale, scale, scale);
    
    char const *c = string;
    for(; c && *c; c++) {
        glutStrokeCharacter(font, *c);
    }
    glPopMatrix();
}

template <typename P>
DisplayImage<P>::DisplayImage(int posX, int posY, int width, int height, const char * title, roiWindow<P> & image)
: Display(posX, posY, width, height, title)
, mImage(image)
, mWindowRatio(static_cast<double>(width) / static_cast<double>(height))
, mImageRatio(1.0)
, mMaxX(0.0)
, mMaxY(0.0)
, mImageTexture(0)
{
    mImageGLformat = PixelType<P>::data_type();
    mInternalGLformat = PixelType<P>::display_type();
    mInvWidth = 1.0 / DisplayImage<P>::mImage.width();
    mInvHeight = 1.0 / DisplayImage<P>::mImage.height();
    Identity_4x4(mS);
    Identity_4x4(mT);
    m_options_dict["Edges"] = 0;
    m_options_dict["Segment Centers"] = 1;
    m_options_dict["Index Locations"] = 2;
    m_options.reset();
    
}

template <typename P>
DisplayImage<P>::DisplayImage(const iRect display_box, const char * title, roiWindow<P> & image)
: Display(display_box, title)
, mImage(image)
, mWindowRatio(static_cast<double>(width () ) / static_cast<double>(height ()))
, mImageRatio(1.0)
, mMaxX(0.0)
, mMaxY(0.0)
, mImageTexture(0)
{
    mImageGLformat = PixelType<P>::data_type();
    mInternalGLformat = PixelType<P>::display_type();
    mInvWidth = 1.0 / DisplayImage<P>::mImage.width();
    mInvHeight = 1.0 / DisplayImage<P>::mImage.height();
    Identity_4x4(mS);
    Identity_4x4(mT);
    m_options_dict["Edges"] = 0;
    m_options_dict["Segment Centers"] = 1;
    m_options_dict["Index Locations"] = 2;
    m_options.reset();
    
}

template<typename P>
void DisplayImage<P>::image2display(fPair& point)
{
    point.first = mMaxX * (2 * (point.x() + 0.5f) * mInvWidth - 1);
    point.second = mMaxY * (1 - 2 * (point.y() + 0.5f) * mInvHeight);
}

template<typename P>
void DisplayImage<P>::image2display(fVector_2d& point)
{
    point.x(mMaxX * (2 * (point.x() + 0.5f) * mInvWidth - 1));
    point.y(mMaxY * (1 - 2 * (point.y() + 0.5f) * mInvHeight));
}


template<typename P>
void DisplayImage<P>::image2display(fRect& rect)
{
    fPair ul = rect.ul();
    fPair lr = rect.lr();
    image2display(ul);
    image2display(lr);
    rect = fRect (ul, lr);
}

template <typename P>
DisplayImage<P>::~DisplayImage()
{
    glDeleteTextures(1, &mImageTexture);
}

inline int
pow2roundup (int x)
{
    if (x < 0)
        return 0;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x+1;
}

template <typename P>
void DisplayImage<P>::init()
{
    glClearColor(kBackgroundGray, kBackgroundGray, kBackgroundGray, 1.0);
    glDisable(GL_CULL_FACE);
    glGenTextures(1, &mImageTexture);
    glBindTexture(GL_TEXTURE_2D, mImageTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    glPixelStoref(GL_UNPACK_ROW_LENGTH, mImage.rowPixelUpdate());
    glTexImage2D(GL_TEXTURE_2D, 0, mInternalGLformat, mImage.width(), mImage.height(), 0, mInternalGLformat, mImageGLformat,  mImage.rowPointer(0));
    
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (doDrawData())
    {
        glPointSize(2.0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
}

template <typename P>
void DisplayImage<P>::drawImage()
{
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mImageRatio = mImage.aspect();
    // After first use SubImage
    glPixelStoref(GL_UNPACK_ROW_LENGTH, mImage.rowPixelUpdate());
    // GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mImage.width(), mImage.height(), mInternalGLformat, mImageGLformat,  mImage.rowPointer(0));
    if (mWindowRatio >= mImageRatio)
    {
        mMaxX = mImageRatio / mWindowRatio;
        mMaxY = 1.0;
    }
    else
    {
        mMaxX = 1.0;
        mMaxY = mWindowRatio / mImageRatio;
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMultMatrixd(mS);
    glMultMatrixd(mT);
    glColor4ub(255, 255, 255, 255);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2d(0, 1);
    glVertex2d(-mMaxX, -mMaxY);
    glTexCoord2d(0, 0);
    glVertex2d(-mMaxX, mMaxY);
    glTexCoord2d(1, 0);
    glVertex2d(mMaxX, mMaxY);
    glTexCoord2d(1, 1);
    glVertex2d(mMaxX, -mMaxY);
    glEnd();
    
}

template <typename P>
void DisplayImage<P>::display()
{
    drawImage();
    gl_cross(0.0f, 0.0f, 1.1);
    if (doDrawData())
        drawData();
    
    Display::display();
}

template <typename P>
void DisplayImage<P>::reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    mWindowRatio = static_cast<double>(width) / static_cast<double>(height);
    Display::reshape(width, height);
}

template <typename P>
void DisplayImage<P>::keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 'o':
            mS[0] = mS[5] = 1.0;
            mT[12] = mT[13] = 0.0;
            break;
        case '+':
            mS[0] *= 1.1;
            mS[5] *= 1.1;
            break;
        case '-':
            mS[0] /= 1.1;
            mS[5] /= 1.1;
            break;
        case 'e':
            m_options[m_options_dict["Edges"]] = ! m_options[m_options_dict["Edges"]];
            break;
        case 's':
            m_options[m_options_dict["Segment Centers"]] = ! m_options[m_options_dict["Segment Centers"]];
            break;
        case 'i':
            m_options[m_options_dict["Index Locations"]] = ! m_options[m_options_dict["Index Locations"]];
            break;
        case 'q':
            exit(EXIT_SUCCESS);
            break;
            
            
    }
    Display::keyboard(key, x, y);
}

template <typename P>
void DisplayImage<P>::special(int key, int x, int y)
{
    const double speed = 0.1 / mS[0];
    switch (key)
    {
        case GLUT_KEY_LEFT:
            mT[12] += speed;
            break;
        case GLUT_KEY_RIGHT:
            mT[12] -= speed;
            break;
        case GLUT_KEY_DOWN:
            mT[13] += speed;
            break;
        case GLUT_KEY_UP:
            mT[13] -= speed;
            break;
    }
    Display::special(key, x, y);
}

template <typename P>
void DisplayImage<P>::motion(int x, int y)
{
    //  std::cout << x << "," << y << std::endl;
    Display::motion(x, y);
}

template <typename P>
void DisplayImage<P>::passiveMotion(int x, int y)
{
    
    //    std::cout << ((x - width()/2) - mMaxX) / (2*mMaxX) << "," << ((y - height()/2) -mMaxY)/ (2*mMaxY) << std::endl;
    //    fPair nm (x,y);
    //    norm(nm);
    //    std::cout << nm.x() << " x " << nm.y() << std::endl;
    Display::motion(x, y);
}

template <typename P>
void DisplayImage<P>::gl_cross(const float x, const float y, double w)
{
    glBegin(GL_LINES);
    glVertex2d(x-w, y);
    glVertex2d(x+w, y);
    glVertex2d(x, y-w);
    glVertex2d(x, y+w);
    glEnd();
}

template<typename P>
void DisplayImage<P>::gl_rect(const fRect& rect)
{
    glBegin(GL_LINE_LOOP);
    glVertex2f(rect.ul().x(), rect.ul().y());
    glVertex2f(rect.ul().x(), rect.lr().y());
    glVertex2f(rect.lr().x(), rect.lr().y());
    glVertex2f(rect.lr().x(), rect.ul().y());
    glEnd();
}


template <typename P>
void DisplayImage<P>::gl_line(fPair f, fPair s)
{
    glBegin(GL_LINE);
    glVertex2d(f.x(), f.y());
    glVertex2d(s.x(), s.y());
    glEnd();
}


template <typename P>
void DisplayImage<P>::gl_line(fVector_2d& f, fVector_2d& s)
{
    glBegin(GL_LINE);
    glVertex2d(f.x(), f.y());
    glVertex2d(s.x(), s.y());
    glEnd();
}



void draw_arrow(
                float ax, float ay, float az,  /* starting point in local space */
                float bx, float by, float bz,  /* starting point in local space */
                float ah, float bh,            /* arrow head size start and end */
                char const * const annotation, /* annotation string */
                float annot_size,              /* annotation string height (local units) */
                bool both_end = false)
{
    int i;
    
    GLdouble mv[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, mv);
    
    /* We're assuming the modelview RS part is (isotropically scaled)
     * orthonormal, so the inverse is the transpose.
     * The local view direction vector is the 3rd column of the matrix;
     * assuming the view direction to be the normal on the arrows tangent
     * space  taking the cross product of this with the arrow direction
     * yields the binormal to be used as the orthonormal base to the
     * arrow direction to be used for drawing the arrowheads */
    
    double d[3] = {
        bx - ax,
        by - ay,
        bz - az
    };
    normalize_v(d);
    
    double r[3] = { mv[0], mv[4], mv[8] };
    int rev = scalarproduct_v(d, r) < 0.;
    
    double n[3] = { mv[2], mv[6], mv[10] };
    {
        double const s = scalarproduct_v(d,n);
        for(int i = 0; i < 3; i++)
            n[i] -= d[i]*s;
    }
    normalize_v(n);
    
    double b[3];
    crossproduct_v(n, d, b);
    
    /* Make a 60° arrowhead ( sin(60°) = 0.866... ) */
    GLfloat const pos[][3] = {
        {ax, ay, az},
        {bx, by, bz},
        { static_cast<GLfloat>(ax + (0.866*d[0] + 0.5*b[0])*ah),
            static_cast<GLfloat>(ay + (0.866*d[1] + 0.5*b[1])*ah),
            static_cast<GLfloat>(az + (0.866*d[2] + 0.5*b[2])*ah) },
        { static_cast<GLfloat>(ax + (0.866*d[0] - 0.5*b[0])*ah),
            static_cast<GLfloat>(ay + (0.866*d[1] - 0.5*b[1])*ah),
            static_cast<GLfloat>(az + (0.866*d[2] - 0.5*b[2])*ah) },
        { static_cast<GLfloat>(bx + (-0.866*d[0] + 0.5*b[0])*bh),
            static_cast<GLfloat>(by + (-0.866*d[1] + 0.5*b[1])*bh),
            static_cast<GLfloat>(bz + (-0.866*d[2] + 0.5*b[2])*bh) },
        { static_cast<GLfloat>(bx + (-0.866*d[0] - 0.5*b[0])*bh),
            static_cast<GLfloat>(by + (-0.866*d[1] - 0.5*b[1])*bh),
            static_cast<GLfloat>(bz + (-0.866*d[2] - 0.5*b[2])*bh) }
    };
    GLushort const idx[][2] = {
        {0, 1},
        {0, 2}, {0, 3},
        {1, 4}, {1, 5}
    };
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, pos);
    
    int lines = both_end ? 5 : 3;
    glDrawElements(GL_LINES, 2*lines, GL_UNSIGNED_SHORT, idx);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    if(annotation) {
        float w = 0;
        for(char const *c = annotation; *c; c++)
            w += glutStrokeWidth(GLUT_STROKE_ROMAN, *c);
        w *= annot_size / 100.;
        
        float tx = (ax + bx)/2.;
        float ty = (ay + by)/2.;
        float tz = (az + bz)/2.;
        
        GLdouble r[16] = {
            d[0], d[1], d[2], 0,
            b[0], b[1], b[2], 0,
            n[0], n[1], n[2], 0,
            0,    0,    0, 1
        };
        glPushMatrix();
        glTranslatef(tx, ty, tz);
        glMultMatrixd(r);
        if(rev)
            glScalef(-1, -1, 1);
        glTranslatef(-w/2., annot_size*0.1, 0);
        draw_strokestring(GLUT_STROKE_ROMAN, annot_size, annotation);
        glPopMatrix();
    }
}

template<typename P>
void DisplayImage<P>::gl_arrow (const fVector_2d& pos, uAngle8 gradient_angle)
{
    float r = 100;
    float sides = 0.0005;
    glPushMatrix();
    glTranslated(pos.x(), pos.y(), 0.0);
    draw_arrow(- cos(gradient_angle)/r, sin(gradient_angle)/r,0, 0.0,0.0, 0.0, sides, sides, 0, 0);
    glPopMatrix();
}




static void pSetHSV(float h, float s, float v)
{
    // H [0, 360] S and V [0.0, 1.0].
    int i = (int)floor(h / 60.0f) % 6;
    float f = h / 60.0f - floor(h / 60.0f);
    float p = v * (float)(1 - s);
    float q = v * (float)(1 - s * f);
    float t = v * (float)(1 - (1 - f) * s);
    switch (i)
    {
        case 0: glColor3f(v, t, p);
            break;
        case 1: glColor3f(q, v, p);
            break;
        case 2: glColor3f(p, v, t);
            break;
        case 3: glColor3f(p, q, v);
            break;
        case 4: glColor3f(t, p, v);
            break;
        case 5: glColor3f(v, p, q);
    }
}

template <typename P>
void tinyGL::DisplayImage<P>::drawData()
{
    std::vector<fRect> bbs = isocline::get_minbb_per_direction(m_data);
    unsigned int ecount = 0;
    for (int i = 0; i < m_data.size(); i++)
    {
        glDisable(GL_TEXTURE_2D);
        pSetHSV((i * 128.0f) / 256.0f, 1.0f, 1.0f);
        glBegin(GL_POINTS);
        
        // Draw mid points of isoclines sloped at this direction
        isocline_vector_t  on_direction = m_data[i];
      
#if 0
        if (m_options[m_options_dict["Segment Centers"]])
        {
            for (unsigned k = 0; k < on_direction.size(); k++)
            {
                fVector_2d mid = on_direction[k].mid();
                image2display(mid);
                gl_cross (mid.x(), mid.y(), 0.001);
                fVector_2d first = on_direction[k].first();
                fVector_2d last = on_direction[k].last();
                image2display(first);
                image2display(last);
                gl_line(first, last);
            }
        }
#endif
        for (unsigned k = 0; k < on_direction.size(); k++)
        {
            const std::vector<protuple>& container = on_direction[k].container();
            
            if (m_options[m_options_dict["Edges"]])
            {
                for(unsigned ee = 0; ee < container.size(); ee++)
                {
                    fVector_2d loc = std::get<1>(container[ee]);
                    uAngle8 ang (std::get<2>(container[ee]));
                    image2display(loc);
                    gl_arrow(loc, ang);
                    //                    gl_cross(loc.x(), loc.y(), 0.01);
                    ecount++;
                }
            }
        }
        
        
        if (m_options[m_options_dict["Index Locations"]])
        {
            if (bbs[i].area() > 300 && bbs[i].area() < 400)
            {
                fRect tmp = bbs[i];
                image2display(tmp);
                gl_rect(tmp);
            }
        }
        glEnd();
    }
    
    std::cout << "rendered " << ecount << " edges " << std::endl;
}


template class tinyGL::DisplayImage<P8U>;
template class tinyGL::DisplayImage<P16U>;




#if 0
// Create 64 x 64 RGB image of a chessboard.
void createChessboard(void)
{
    int i, j;
    for (i = 0; i < 64; i++)
        for (j = 0; j < 64; j++)
            if ( ( ((i/8)%2) && ((j/8)%2) ) || ( !((i/8)%2) && !((j/8)%2) ) )
            {
                chessboard[i][j][0] = 0x00;
                chessboard[i][j][1] = 0x00;
                chessboard[i][j][2] = 0x00;
            }
            else
            {
                chessboard[i][j][0] = 0xFF;
                chessboard[i][j][1] = 0xFF;
                chessboard[i][j][2] = 0xFF;
            }
}
#endif


