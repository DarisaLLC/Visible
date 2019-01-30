//
//  ParticleSystem.h
//  CinderFlow
//
//  Created by Hans Petter Eikemo on 7/20/10.
//

#include "cinder/Vector.h"
#include "cinder/Color.h"

using namespace ci;

struct vectorCell {
    ci::vec2 movement;
    vec2 change;
    vectorCell() : movement(0.0f,0.0f) , change(0.0f,0.0f) {};
};

struct particle {
    vec2 position;
    vec2 momentum;
    ci::ColorAf color;
    particle() : position(0.0f,0.0f) , momentum(0.0f,0.0f) {};
    
};

struct emitter {
    vec2 position;
    ci::ColorAf color;
    int timeToLive;
    unsigned emitRate;
};
