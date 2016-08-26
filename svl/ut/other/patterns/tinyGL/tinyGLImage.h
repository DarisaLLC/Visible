#pragma once

#include "tinyGL.h"
#include "vision/roiWindow.h"
#include "vision/edgel.h"
#include <glut/glut.h>
#include <bitset>

namespace tinyGL
{
template <typename P>
class DisplayImage : public Display
{
public:
    DisplayImage(int posX, int posY, int width, int height, const char * title, roiWindow<P> & image);
    DisplayImage(const iRect display_box, const char * title, roiWindow<P> & image);
    ~DisplayImage();
    DisplayImage(const DisplayImage & other) = delete;
    DisplayImage(DisplayImage && other) = delete;
    DisplayImage & operator=(const DisplayImage & other) = delete;
    DisplayImage & operator=(DisplayImage && other) = delete;

    void init() override;
    void display() override;
    void reshape(int width, int height) override;
    void keyboard(unsigned char key, int x, int y) override;
    void special(int key, int x, int y) override;
    void motion (int x, int y) override;
    void passiveMotion (int x, int y) override;

    void setData(const edgels::directed_features_t& data) const { m_data = data; }
    const edgels::directed_features_t& getData () const { return m_data; }
    void enableDrawData (bool which) const { m_draw_data = which; }
    bool doDrawData () const { return m_draw_data; }
    
    void image2display (fVector_2d& point);
    void image2display (fPair& point);
    void image2display (fRect& rect);
    
protected:
    void drawImage();
    void drawData ();
    void gl_cross(const float x, const float y, const double w);
    void gl_rect (const fRect& );
    void gl_line (fPair first, fPair second);
    void gl_line (fVector_2d& first, fVector_2d& second);
    void gl_arrow (const fVector_2d&, uAngle8);

    fPair mTextureSize;
    roiWindow<P> mImage;
    GLuint mImageTexture;
    GLenum mImageGLformat;
    GLenum mInternalGLformat;
    double mImageRatio;
    double mWindowRatio;
    double mMaxX;
    double mMaxY;
    double mS[16];
    double mT[16];
    double mInvWidth;
    double mInvHeight;
    mutable bool m_draw_data;
    mutable edgels::directed_features_t m_data;
    mutable bitset<16> m_options;
    std::map<std::string,uint32_t> m_options_dict;
};

    
    
}