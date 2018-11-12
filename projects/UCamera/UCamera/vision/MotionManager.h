
#pragma once

#include "core/GLMmath.h"
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace glm;


class CoreMotionImpl;

/*
 Singleton object that provides information from the device motion sensors in a variety of forms
 
 */
class MotionManager
{
public:
    enum SensorMode { Accelerometer, Gyroscope };
    
    enum InterfaceOrientation {
        Unknown					= 0,
        Portrait				= 1 << 0,
        PortraitUpsideDown		= 1 << 1,
        LandscapeLeft			= 1 << 2,
        LandscapeRight			= 1 << 3,
        PortraitAll				= (Portrait | PortraitUpsideDown),
        LandscapeAll			= (LandscapeLeft | LandscapeRight),
        All						= (PortraitAll | LandscapeAll)
    };
    
    
    //! Enable motion updates with the target \a updateFrequency in hertz.
    static void 	enable( float updateFrequency = 60.0f, SensorMode mode = Gyroscope, bool showsCalibrationDisplay = false );
    //! Disable motion updates.
    static void		disable();
    //! Returns whether motion detection is enabled.
    static bool		isEnabled();
    //! Returns the current mode of the MotionManager.
    static SensorMode	getSensorMode();
    //! Returns whether motion data is currently available for polling.
    static bool		isDataAvailable();
    //! Returns whether a gyroscope is available on the device.
    static bool		isGyroAvailable();
    //! Returns whether you can depend on the -Z axis pointing towards north with getRotation and getRotationMatrix
    static bool		isNorthReliable();
    //! Enables the system calibration view which will appear when the device needs some motion to recalibrate.
    static void		setShowsCalibrationView( bool shouldShow = true );
    
    //! Direction of gravity expressed as acceleration in the x, y and z axes. The output is correct for \a orientation if other than Portrait.
    static glm::vec3		getGravityDirection( InterfaceOrientation orientation = InterfaceOrientation::Portrait );
    //! Rotation represents the orientation of the device as the amount rotated from with the North Pole, which we define to be -Z when the device is upright. The output is correct for \a orientation if other than Portrait.
    static glm::quat		getRotation( InterfaceOrientation orientation = InterfaceOrientation::Portrait );
    //! Convenience method for returning the rotation repesented as a matrix. The output is correct for \a orientation if other than Portrait.
    static glm::mat4	getRotationMatrix( InterfaceOrientation orientation = InterfaceOrientation::Portrait ) { return glm::toMat4( getRotation( orientation ) ); }
    //! Rotation rate along the x, y, and z axes, measured in radians per second. The output is correct for \a orientation if other than Portrait.
    static glm::vec3		getRotationRate( InterfaceOrientation orientation = InterfaceOrientation::Portrait );
    //! Acceleration in G's along the x, y, and z axes.  Earth's gravity is filtered out. The output is correct for \a orientation if other than Portrait.
    static glm::vec3		getAcceleration( InterfaceOrientation orientation = InterfaceOrientation::Portrait );
    
    //! Detect if the device is currently shaking using the current and previous acceleration, as defined by an acceleration of magnitude >= \a minShakeDeltaThreshold.  \note This method is meant to be polled at a regular rate.
    static bool     isShaking( float minShakeDeltaThreshold = 2.2f );
    
    //! The amount the device is currently shaking. \note This is only calculated when isShaking() has been called.
    static float    getShakeDelta() { return get()->mShakeDelta; }
    
    //! Only applies in accelerometer-only mode. Returns the amount the accelerometer data is filtered, in the range [0-1). \c 0 means unfiltered, \c 0.99 is very smoothed (but less responsive). Default is \c 0.3.
    static float	getAccelerometerFilter() { return get()->getAccelerometerFilterImpl(); }
    //! Only applies in accelerometer-only mode. Returns the amount the accelerometer data is filtered, in the range [0-1). \c 0 means unfiltered, \c 0.99 is very smoothed (but less responsive). Default is \c 0.3.
    static void		setAccelerometerFilter( float filtering ) { get()->setAccelerometerFilterImpl( filtering ); }
    
protected:
    MotionManager() : mShakeDelta( 0.0f ) {}
    static MotionManager* get ();
private:
    bool	isShakingImpl( float minShakeDeltaThreshold );
    float	getAccelerometerFilterImpl();
    void	setAccelerometerFilterImpl( float filtering );
    
    
    std::shared_ptr<CoreMotionImpl>	mImpl;
    float mShakeDelta;
    vec3 mPrevAcceleration;
    
    static std::mutex sMutex;
};

//! Thrown when the necessary sensors cannot be found
class ExcNoSensors : public std::exception {
    virtual const char * what() const throw() { return "No available sensors"; }
};
