//
//  FieldController.h
//  CinderFlow
//
//  Created by Hans Petter Eikemo on 7/6/10.
//

#include <boost/numeric/ublas/matrix.hpp>
#include "cinder/Vector.h"
#include "cinder/Color.h"
#include <list>
#include "ParticleSystem.h"

class FieldController {
public:
    FieldController();
    void setup();
    void draw();
	void drawDebug();

    void update();
    
    vec2 screenRatio;
    
    boost::numeric::ublas::matrix<vectorCell>  field;        
    std::list<particle>  particles;

};
