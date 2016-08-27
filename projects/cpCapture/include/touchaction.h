#pragma once

#include "cinder/cinder.h"
#include "CinderOpenCV.h"

using namespace cinder;
using namespace cinder::app;

// Functor which sets the bool referenced

struct boolAccessorFunctor
{
    boolAccessorFunctor(bool& option )
    : mOption( option )
    {}
    
    void operator()()
    {
        mOption = ~ mOption;
    }
    
    bool& mOption;
};


struct touch : public vec2
{
    typedef std::function<void()> callback_t;
    
    touch (const vec2& center, const ColorA& color, const float r):
    vec2(center), mColor (color), mRadius(r), mAlive(false), mStartTime(-1.0f)
    {
        mAlive_cb = nullptr;
        mDead_cb = nullptr;
    }
    
    void set_alive_call_back (callback_t cb)
    {
        mAlive_cb = cb;
    }
    
    void set_died_call_back (callback_t cb)
    {
        mDead_cb = cb;
    }
    
    const vec2& position () const { return *this; }
    bool isAlive () const { bool tmp = mAlive; return tmp; }
    
    const ColorA& color () { return mColor; }
    
    
    void update (bool onOroff) const
    {
        if (onOroff && !mAlive)
        {
            set_alive();
            mStartTime = getElapsedSeconds();
        }
        else if (! onOroff && mAlive)
        {
            set_dead ();
        }
    }
    
    float age () const
    {
        if (mAlive)
            return getElapsedSeconds() - mStartTime;
        else return -1.0f;
    }
    
    void draw(const Area& wa )
    {
        gl::ScopedColor cl (mColor);
        vec2 ipos (wa.getSize().x * position().x, wa.getSize().y* position().y);
        gl::drawSolidCircle(ipos, mRadius * (wa.getSize().x + wa.getSize().y ) / 2);
    }
    
    inline bool operator== (const touch& tt) const { return tt.position() == position(); }
    
    void set_alive () const
    {
        mAlive = true;
        if (mAlive_cb) mAlive_cb ();
    }
    void set_dead () const
    {
        mAlive = false;
        if (mDead_cb) mDead_cb ();
    }
    
    float		mRadius;
    ColorA      mColor;
    mutable bool        mAlive;
    mutable float       mStartTime;
    callback_t    mAlive_cb;
    callback_t    mDead_cb;
    
};




