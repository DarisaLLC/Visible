
#include "CoreMotionImpl.h"

#import <CoreMotion/CoreMotion.h>

// note we use ios sdk's /usr/include/math.h definition of PI etc.

CoreMotionImpl::CoreMotionImpl()
	: mSensorMode( MotionManager::SensorMode::Gyroscope ), mLastAccel( 0, 0, 0 ), mAccelFilter( 0.3f ), mLastAccelValid( false )
{
	mMotionManager = [[CMMotionManager alloc] init];
}

CoreMotionImpl::~CoreMotionImpl()
{
    stopMotionUpdates();
	[mMotionManager stopAccelerometerUpdates];
	mMotionManager = nil;
}

bool CoreMotionImpl::isMotionUpdatesActive()
{
    return mMotionManager.isDeviceMotionActive;
}

bool CoreMotionImpl::isMotionDataAvailable()
{
	return ( isMotionUpdatesActive() && mMotionManager.deviceMotion );
}

bool CoreMotionImpl::isGyroAvailable()
{
	return static_cast<bool>( mMotionManager.gyroAvailable );
}

bool CoreMotionImpl::isAccelAvailable()
{
	return static_cast<bool>( mMotionManager.accelerometerAvailable );
}

bool CoreMotionImpl::isNorthReliable()
{
	NSUInteger available = [CMMotionManager availableAttitudeReferenceFrames];
	return (available & CMAttitudeReferenceFrameXTrueNorthZVertical) || (available & CMAttitudeReferenceFrameXMagneticNorthZVertical) ? true : false;
}

void CoreMotionImpl::setSensorMode( MotionManager::SensorMode mode )
{
	if( mode == MotionManager::SensorMode::Gyroscope && ! isGyroAvailable() ) {
		mSensorMode = MotionManager::SensorMode::Accelerometer;
	}
	if( mode == MotionManager::SensorMode::Accelerometer && ! isAccelAvailable() )
		throw ExcNoSensors();

	mSensorMode = mode;
}

void CoreMotionImpl::startMotionUpdates()
{
	if( mSensorMode == MotionManager::SensorMode::Gyroscope ) {
		if( mMotionManager.deviceMotionAvailable ) {
			NSUInteger availableReferenceFrames = [CMMotionManager availableAttitudeReferenceFrames];
			CMAttitudeReferenceFrame referenceFrame;
			if (availableReferenceFrames & CMAttitudeReferenceFrameXTrueNorthZVertical)
				referenceFrame = CMAttitudeReferenceFrameXTrueNorthZVertical;
			else if (availableReferenceFrames & CMAttitudeReferenceFrameXMagneticNorthZVertical)
				referenceFrame = CMAttitudeReferenceFrameXMagneticNorthZVertical;
			else if (availableReferenceFrames & CMAttitudeReferenceFrameXArbitraryCorrectedZVertical)
				referenceFrame = CMAttitudeReferenceFrameXArbitraryCorrectedZVertical;
			else
				referenceFrame = CMAttitudeReferenceFrameXArbitraryZVertical;

			[mMotionManager startDeviceMotionUpdatesUsingReferenceFrame:referenceFrame];
			return;
		}
		else {
			setSensorMode( MotionManager::SensorMode::Accelerometer );
		}
	}
	// accelerometer mode
	[mMotionManager startAccelerometerUpdates];
}

void CoreMotionImpl::stopMotionUpdates()
{
    if( ! mMotionManager.isDeviceMotionActive )
        return;

    [mMotionManager stopDeviceMotionUpdates];
}

void CoreMotionImpl::setUpdateFrequency( float updateFrequency )
{
	mMotionManager.deviceMotionUpdateInterval = 1.0 / (NSTimeInterval)updateFrequency;
}

void CoreMotionImpl::setShowsCalibrationView( bool shouldShow )
{
	mMotionManager.showsDeviceMovementDisplay = shouldShow;
}

namespace {
	vec3 vecOrientationCorrected( const vec3 &vec, MotionManager::InterfaceOrientation orientation )
	{
		switch ( orientation ) {
			case MotionManager::PortraitUpsideDown:	return vec3( -vec.x, -vec.y, vec.z );
			case MotionManager::LandscapeLeft:		return vec3(  vec.y, -vec.x, vec.z );
			case MotionManager::LandscapeRight:		return vec3( -vec.y,  vec.x, vec.z );
			default:						return vec3(  vec.x,  vec.y, vec.z );
		}
	}
}

vec3 CoreMotionImpl::getGravityDirection( MotionManager::InterfaceOrientation orientation )
{
    if( ! isMotionDataAvailable() )
        return vec3( 0.0f, -1.0f, 0.0f );

    ::CMAcceleration g = mMotionManager.deviceMotion.gravity;
	return vecOrientationCorrected( vec3( g.x, g.y, g.z ), orientation );
}

quat CoreMotionImpl::getRotation( MotionManager::InterfaceOrientation orientation )
{
	static const float kPiOverTwo = M_PI / 2.0f;
	static const quat kCorrectionRotation = angleAxis( kPiOverTwo, vec3( 0, 1, 0 ) ) * angleAxis( kPiOverTwo, vec3( -1, 0, 0 ) );

	if( ! isMotionDataAvailable() ) {
		if( mLastAccelValid )
			return glm::rotation( vec3( 0, -1, 0 ), normalize( mLastAccel ) );
		else
			return quat();
	}

	::CMQuaternion cq = mMotionManager.deviceMotion.attitude.quaternion;
	quat q = kCorrectionRotation * quat( cq.w, cq.x, cq.y, cq.z );
	switch( orientation ) {
		case MotionManager::PortraitUpsideDown:
			q = angleAxis( (float)M_PI, vec3( 0, 0, 1 ) ) * quat( q.w, -q.x, -q.y, q.z );
		break;
		case MotionManager::LandscapeLeft:
			q = angleAxis( kPiOverTwo, vec3( 0, 0, 1 ) ) * quat( q.w, q.y, -q.x, q.z );
		break;
		case MotionManager::LandscapeRight:
			q = angleAxis( kPiOverTwo, vec3( 0, 0, -1 ) ) * quat( q.w, -q.y,  q.x, q.z );
		break;
		default:
		break;
	}

	return q;
}

vec3 CoreMotionImpl::getRotationRate( MotionManager::InterfaceOrientation orientation )
{
	if( ! isMotionDataAvailable() )
		return vec3( 0 );

	::CMRotationRate rot = mMotionManager.deviceMotion.rotationRate;
	return vecOrientationCorrected( vec3( rot.x, rot.y, rot.z ), orientation );
}

vec3 CoreMotionImpl::getAcceleration( MotionManager::InterfaceOrientation orientation )
{
	if( mSensorMode == MotionManager::SensorMode::Gyroscope ) {
		if( ! isMotionDataAvailable() )
			return vec3( 0 );

		::CMAcceleration accel = mMotionManager.deviceMotion.userAcceleration;
		return vecOrientationCorrected( vec3( accel.x, accel.y, accel.z ), orientation );
	}

	// accelerometer mode
	if( ! mMotionManager.accelerometerData )
		return vec3( 0 );

	::CMAcceleration accelCM = mMotionManager.accelerometerData.acceleration;
	vec3 accel( accelCM.x, accelCM.y, accelCM.z );
	vec3 accelFiltered = mLastAccel * mAccelFilter + accel * (1.0f - mAccelFilter);
	mLastAccel = accelFiltered;
	mLastAccelValid = true;
	
	return vecOrientationCorrected( accelFiltered, orientation );
}

