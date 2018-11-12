
#include "MotionManager.h"
#include "CoreMotionImpl.h"



std::mutex MotionManager::sMutex;
void MotionManager::enable( float updateFrequency, SensorMode mode, bool showsCalibrationDisplay )
{
    auto impl = get()->mImpl;
    impl->setSensorMode( mode );
    impl->setUpdateFrequency( updateFrequency );
    impl->setShowsCalibrationView( showsCalibrationDisplay );
    impl->startMotionUpdates();
}

void MotionManager::disable()
{
    get()->mImpl->stopMotionUpdates();
}

bool MotionManager::isEnabled()
{
    return get()->mImpl->isMotionUpdatesActive();
}

MotionManager::SensorMode MotionManager::getSensorMode()
{
    return get()->mImpl->getSensorMode();
}

bool MotionManager::isDataAvailable()
{
    return get()->mImpl->isMotionDataAvailable();
}

bool MotionManager::isGyroAvailable()
{
    return get()->mImpl->isGyroAvailable();
}

bool MotionManager::isNorthReliable()
{
    return get()->mImpl->isNorthReliable();
}

void MotionManager::setShowsCalibrationView( bool shouldShow )
{
    get()->mImpl->setShowsCalibrationView( shouldShow );
}

vec3 MotionManager::getGravityDirection( InterfaceOrientation orientation )
{
    return get()->mImpl->getGravityDirection( orientation );
}

quat MotionManager::getRotation( InterfaceOrientation orientation )
{
    return get()->mImpl->getRotation( orientation );
}

vec3 MotionManager::getRotationRate( InterfaceOrientation orientation )
{
    return get()->mImpl->getRotationRate( orientation );
}

vec3 MotionManager::getAcceleration( InterfaceOrientation orientation)
{
    return get()->mImpl->getAcceleration( orientation );
}

bool MotionManager::isShaking( float minShakeDeltaThreshold )
{
    return get()->isShakingImpl( minShakeDeltaThreshold );
}



MotionManager* MotionManager::get()
{
    static MotionManager *sInst = 0;
    std::unique_lock<std::mutex> lock( sMutex );
    if( ! sInst ) {
        sInst = new MotionManager;
        sInst->mImpl = std::make_shared<CoreMotionImpl>();
    }
    return sInst;
}


bool MotionManager::isShakingImpl( float minShakeDeltaThreshold )
{
    const vec3& accel = getAcceleration();
    std::unique_lock<std::mutex> lock( sMutex ); // lock after we get the acceleration so there is no deadlock
    bool isShaking = false;
    if( mPrevAcceleration != vec3( 0 ) ) {
        mShakeDelta = length2(accel - mPrevAcceleration);
        if( length2(accel - mPrevAcceleration) > (minShakeDeltaThreshold * minShakeDeltaThreshold) )
            isShaking = true;
    }
    mPrevAcceleration = accel;
    return isShaking;
}

float MotionManager::getAccelerometerFilterImpl()
{
    return mImpl->getAccelFilter();
}

void MotionManager::setAccelerometerFilterImpl( float filtering )
{
    mImpl->setAccelFilter( svl::math<float>::clamp( filtering, 0, 0.999f ) );
}

