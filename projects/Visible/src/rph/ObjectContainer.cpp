/*
 * ObjectContainer.cpp
 *
 * Created by Daniel Scheibel on 10/25/12.
 *
 * Copyright (c) 2012, Red Paper Heart, All rights reserved.
 * This code is intended for use with the Cinder C++ library: http://libcinder.org
 *
 * To contact Red Paper Heart, email hello@redpaperheart.com or tweet @redpaperhearts
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "ObjectContainer.h"
namespace rph {
    
    Object* ObjectContainer::removeChildAt(int index){
        return *mChildren.erase( mChildren.begin()+index );
    }
    
    void ObjectContainer::removeChildren(int beginIndex, int endIndex){
        mChildren.erase(mChildren.begin()+beginIndex, mChildren.begin()+endIndex);
    };
    
    void ObjectContainer::update( float deltaTime, int beginIndex, int endIndex ){
        for( std::vector<Object *>::iterator it = mChildren.begin(); it != mChildren.end();){
            if( (*it)->isDead() ){
                it = mChildren.erase(it);
            } else {
                (*it)->update( deltaTime );
                ++it;
            }
        }
    }

    void ObjectContainer::draw(int beginIndex, int endIndex){
        for( std::vector<Object *>::iterator it = mChildren.begin(); it != mChildren.end(); it++){
            (*it)->draw();
        }
    }

}
