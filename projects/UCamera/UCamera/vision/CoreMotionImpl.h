
#pragma once

#include "core/GLMmath.h"
#include "MotionManager.h"

#if defined( __OBJC__ )
@class CMMotionManager;
@class NSOperationQueue;
#else
class CMMotionManager;
class NSOperationQueue;
#endif



class CoreMotionImpl
{
public:
    CoreMotionImpl();
    ~CoreMotionImpl();
    
    bool isMotionUpdatesActive();
    bool isMotionDataAvailable();
    bool isGyroAvailable();
    bool isAccelAvailable();
    bool isNorthReliable();
    void startMotionUpdates();
    void stopMotionUpdates();
    void setSensorMode( MotionManager::SensorMode mode );
    MotionManager::SensorMode	getSensorMode() { return mSensorMode; }
    
    void setUpdateFrequency( float updateFrequency );
    void setShowsCalibrationView( bool shouldShow );
    
    vec3	getGravityDirection( MotionManager::InterfaceOrientation orientation );
    quat	getRotation( MotionManager::InterfaceOrientation orientation );
    vec3	getRotationRate( MotionManager::InterfaceOrientation orientation );
    vec3	getAcceleration( MotionManager::InterfaceOrientation orientation );
    
    float		getAccelFilter() const { return mAccelFilter; }
    void		setAccelFilter( float filtering ) { mAccelFilter = filtering; }
    
    bool drawCoordinateSystem ();
    
private:
    
    
    CMMotionManager				*mMotionManager;
    MotionManager::SensorMode	mSensorMode;
    
    vec3					mLastAccel;
    float						mAccelFilter;
    bool						mLastAccelValid;
};

