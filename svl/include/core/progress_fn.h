//
//  progress_fn.h
//  Visible
//
//  Created by Arman Garakani on 5/8/20.
//

#ifndef progress_fn_h
#define progress_fn_h

namespace svl{
// =========   Fraction Done Callback Definition ===========
// Added here to be used everywhere !!

    typedef std::function<void(float fraction_completed)> progress_fn_t;

}

#endif /* progress_fn_h */
