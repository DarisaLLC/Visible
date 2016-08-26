#include "tinyGLImage.h"
#include <cstdio>
#include <mutex>
#include <memory>
#include <iostream>
#include "vision/roiWindow.h"
#include "drawUtils.hpp"
using namespace std;
using namespace tinyGL;
using namespace svl;


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
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStoref(GL_UNPACK_ROW_LENGTH, mImage.rowUpdate() / mImage.components());
    glTexImage2D(GL_TEXTURE_2D, 0, mInternalGLformat, mImage.width(), mImage.height(), 0, mInternalGLformat, mImageGLformat,  mImage.rowPointer(0));
    glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
    glPixelStorei(GL_UNPACK_ALIGNMENT, 0);
    
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
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStoref(GL_UNPACK_ROW_LENGTH, mImage.rowUpdate() /  mImage.components());
    // GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mImage.width(), mImage.height(), mInternalGLformat, mImageGLformat,  mImage.rowPointer(0));
    glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
    glPixelStorei(GL_UNPACK_ALIGNMENT, 0);
    
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
    gl_cross(0.0f, 0.0f, 0.2);
    if (doDrawData())
        drawData();
    
    tinyGL::Display::display();
}

template <typename P>
void DisplayImage<P>::reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    mWindowRatio = static_cast<double>(width) / static_cast<double>(height);
    tinyGL::Display::reshape(width, height);
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
            throw Display::normal_return ();
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

template<typename P>
void DisplayImage<P>::gl_arrow (const fVector_2d& pos, uAngle8 gradient_angle)
{
    float w = 0.01;
    uDegree ga (gradient_angle);
    glPushMatrix();
    //Set Position and Rotation
    glTranslated(pos.x(), pos.y(), 0.0);
    glRotated(ga.Double() - 180.0, 0.0, 0.0, 1.0);
    glBegin(GL_LINES);
    glVertex2d(-w, 0);
    glVertex2d(w, 0);
    glVertex2d(0, -w);
    glVertex2d(0, w);
    glEnd();

    glPopMatrix();
#if 0
    
    int line_thickness = 1;
    
    fVector_2d p = pos;
    fVector_2d q (3.0f, uRadian(gradient_angle));
    q = p + q;
    
    double angle = atan2((double) p.y() - q.y(), (double) p.x() - q.x());
    
    double hypotenuse = sqrt( (double)(p.y() - q.y())*(p.y() - q.y()) + (double)(p.x() - q.x() )*(p.x() - q.x()) );
    
    if (hypotenuse < 0.1)
        return;
    
    // Here we lengthen the arrow by a factor of three.
  //  q.x(p.x() - 3 * hypotenuse * cos(angle));
//    q.y(p.y() - 3 * hypotenuse * sin(angle));
    
    // Now we draw the main line of the arrow.
    image2display(p);
    image2display(q);
    gl_line(p, q);
    
    // Now draw the tips of the arrow. I do some scaling so that the
    // tips look proportional to the main line of the arrow.
    
    p.x (q.x() + 9 * cos(angle + M_PI / 4));
    p.y (q.y() + 9 * sin(angle + M_PI / 4));
    gl_line(p, q);
    
    p.x (q.x() + 9 * cos(angle - M_PI / 4));
    p.y (q.y() + 9 * sin(angle - M_PI / 4));
    gl_line(p, q);
#endif
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
    //std::vector<fRect> bbs = isocline::get_minbb_per_direction(m_data);
    unsigned int ecount = 0;
    
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_POINTS);
    for (int i = 0; i < m_data.size(); i++)
    {
        // Draw mid points of isoclines sloped at this direction
        edgels::feature_vector_t  on_direction = m_data[i].container();
        for (unsigned k = 0; k < on_direction.size(); k++)
        {
            fVector_2d loc = on_direction[k].position();
            uAngle8 ang = on_direction[k].angle();
            pSetHSV((ang.basic() * 720.0f) / 256.0f, 1.0f, 1.0f);
            image2display(loc);
            gl_arrow(loc, ang);
            ecount++;
        }
    }
    glEnd();
    std::cout << "rendered " << ecount << " edges " << std::endl;
}


template class tinyGL::DisplayImage<P8U>;
template class tinyGL::DisplayImage<P16U>;
template class tinyGL::DisplayImage<P8UC3>;




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


