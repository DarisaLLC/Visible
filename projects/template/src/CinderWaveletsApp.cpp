#include "cinder/gl/gl.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/audio/InputNode.h"
#include "cinder/audio/MonitorNode.h"

#include "DftNode.h"
#include "DwtNode.h"
#include "Resources.h"
#include "AudioDrawUtils.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CinderWaveletsApp : public App {
public:
    void                        setup() override;
    void                        draw() override;
    void                        resize() override;
    void                        keyDown( KeyEvent event ) override;

private:
    audio::InputDeviceNodeRef	mInputDeviceNode;
    wavy::DftNodeRef	        mDftNode;
    wavy::DwtNodeRef	        mDwtNode1;
    wavy::DwtNodeRef	        mDwtNode2;
    wavy::DwtNodeRef	        mDwtNode3;
    SpectrumPlot				mDftPlot;
    WaveletDecompositionPlot    mDwtPlot1;
    WaveletDecompositionPlot    mDwtPlot2;
    WaveletDecompositionPlot    mDwtPlot3;
    gl::FboRef                  mPausedScreen;
    gl::GlslProgRef             mPaletteShader;
    int                         mSampleSize = 512;
    bool                        mPaused     = false;
};

void CinderWaveletsApp::setup()
{
    try
    {
        mPaletteShader = gl::GlslProg::create(
            ci::app::loadResource(PALETTE_VERT_SHADER),
            ci::app::loadResource(PALETTE_FRAG_SHADER));
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Shader load failed: " << ex.what() << std::endl;
        mPaletteShader.reset();
    }

    auto ctx = audio::Context::master();

    mInputDeviceNode = ctx->createInputDeviceNode();

    auto dftFormat = wavy::DftNode::Format()
        .fftSize(mSampleSize * 2)
        .windowSize(mSampleSize);

    mDftNode = ctx->makeNode(new wavy::DftNode(dftFormat));

    auto dwtFormat = wavy::DwtNode::Format()
        .decompositionLevels(5)
        .windowSize(mSampleSize);

    dwtFormat.motherWavelet(dsp::MotherWavelet::Haar);
    mDwtNode1 = ctx->makeNode(new wavy::DwtNode(dwtFormat));

    dwtFormat.motherWavelet(dsp::MotherWavelet::Daubechies5);
    mDwtNode2 = ctx->makeNode(new wavy::DwtNode(dwtFormat));

    dwtFormat.motherWavelet(dsp::MotherWavelet::Bior1_5);
    mDwtNode3 = ctx->makeNode(new wavy::DwtNode(dwtFormat));

    mInputDeviceNode >> mDftNode;
    mInputDeviceNode >> mDwtNode1;
    mInputDeviceNode >> mDwtNode2;
    mInputDeviceNode >> mDwtNode3;

    mInputDeviceNode->enable();
    ctx->enable();

    mDwtPlot1 = WaveletDecompositionPlot(mDwtNode1);
    mDwtPlot2 = WaveletDecompositionPlot(mDwtNode2);
    mDwtPlot3 = WaveletDecompositionPlot(mDwtNode3);

    mDwtPlot1.setPaletteShader(mPaletteShader);
    mDwtPlot2.setPaletteShader(mPaletteShader);
    mDwtPlot3.setPaletteShader(mPaletteShader);

    getWindow()->setTitle("Wavelet decompositions vs. FFT bins");
}

void CinderWaveletsApp::resize()
{
    float margin_percent = 0.05f;

    float margin_x = margin_percent * getWindowWidth();
    float margin_y = margin_percent * getWindowHeight();

    float plot_width = (getWindowWidth() - 3.0f * margin_x) * 0.5f;
    float plot_height = (getWindowHeight() - 3.0f * margin_y) * 0.5f;

    mDftPlot.setBounds(ci::Rectf(margin_x, margin_y, margin_x + plot_width, margin_y + plot_height));
    mDwtPlot1.setBounds(ci::Rectf(2.0f * margin_x + plot_width, margin_y, 2.0f * (margin_x + plot_width), margin_y + plot_height));
    mDwtPlot2.setBounds(ci::Rectf(margin_x, 2.0f * margin_y + plot_height, margin_x + plot_width, 2.0f * (margin_y + plot_height)));
    mDwtPlot3.setBounds(ci::Rectf(2.0f * margin_x + plot_width, 2.0f * margin_y + plot_height, 2.0f * (margin_x + plot_width), 2.0f * (margin_y + plot_height)));
}

void CinderWaveletsApp::keyDown(KeyEvent event)
{
    if (event.getCode() == event.KEY_ESCAPE)
        quit();
    else if (event.getCode() == event.KEY_p)
        mPaused = !mPaused;
    else if (event.getCode() == event.KEY_f)
        setFullScreen(!isFullScreen());
    else if (event.getCode() == event.KEY_d)
    {
        mDftPlot.enableScaleDecibels(!mDftPlot.getScaleDecibels());
        mDwtPlot1.enableScaleDecibels(!mDwtPlot1.getScaleDecibels());
        mDwtPlot2.enableScaleDecibels(!mDwtPlot2.getScaleDecibels());
        mDwtPlot3.enableScaleDecibels(!mDwtPlot3.getScaleDecibels());
    }

    if (mPaused)
    {
        mPausedScreen = gl::Fbo::create(getWindowWidth(), getWindowHeight());
        mPausedScreen->blitFromScreen(getWindowBounds(), mPausedScreen->getBounds());
    }
}

void CinderWaveletsApp::draw()
{
    gl::clear(Color::black());

    if (mPaused && mPausedScreen)
    {
        mPausedScreen->blitToScreen(mPausedScreen->getBounds(), getWindowBounds());
    }
    else
    {
        mDftPlot.draw(mDftNode->getMagSpectrum());
        mDwtPlot1.draw();
        mDwtPlot2.draw();
        mDwtPlot3.draw();
    }
}

CINDER_APP(CinderWaveletsApp, RendererGl)
