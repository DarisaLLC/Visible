//
//  main.cpp
//  CinderFlow
//
//  Created by Hans Petter Eikemo on 7/6/10.
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wcomma"


#include "cinder_opencv.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Timer.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/app/Platform.h"
#include "cinder/Rand.h"
#include "cinder/Color.h"

#include "FieldController.h"
#include <stdio.h>
#include <iterator>
#include <algorithm>

using namespace ci;
using namespace ci::app;
using namespace std;


class Main :  public App {
public:
    void prepareSettings( ci::app::AppBase::Settings *settings );
    void setup();
    void update();
    void draw();
    void keyDown( ci::app::KeyEvent event );
    void mouseDown( ci::app::MouseEvent event );
};


//Globals:
Timer timer;
FieldController field;
bool debugMode;

//Screenshots:
unsigned screenCount;
void writeScreenshot() {
 //   char num[5];
 //   sprintf(num, "%05i", screenCount );    
 //   writeImage( getHomeDirectory() + "output/flow_" + num + ".png", copyWindowSurface() );
    ++screenCount;
}


ColorAf getColor() {
//    return ColorAf(Rand::randFloat(),Rand::randFloat(),Rand::randFloat(),0.8f); //Random color;
    if ( (rand() % 2) == 1) {
        return ColorAf(0.0f,0.2f,0.3f,0.5f);    
        
    } else {
        return ColorAf(0.3f,0.2f,0.0f,0.5f);            
        
    }
    
    
}

list<emitter> emitters;
void createEmitter(vec2 pos, unsigned ttl=70) {
    emitter e;
    e.position = pos;
    e.color = getColor();
    e.emitRate = 1+rand()%9;
    e.timeToLive = ttl;
    emitters.push_back(e);
}

void updateEmitters() {
    for( list<emitter>::iterator em = emitters.begin(); em != emitters.end(); ++em ){
        for (unsigned i = 0;i < em->emitRate; ++i) {
            vec2 r = Rand::randVec2() * 0.06f;
            particle p;
            p.position = (em->position)/field.screenRatio ;
            p.momentum = r;
            p.color = em->color;
            field.particles.push_back(p);            
        }
        --em->timeToLive;
        if (em->timeToLive < 1){
            std::cout << em->timeToLive << std::endl;
            std::cout << emitters.size() << std::endl;
            auto d = std::distance(emitters.begin(), em);
            std::cout << d << std::endl;
        }
    }
}


//Main impl:

void Main::prepareSettings( Settings* settings ) {
    settings->setWindowSize( 1280 , 720 );
    settings->setFrameRate( 30.0f );
    
    srand( (unsigned)time( NULL ) );
    
}

void Main::setup() {
    field.setup();
    gl::clear( Color( 0.0f, 0.0f, 0.0f ) );
  //  debugMode = true;
}

void Main::keyDown( KeyEvent event ) {
	if( event.getChar() == 'f' ) {
        setFullScreen( ! isFullScreen() );
        gl::clear( Color( 0.0f, 0.0f, 0.0f ) );
    }
    if( event.getChar() == 'd' ) { 
        debugMode = !debugMode;
    }
        
}

void Main::mouseDown( MouseEvent event ) {	
    createEmitter(event.getPos());        
}

void Main::update() {
    timer.start();

    field.update();
    updateEmitters();

    timer.stop();
    //cout << "update in "<< timer.getSeconds() << "\n";
    
    if (field.particles.size() < 10000 && Rand::randFloat() > 0.9f) {
        vec2 wSize = getWindowSize();
        vec2 rvec = Rand::randVec2();
        rvec *= vec2(50,50);
        auto pos = wSize/2.0f +  rvec;
        createEmitter( pos, 40 * Rand::randFloat() );
        cout << field.particles.size() << " particles" << endl;
    }
        
}

void Main::draw() {
    timer.start();
    
    gl::setMatricesWindow( getWindowSize() );
            
    if (debugMode) {
        field.drawDebug();
    } else {
        field.draw();
    }
        
    
    timer.stop();
    //cout << "draw in "<< timer.getSeconds() << "\n";
    
   // writeScreenshot();
}

CINDER_APP ( Main, RendererGl );


#pragma GCC diagnostic pop

