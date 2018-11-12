/*
	Copyright (C) 2016 Apple Inc. All Rights Reserved.
	See LICENSE.txt for this sample’s licensing information
	
	Abstract:
	View controller for camera interface.
 */

import UIKit
import AVFoundation
import Metal
import CoreVideo
import Photos
import MobileCoreServices



let CameraControlObservableSettingLensPosition = "CameraControlObservableSettingLensPosition"
let CameraControlObservableSettingFocusMode = "CameraControlObservableSettingFocusMode"
let CameraControlObservableSettingFrameRate = "CameraControlObservableSettingFrameRate"
let CameraControlObservableSettingAdjustingFocus = "CameraControlObservableSettingAdjustingFocus"


@objc protocol CameraSettingValueObserver {
    func cameraSetting(_ setting:String, valueChanged value:AnyObject)
}



class CameraViewController: UIViewController, AVCapturePhotoCaptureDelegate, CameraSettingValueObserver, AVCaptureVideoDataOutputSampleBufferDelegate {
    
    
    // MARK: Properties
    @IBOutlet weak var currentInfoLabel: UILabel!
    @IBOutlet weak var slider: UISlider!
    @IBOutlet weak var modeSwitch: UISwitch!
    @IBOutlet weak var focusMarkerView: FocusMarkerView!
    @IBOutlet weak var locationMarkerView: LocationMarkerView!
    
    
    @IBOutlet weak var lensPositionAtCapturedView: UIView!
    
    @IBOutlet weak var adjustingFocusIndicator: UIActivityIndicatorView!
    
    @IBOutlet weak private var cameraButton: UIButton!
    
    @IBOutlet weak private var photoButton: UIButton!
    
    @IBOutlet weak private var stackButton: UIButton!
    
    @IBOutlet weak private var resumeButton: UIButton!
    
    @IBOutlet weak var LPDisplay: UILabel!
    
    @IBOutlet weak var onCalibrate: UIButton!
    
    @IBOutlet weak var recordData: UIButton!
    @IBOutlet weak private var cameraUnavailableLabel: UILabel!
    
    @IBOutlet weak private var previewView: FilterMetalView!
    
    // Calibrator UI Elements.
    
    @IBOutlet weak var CalibResultView: UIImageView!
    fileprivate var spinnerView: UIActivityIndicatorView!
    fileprivate var textView: UILabel!
    fileprivate var progressView: UIProgressView!
    
    // Calibrator context.
    fileprivate var calibrator: ocvCalibrator!
    
    // Focus point for the camera.
    fileprivate var focusPoint: CGPoint?
    
    
    private enum SessionSetupResult {
        case success
        case notAuthorized
        case configurationFailed
    }
    
    fileprivate var lensPositionContext = 0
    fileprivate var focusModeContext = 0
    fileprivate var adjustingFrameRateContext = 0
    fileprivate var adjustingFocusContext = 0
    
    fileprivate var controlObservers = [String: [AnyObject]]()
    
    fileprivate var cvlink = CVSiteTemplate(title: "00")
    
    private var setupResult: SessionSetupResult = .success
    
    private let session = AVCaptureSession()
    
    private var isSessionRunning = false
    fileprivate var isTouchFocusOn : Bool = false
    fileprivate var isFullCalibrationOn : Bool = false
    fileprivate var isWaitingForManualStep : Bool = false
    private var isCalibrating = false
    
    
    fileprivate let sessionQueue = DispatchQueue(label: "session queue", attributes: [], autoreleaseFrequency: .workItem) // Communicate with the session and other session objects on this queue.
    /*
     In a multi-threaded producer consumer system it's generally a good idea to make sure that producers do not get starved of CPU time by their consumers.
     The VideoDataOutput frames come from a high priority queue, and downstream the preview uses the main queue.
     */
    private let videoDataOutputQueue = DispatchQueue(label: "video data queue", qos: .userInitiated, attributes: [], autoreleaseFrequency: .workItem)
    
    private let photoOutput = AVCapturePhotoOutput()
    
    private let videoDataOutput = AVCaptureVideoDataOutput()
    
    private var videoDeviceInput: AVCaptureDeviceInput!
    
    private var currentCameraDevice: AVCaptureDevice?
    
    fileprivate var metadataOutput:AVCaptureMetadataOutput!
    
    private var textureCache: CVMetalTextureCache!
    
    // private let syncQueue = DispatchQueue(label: "synchronization queue", attributes: [], autoreleaseFrequency: .workItem) // Synchronize access to currentFilteredPixelBuffer.
    
    private var renderingEnabled = true
    
    private var distanceCalSeq = [7,8,9,10,11,12,13,14,15,16,17,18,19,20,23,24,26,29,32,35,38,41,46,50]
    fileprivate var distanceCalStatus = -1
    
    private let focusScanStart: Float = 0.05
    private let focusScanRange: Float = 0.45
    private let focusScanIncrement: Float = 0.05
    private var focusScanSeq = [Float] ()
    
    private var calibrationFramesNeeded = 10
    private var calibrationFramesProcessed = 0
    
    fileprivate var focusTrackingStatus = -1
    
    private var temporalWindow = MovingRMS (period: 5)
    
    
    private let processingQueue = DispatchQueue(label: "photo processing queue", attributes: [], autoreleaseFrequency: .workItem)
    
    private let videoDeviceDiscoverySession = AVCaptureDeviceDiscoverySession(deviceTypes: [.builtInWideAngleCamera], mediaType: AVMediaTypeVideo, position: .unspecified)!
    
    private var statusBarOrientation: UIInterfaceOrientation = .portrait
    
    private var _previousSecondTimestamps: [CMTime] = []
    
    private var videoFrameRate = 0.0
    
    fileprivate var videoOutRect:CGRect = CGRect()
    
    fileprivate var exposureProbe:CGPoint = CGPoint(x: 0.5, y: 0.5)
    fileprivate var focusStackOn:Bool = false
    fileprivate var isfocusStackStarted:Bool = false
    
    
    /*!
     @property uniqueID
     @abstract
     uniqueID matches that of the AVCapturePhotoSettings instance you passed to -capturePhotoWithSettings:delegate:.
     */
    private var uniqueID: Int64 = 0
    
    
    /*!
     @property media Dimensions
     */
    private(set) var rawPhotoDimensions: CMVideoDimensions = CMVideoDimensions()
    private(set) var photoDimensions: CMVideoDimensions = CMVideoDimensions()
    private(set) var videoDimensions: CMVideoDimensions = CMVideoDimensions()
    private(set) var previewDimensions: CMVideoDimensions = CMVideoDimensions()
    
    private(set) var colorStatsROI: CGRect = CGRect()
    
    typealias LensPosCalibT = Dictionary<Int,calibParameters>
    typealias LensPosDistanceT = [Int:Float]
    
    private var calibrationDict: LensPosCalibT = [:]
    private var lensDistanceDict: LensPosDistanceT = [:]
    
    
    // MARK: Observer Registration
    
    func registerObserver<T>(_ observer:T, property:String) where T:NSObject, T:CameraSettingValueObserver {
        var propertyObservers = controlObservers[property]
        if propertyObservers == nil {
            propertyObservers = [AnyObject]()
        }
        
        propertyObservers?.append(observer)
        controlObservers[property] = propertyObservers
    }
    
    
    func unregisterObserver<T>(_ observer:T, property:String) where T:NSObject, T:CameraSettingValueObserver {
        _ = [Int]()
        if let propertyObservers = controlObservers[property] {
            let filteredPropertyObservers = propertyObservers.filter({ (obs) -> Bool in
                obs as! NSObject != observer
            })
            controlObservers[property] = filteredPropertyObservers
        }
    }
    
    /**
     Sets up the user interface.
     */
    fileprivate func createUI() {
        
        let frame = self.view.frame
        
        // Progress bar on top of the image.
        var progressRect = CGRect()
        progressRect.size.width = 100
        progressRect.size.height = 0
        progressRect.origin.x = (frame.size.width - progressRect.size.width) / 2
        progressRect.origin.y = (frame.size.height - progressRect.size.height) / 2
        progressView = UIProgressView(frame: progressRect)
        progressView!.isHidden = true
        view.addSubview(progressView)
        
        // Spinner shown during calibration.
        var spinnerRect = CGRect()
        spinnerRect.size.width = 20
        spinnerRect.size.height = 20
        spinnerRect.origin.x = (frame.size.width - spinnerRect.size.width) / 2
        spinnerRect.origin.y = (frame.size.height - spinnerRect.size.height) / 2
        spinnerView = UIActivityIndicatorView(frame: spinnerRect)
        view.addSubview(spinnerView)
        
        // Text view indicating status.
        var textRect = CGRect()
        textRect.size.width = 200
        textRect.size.height = 20
        textRect.origin.x = (frame.size.width - textRect.size.width) / 2
        textRect.origin.y = (frame.size.height - textRect.size.height) / 2 + 20
        textView = UILabel(frame: textRect)
        textView!.textColor = UIColor.white
        textView!.textAlignment = .center
        textView!.text = ""
        textView!.isHidden = true
        view.addSubview(textView)
    }
    
    /**
     Called when an image is processed by the calibrator.
     */
    func onProgress(_ progress: Float) {
        DispatchQueue.main.async {
            if (progress < 1.0) {
                self.textView.isHidden = false
                self.textView.text = "Capturing data"
                
                self.progressView.isHidden = false
                self.progressView.progress = progress
                
                self.spinnerView.stopAnimating()
            } else {
                self.textView.isHidden = false
                self.textView.text = "Calibrating"
                
                self.progressView.isHidden = true
                
                self.spinnerView.startAnimating()
            }
        }
    }
    
    // MARK: View Controller Life Cycle
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        self.registerObserver(self, property: CameraControlObservableSettingAdjustingFocus)
        self.registerObserver(self, property: CameraControlObservableSettingLensPosition)
        self.registerObserver(self, property: CameraControlObservableSettingFocusMode)
        
        // Disable UI. The UI is enabled if and only if the session starts running.
        cameraButton.isEnabled = false
        photoButton.isEnabled = false
        stackButton.isEnabled = false
        
        let gestureRecognizer = UITapGestureRecognizer(target: self, action: #selector(focusAndExposeTap))
        previewView.addGestureRecognizer(gestureRecognizer)
        previewDimensions.width = Int32(previewView.layer.bounds.width)
        previewDimensions.height = Int32(previewView.layer.bounds.height)
        
        // Check video authorization status, video access is required
        switch AVCaptureDevice.authorizationStatus(forMediaType: AVMediaTypeVideo) {
        case .authorized:
            // The user has previously granted access to the camera
            break
            
        case .notDetermined:
            /*
             The user has not yet been presented with the option to grant video access
             We suspend the session queue to delay session setup until the access request has completed
             */
            sessionQueue.suspend()
            AVCaptureDevice.requestAccess(forMediaType: AVMediaTypeVideo, completionHandler: { granted in
                if !granted {
                    self.setupResult = .notAuthorized
                }
                self.sessionQueue.resume()
            })
            
        default:
            // The user has previously denied access
            setupResult = .notAuthorized
        }
        
        // Initialize texture cache for metal rendering
        let metalDevice = MTLCreateSystemDefaultDevice()
        var textCache: CVMetalTextureCache?
        if (CVMetalTextureCacheCreate(kCFAllocatorDefault, nil, metalDevice!, nil, &textCache) != kCVReturnSuccess) {
            print("Unable to allocate texture cache")
            setupResult = .configurationFailed
        }
        else {
            textureCache = textCache
        }
        
        // Setup Distance Calibration
        self.recordData.isHidden = true
        self.resumeButton.isHidden = true
        
        // Setup Static Focus stack
        self.focusScanSeq.removeAll ()
        self.focusScanSeq.append (focusScanStart - focusScanIncrement)
        let after : Float = focusScanStart + focusScanRange + focusScanIncrement
        
        while self.focusScanSeq.last! < after
        {
            let lastv : Float = self.focusScanSeq.last!
            self.focusScanSeq.append(lastv + focusScanIncrement)
        }
        
        for element in self.focusScanSeq {
            print(element, terminator: ",")
        }
        
        /*
         Setup the capture session.
         In general it is not safe to mutate an AVCaptureSession or any of its
         inputs, outputs, or connections from multiple threads at the same time.
         
         Why not do all of this on the main queue?
         Because AVCaptureSession.startRunning() is a blocking call which can
         take a long time. We dispatch session setup to the sessionQueue so
         that the main queue isn't blocked, which keeps the UI responsive.
         */
        sessionQueue.async {
            self.configureSession()
        }
        
    }
    
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        let interfaceOrientation = UIApplication.shared.statusBarOrientation
        statusBarOrientation = interfaceOrientation
        
        sessionQueue.async {
            switch self.setupResult {
            case .success:
                // Only setup observers and start the session running if setup succeeded
                self.addObservers()
                if let photoOrientation = interfaceOrientation.videoOrientation {
                    self.photoOutput.connection(withMediaType: AVMediaTypeVideo).videoOrientation = photoOrientation
                }
                let videoOrientation = self.videoDataOutput.connection(withMediaType: AVMediaTypeVideo).videoOrientation
                let videoDevicePosition = self.videoDeviceInput.device.position
                
                // Setup pass-through
                let rotation = FilterMetalView.Rotation(with: interfaceOrientation, videoOrientation: videoOrientation, cameraPosition: videoDevicePosition)
                
                //
                DispatchQueue.main.async {
                    self.previewView.setDeferDrawingUntilNewTexture = true
                    self.previewView.mirroring = (videoDevicePosition == .front)
                    if let rotation = rotation {
                        self.previewView.rotation = rotation
                    }
                    self.videoDataOutputQueue.async {
                        self.renderingEnabled = true
                    }
                }
                self.observeValues()
                self.session.startRunning()
                self.isSessionRunning = self.session.isRunning
                
            case .notAuthorized:
                DispatchQueue.main.async {
                    let message = NSLocalizedString("AVCamPhotoFilter doesn't have permission to use the camera, please change privacy settings", comment: "Alert message when the user has denied access to the camera")
                    let alertController = UIAlertController(title: "AVCamPhotoFilter", message: message, preferredStyle: .alert)
                    alertController.addAction(UIAlertAction(title: NSLocalizedString("OK", comment: "Alert OK button"), style: .cancel, handler: nil))
                    alertController.addAction(UIAlertAction(title: NSLocalizedString("Settings", comment: "Alert button to open Settings"), style: .`default`, handler: { action in
                        UIApplication.shared.open(URL(string: UIApplicationOpenSettingsURLString)!, options: [:], completionHandler: nil)
                    }))
                    
                    self.present(alertController, animated: true, completion: nil)
                }
                
            case .configurationFailed:
                DispatchQueue.main.async {
                    let message = NSLocalizedString("Unable to capture media", comment: "Alert message when something goes wrong during capture session configuration")
                    let alertController = UIAlertController(title: "AVCamPhotoFilter", message: message, preferredStyle: .alert)
                    alertController.addAction(UIAlertAction(title: NSLocalizedString("OK", comment: "Alert OK button"), style: .cancel, handler: nil))
                    
                    self.present(alertController, animated: true, completion: nil)
                }
            }
        }
    }
    
    override func viewWillDisappear(_ animated: Bool) {
        videoDataOutputQueue.async {
            self.renderingEnabled = false
        }
        sessionQueue.async {
            if self.setupResult == .success {
                self.session.stopRunning()
                self.isSessionRunning = self.session.isRunning
                self.removeObservers()
            }
        }
        
        super.viewWillDisappear(animated)
    }
    
    func didEnterBackground(notification: NSNotification) {
        // Free up resources
        videoDataOutputQueue.async {
            self.renderingEnabled = false
            //   self.syncQueue.sync {
            //  entered async in this queue, set current to nil
            // }
        }
        processingQueue.async {
            // entered async in this queue, reset
        }
    }
    
    func willEnterForground(notification: NSNotification) {
        videoDataOutputQueue.async {
            self.renderingEnabled = true
        }
    }
    
    override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
        return .all
    }
    
    override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
        super.viewWillTransition(to: size, with: coordinator)
        
        coordinator.animate(
            alongsideTransition: { context in
                let interfaceOrientation = UIApplication.shared.statusBarOrientation
                self.statusBarOrientation = interfaceOrientation
                self.sessionQueue.async {
                    /*
                     The photo orientation is based on the interface orientation.
                     You could also set the orientation of the photo connection based on the device orientation by observing UIDeviceOrientationDidChangeNotification.
                     */
                    if let photoOrientation = interfaceOrientation.videoOrientation {
                        self.photoOutput.connection(withMediaType: AVMediaTypeVideo).videoOrientation = photoOrientation
                    }
                    let videoOrientation = self.videoDataOutput.connection(withMediaType: AVMediaTypeVideo).videoOrientation
                    if let rotation = FilterMetalView.Rotation(with: interfaceOrientation, videoOrientation: videoOrientation, cameraPosition: self.videoDeviceInput.device.position) {
                        DispatchQueue.main.async {
                            if rotation != self.previewView.rotation {
                                self.previewView.setDeferDrawingUntilNewTexture = true
                                self.previewView.rotation = rotation
                            }
                        }
                    }
                }
        }, completion: nil
        )
    }
    
    // MARK: KVO and Notifications
    
    private var sessionRunningContext = 0
    
    func cameraSetting(_ setting: String, valueChanged value: AnyObject) {
        switch setting {
        case CameraControlObservableSettingAdjustingFocus:
            if let adjusting = value as? Bool {
                if adjusting && !adjustingFocusIndicator.isAnimating
                {
                    adjustingFocusIndicator.startAnimating()
                }
                if !adjusting && adjustingFocusIndicator.isAnimating
                {
                    adjustingFocusIndicator.stopAnimating()
                }
            }
            
        case CameraControlObservableSettingLensPosition:
            if let lensPosition = value as? Float {
                slider.value = lensPosition
            }
            
        case CameraControlObservableSettingFocusMode:
            if let autoFocus = value as? Bool {
                modeSwitch.isOn = autoFocus
                slider.isEnabled = !autoFocus
            }
            
        case CameraControlObservableSettingFrameRate:
            
            
            displayCurrentValues()
            
        default: break
        }
    }
    
    private func addObservers() {
        NotificationCenter.default.addObserver(self, selector: #selector(didEnterBackground), name: NSNotification.Name.UIApplicationDidEnterBackground, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(willEnterForground), name: NSNotification.Name.UIApplicationWillEnterForeground, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(sessionRuntimeError), name: NSNotification.Name.AVCaptureSessionRuntimeError, object: session)
        
        session.addObserver(self, forKeyPath: "running", options: NSKeyValueObservingOptions.new, context: &sessionRunningContext)
        
        /*
         A session can only run when the app is full screen. It will be interrupted
         in a multi-app layout, introduced in iOS 9, see also the documentation of
         AVCaptureSessionInterruptionReason. Add observers to handle these session
         interruptions and show a preview is paused message. See the documentation
         of AVCaptureSessionWasInterruptedNotification for other interruption reasons.
         */
        NotificationCenter.default.addObserver(self, selector: #selector(sessionWasInterrupted), name: NSNotification.Name.AVCaptureSessionWasInterrupted, object: session)
        NotificationCenter.default.addObserver(self, selector: #selector(sessionInterruptionEnded), name: NSNotification.Name.AVCaptureSessionInterruptionEnded, object: session)
        NotificationCenter.default.addObserver(self, selector: #selector(subjectAreaDidChange), name: NSNotification.Name.AVCaptureDeviceSubjectAreaDidChange, object: videoDeviceInput.device)
    }
    
    private func removeObservers() {
        NotificationCenter.default.removeObserver(self)
        
        session.removeObserver(self, forKeyPath: "running", context: &sessionRunningContext)
    }
    
    override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey : Any]?, context: UnsafeMutableRawPointer?) {
        
        if context == &sessionRunningContext {
            let newValue = change?[.newKey] as AnyObject?
            guard let isSessionRunning = newValue?.boolValue else { return }
            DispatchQueue.main.async {
                self.cameraButton.isEnabled = (isSessionRunning && self.videoDeviceDiscoverySession.devices.count > 1)
                self.photoButton.isEnabled = isSessionRunning
                self.stackButton.isEnabled = isSessionRunning
            }
        }
        else
        {
            var key = ""
            let newValue: AnyObject = change![NSKeyValueChangeKey.newKey]! as AnyObject
            
            switch context {
                
            case .some(&lensPositionContext):
                key = CameraControlObservableSettingLensPosition
                
            case .some(&focusModeContext):
                key = CameraControlObservableSettingFocusMode
                
            case .some(&adjustingFocusContext):
                key = CameraControlObservableSettingAdjustingFocus
                
            case .some(&adjustingFrameRateContext):
                key = CameraControlObservableSettingFrameRate
                
            default:
                key = "unknown context"
            }
            
            notifyObservers(key, value: newValue)
        }
    }
    
    // MARK: Session Management
    
    // Call this on the session queue
    private func configureSession() {
        if setupResult != .success {
            return
        }
        
        guard let videoDevice = AVCaptureDevice.defaultDevice(withDeviceType: .builtInWideAngleCamera, mediaType: AVMediaTypeVideo, position: .unspecified) else {
            print("Could not find any video device")
            setupResult = .configurationFailed
            return
        }
        do {
            self.currentCameraDevice = videoDevice
            videoDeviceInput = try AVCaptureDeviceInput(device: videoDevice)
        }
        catch {
            print("Could not create video device input: \(error)")
            setupResult = .configurationFailed
            return
        }
        
        session.beginConfiguration()
        
        // Photo setting sets preview size to 1000 75 and Photo Size to max
        session.sessionPreset = AVCaptureSessionPresetPhoto
        session.automaticallyConfiguresCaptureDeviceForWideColor = false
        
        guard session.canAddInput(videoDeviceInput) else {
            print("Could not add video device input to the session")
            setupResult = .configurationFailed
            session.commitConfiguration()
            return
        }
        
        session.addInput(videoDeviceInput)
        if session.canAddOutput(videoDataOutput) {
            session.addOutput(videoDataOutput)
            videoDataOutput.alwaysDiscardsLateVideoFrames = true
            videoDataOutput.videoSettings = [kCVPixelBufferPixelFormatTypeKey as String: Int(kCVPixelFormatType_32BGRA)]
            videoDataOutput.setSampleBufferDelegate(self, queue: videoDataOutputQueue)
        }
        else {
            print("Could not add video data output to the session")
            setupResult = .configurationFailed
            session.commitConfiguration()
            return
        }
        if session.canAddOutput(photoOutput) {
            session.addOutput(photoOutput)
        }
        else {
            print("Could not add photo output to the session")
            setupResult = .configurationFailed
            session.commitConfiguration()
            return
        }
        
        session.commitConfiguration()
        
        //        self.configure60FPSDevice(self.currentCameraDevice!)
    }
    
    private func resumeToNormal ()
    {
        let devicePoint = CGPoint(x: 0.5, y: 0.5)
        resetFocus(with: .continuousAutoFocus, at: devicePoint, monitorSubjectAreaChange: true)
    }
    
    private func resetFocus(with focusMode: AVCaptureFocusMode, at devicePoint: CGPoint, monitorSubjectAreaChange: Bool) {
        sessionQueue.async {
            guard let videoDevice = self.videoDeviceInput.device else {
                return
            }
            
            do {
                try videoDevice.lockForConfiguration()
                if videoDevice.isFocusPointOfInterestSupported && videoDevice.isFocusModeSupported(focusMode) {
                    videoDevice.focusPointOfInterest = devicePoint
                    videoDevice.focusMode = focusMode
                }
                
                videoDevice.isSubjectAreaChangeMonitoringEnabled = monitorSubjectAreaChange
                videoDevice.unlockForConfiguration()
            }
            catch {
                print("Could not lock device for configuration: \(error)")
            }
        }
    }
    
    
    private func resetExpose (with exposureMode: AVCaptureExposureMode, at devicePoint: CGPoint, monitorSubjectAreaChange: Bool) {
        sessionQueue.async {
            guard let videoDevice = self.videoDeviceInput.device else {
                return
            }
            
            do {
                try videoDevice.lockForConfiguration()
                if videoDevice.isExposurePointOfInterestSupported && videoDevice.isExposureModeSupported(exposureMode) {
                    videoDevice.exposurePointOfInterest = self.exposureProbe
                    videoDevice.exposureMode = exposureMode
                }
                
                videoDevice.isSubjectAreaChangeMonitoringEnabled = monitorSubjectAreaChange
                videoDevice.unlockForConfiguration()
            }
            catch {
                print("Could not lock device for configuration: \(error)")
            }
        }
    }
    
    private func resetFocusAndExposure(with focusMode: AVCaptureFocusMode, exposureMode: AVCaptureExposureMode, at devicePoint: CGPoint, monitorSubjectAreaChange: Bool) {
        sessionQueue.async {
            guard let videoDevice = self.videoDeviceInput.device else {
                return
            }
            
            do {
                try videoDevice.lockForConfiguration()
                if videoDevice.isFocusPointOfInterestSupported && videoDevice.isFocusModeSupported(focusMode) {
                    videoDevice.focusPointOfInterest = devicePoint
                    videoDevice.focusMode = focusMode
                }
                
                if videoDevice.isExposurePointOfInterestSupported && videoDevice.isExposureModeSupported(exposureMode) {
                    videoDevice.exposurePointOfInterest = self.exposureProbe
                    videoDevice.exposureMode = exposureMode
                }
                
                videoDevice.isSubjectAreaChangeMonitoringEnabled = monitorSubjectAreaChange
                videoDevice.unlockForConfiguration()
            }
            catch {
                print("Could not lock device for configuration: \(error)")
            }
        }
    }
    
    // MARK: Focus, Tap and Watch
    @IBAction private func focusAndExposeTap(_ gestureRecognizer: UIGestureRecognizer) {
        var devicePoint = CGPoint(x: 0.5, y: 0.5)
        
        // Supporting user slectable focus point
        if self.isTouchFocusOn
        {
            let location = gestureRecognizer.location(in: previewView)
            guard let texturePoint = previewView.texturePointForView(point: location) else {
                return
            }
            focusMarkerView.center = location
            let textureRect = CGRect(origin: texturePoint, size: CGSize.zero)
            let deviceRect = videoDataOutput.metadataOutputRectOfInterest(for: textureRect)
            devicePoint =  deviceRect.origin
        }
        
        resetFocusAndExposure(with: .continuousAutoFocus, exposureMode: .autoExpose, at:devicePoint, monitorSubjectAreaChange: true)
        
        if (self.calibrator != nil)
        {
            self.calibrator.focus((self.currentCameraDevice?.lensPosition)!,
                                  x: Float(devicePoint.x),
                                  y: Float(devicePoint.y))
        }
    }
    
    // MARK: AreaChange: switch to AutoFocus
    func subjectAreaDidChange(notification: NSNotification) {
        let devicePoint = CGPoint(x: 0.5, y: 0.5)
        resetFocusAndExposure(with: .continuousAutoFocus, exposureMode: .continuousAutoExposure, at: devicePoint, monitorSubjectAreaChange: false)
        
        
    }
    
    func sessionWasInterrupted(notification: NSNotification) {
        // In iOS 9 and later, the userInfo dictionary contains information on why the session was interrupted.
        if let userInfoValue = notification.userInfo?[AVCaptureSessionInterruptionReasonKey] as AnyObject?, let reasonIntegerValue = userInfoValue.integerValue, let reason = AVCaptureSessionInterruptionReason(rawValue: reasonIntegerValue) {
            print("Capture session was interrupted with reason \(reason)")
            
            if reason == .videoDeviceInUseByAnotherClient {
                // Simply fade-in a button to enable the user to try to resume the session running.
            }
            else if reason == .videoDeviceNotAvailableWithMultipleForegroundApps {
                // Simply fade-in a label to inform the user that the camera is unavailable.
                cameraUnavailableLabel.isHidden = false
                cameraUnavailableLabel.alpha = 0.0
                UIView.animate(withDuration: 0.25) {
                    self.cameraUnavailableLabel.alpha = 1.0
                }
            }
        }
    }
    
    func sessionInterruptionEnded(notification: NSNotification) {
        //        if !resumeButton.isHidden {
        //            UIView.animate(withDuration: 0.25,
        //                           animations: {
        //                            self.resumeButton.alpha = 0
        //            }, completion: { finished in
        //                self.resumeButton.isHidden = true
        //            }
        //            )
        //        }
        if !cameraUnavailableLabel.isHidden {
            UIView.animate(withDuration: 0.25,
                           animations: {
                            self.cameraUnavailableLabel.alpha = 0
            }, completion: { finished in
                self.cameraUnavailableLabel.isHidden = true
            }
            )
        }
    }
    
    func sessionRuntimeError(notification: NSNotification) {
        guard let errorValue = notification.userInfo?[AVCaptureSessionErrorKey] as? NSError else {
            return
        }
        
        let error = AVError(_nsError: errorValue)
        print("Capture session runtime error: \(error)")
        
        /*
         Automatically try to restart the session running if media services were
         reset and the last start running succeeded. Otherwise, enable the user
         to try to resume the session running.
         */
        if error.code == .mediaServicesWereReset {
            sessionQueue.async {
                if self.isSessionRunning {
                    self.session.startRunning()
                    self.isSessionRunning = self.session.isRunning
                }
                else {
                    DispatchQueue.main.async {
                        self.resumeButton.isHidden = false
                    }
                }
            }
        }
        else {
            resumeButton.isHidden = false
        }
    }
    
    
    @IBAction func StackFocus(_ stackButton: UIButton) {
        self.focusTrackingStatus = 0
    }
    
    @IBAction func nextDistance(_ sender: UIButton) {
        sessionQueue.async {
            if !self.session.isRunning {
                DispatchQueue.main.async {
                    let message = NSLocalizedString("Unable to Advance", comment: "Alert message when unable to advance")
                    let alertController = UIAlertController(title: "AVCamPhotoFilter", message: message, preferredStyle: .alert)
                    let cancelAction = UIAlertAction(title:NSLocalizedString("OK", comment: "Alert OK button"), style: .cancel, handler: nil)
                    alertController.addAction(cancelAction)
                    self.present(alertController, animated: true, completion: nil)
                }
            }
            else {
                DispatchQueue.main.async {
                    // Announce new Distance and show Next button
                    self.distanceCalStatus += 1
                    if self.isCalibrating || self.distanceCalStatus < self.distanceCalSeq.count
                    {
                        let clp = self.currentLensPosition()
                        self.LPDisplay.text = String(format: "[%2d ] = %1.2f", self.distanceCalSeq[self.distanceCalStatus], clp!)
                        self.LPDisplay.isHidden = false
                        self.LPDisplay.alpha = 0.85
                        
                        // Wait until new distance is setup and auto focus has been achieved
                        self.resumeButton.isHidden = true
                        self.recordData.isHidden = false
                    }
                }
            }
        }
    }
    
    // Indiates AutoFocus is Achieved.
    @IBAction func recordLensDistance(_ sender: Any) {
        
        DispatchQueue.main.async {
            
            if self.isCalibrating && self.resumeButton.isHidden
            {
                let dist = self.distanceCalSeq[self.distanceCalStatus]
                let lp = Float(self.currentLensPosition()!)
                self.lensDistanceDict[dist] = lp
                
                print("\(dist) : \(String(describing: lp))")
                
                if self.distanceCalStatus == (self.distanceCalSeq.count - 1) {
                    //  FIXME   self.outputDictionary()
                    self.resumeButton.isHidden = true
                    self.recordData.isHidden = true
                    self.isCalibrating = false
                    
                }
                else{
                    // signal new distance
                    self.resumeButton.isHidden = false
                    self.recordData.isHidden = true
                    
                }
                
            }
        }
    }
    
    @IBAction private func calibrateCamera(_ sender: UIButton) {
        
        if isCalibrating == false && self.currentCameraDevice?.position != .front && self.distanceCalStatus == -1
        {
            DispatchQueue.main.async {
                // Announce new Distance and show Next button
                self.LPDisplay.text = "Calibrating: \(self.distanceCalSeq.count) "
                self.LPDisplay.isHidden = false
                self.LPDisplay.alpha = 0.85
                self.isCalibrating = true
                self.resumeButton.isHidden = false
                self.recordData.isHidden = true
            }
        }
        
    }
    
    public func outputDictionary(){
        
        do{
            let jsonData = try JSONSerialization.data(withJSONObject: self.lensDistanceDict, options: .prettyPrinted)
            // here "jsonData" is the dictionary encoded in JSON data
            
            let decoded = try JSONSerialization.jsonObject(with: jsonData, options: [])
            // here "decoded" is of type `Any`, decoded from JSON data
            
            // you can now cast it with the right type
            if let dictFromJSON = decoded as? [String:String] {
                print(dictFromJSON)
            }
        } catch {
            print(error.localizedDescription)
        }
    }
    
    
    @IBAction private func changeCamera(_ sender: UIButton) {
        cameraButton.isEnabled = false
        photoButton.isEnabled = false
        stackButton.isEnabled = false
        videoDataOutputQueue.sync {
            self.renderingEnabled = false
        }
        
        previewView.setDeferDrawingUntilNewTexture = true
        let interfaceOrientation = self.statusBarOrientation
        
        sessionQueue.async {
            guard let currentVideoDevice = self.videoDeviceInput.device else {
                return
            }
            var preferredPosition = AVCaptureDevicePosition.unspecified
            let currentPhotoOrientation = self.photoOutput.connection(withMediaType: AVMediaTypeVideo).videoOrientation
            
            switch currentVideoDevice.position {
            case .unspecified, .front:
                preferredPosition = .back
                
            case .back:
                preferredPosition = .front
            }
            
            let devices = self.videoDeviceDiscoverySession.devices
            if let videoDevice = devices?.filter({ $0.position == preferredPosition }).first {
                var videoDeviceInput: AVCaptureDeviceInput
                do {
                    videoDeviceInput = try AVCaptureDeviceInput(device: videoDevice)
                }
                catch {
                    print("Could not create video device input: \(error)")
                    self.videoDataOutputQueue.async {
                        self.renderingEnabled = true
                    }
                    return
                }
                self.session.beginConfiguration()
                
                // Remove the existing device input first, since using the front and back camera simultaneously is not supported.
                self.session.removeInput(self.videoDeviceInput)
                
                if self.session.canAddInput(videoDeviceInput) {
                    NotificationCenter.default.removeObserver(self, name: NSNotification.Name.AVCaptureDeviceSubjectAreaDidChange, object: currentVideoDevice)
                    NotificationCenter.default.addObserver(self, selector: #selector(self.subjectAreaDidChange), name: NSNotification.Name.AVCaptureDeviceSubjectAreaDidChange, object: videoDevice)
                    
                    self.session.addInput(videoDeviceInput)
                    self.videoDeviceInput = videoDeviceInput
                }
                else {
                    print("Could not add video device input to the session")
                    self.session.addInput(self.videoDeviceInput)
                }
                self.photoOutput.connection(withMediaType: AVMediaTypeVideo).videoOrientation = currentPhotoOrientation
                
                self.session.commitConfiguration()
            }
            
            let videoPosition = self.videoDeviceInput.device.position
            let videoOrientation = self.videoDataOutput.connection(withMediaType: AVMediaTypeVideo).videoOrientation
            let rotation = FilterMetalView.Rotation(with: interfaceOrientation, videoOrientation: videoOrientation, cameraPosition: videoPosition)
            
            DispatchQueue.main.async {
                self.cameraButton.isEnabled = true
                self.photoButton.isEnabled = true
                self.stackButton.isEnabled = true
                self.previewView.mirroring = (videoPosition == .front)
                if let rotation = rotation {
                    self.previewView.rotation = rotation
                }
                self.videoDataOutputQueue.async {
                    self.renderingEnabled = true
                }
            }
        }
    }
    
    /**
     Called when the calibration process is completed.
     */
    func onComplete(_ rms: Float, params: calibParameters) {
        self.isCalibrating = false
        DispatchQueue.main.async {
            self.textView.isHidden = true
            self.progressView.isHidden = true
            self.spinnerView.stopAnimating()
            
            let alert = UIAlertController(
                title: "Save",
                message: "Reprojection Error = \(rms)",
                preferredStyle: .alert
            )
            
            alert.addAction(UIAlertAction(
                title: "Use",
                style: .default)
            { (_) in
                self.navigationItem.rightBarButtonItem?.isEnabled = true
                calibParameters.saveToFile(params)
            })
            
            alert.addAction(UIAlertAction(
                title: "Retry",
                style: .destructive)
            { (_) in
                self.calibrator = ocvCalibrator(delegate: self)
            })
            
            self.present(alert, animated: true, completion: nil)
        }
    }
    
    
    
    
    func timeNow () -> Float {
        let date = Date()
        let calendar = Calendar.current
        return Float(calendar.component(.nanosecond, from: date))
    }
    
    func convertCIImageToCGImage(inputImage: CIImage) -> CGImage! {
        let context = CIContext(options: nil)
        return context.createCGImage(inputImage, from: inputImage.extent)
    }
    
    @IBAction private func capturePhoto(_ photoButton: UIButton) {
        
        
        print("O")
        let photoSettings = AVCapturePhotoSettings(format: [kCVPixelBufferPixelFormatTypeKey as String: Int(kCVPixelFormatType_32BGRA)])
        
        if let inputDevice = self.videoDeviceInput?.device,
            inputDevice.position == .front,
            inputDevice.isFlashAvailable,
            self.photoOutput.supportedFlashModes.contains(NSNumber(value: AVCaptureFlashMode.on.rawValue)) {
            photoSettings.flashMode = .auto
        } else {
            photoSettings.flashMode = .auto
        }
        sessionQueue.async {
            self.photoOutput.capturePhoto(with: photoSettings, delegate: self)
        }
    }
    
    // MARK: Video Data Output Delegate
    
    func captureOutput(_ captureOutput: AVCaptureOutput!, didOutputSampleBuffer sampleBuffer: CMSampleBuffer!, from connection: AVCaptureConnection!) {
        if !renderingEnabled {
            return
        }
        
        
        if let pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer) {
            
            let srcDimensions = CMVideoDimensions(width: Int32(CVPixelBufferGetWidth(pixelBuffer)), height: Int32(CVPixelBufferGetHeight(pixelBuffer)))
            
            if (videoDimensions != srcDimensions)
            {
                videoDimensions = srcDimensions
            }
            
            DispatchQueue.main.async {
                if let currentLensPosition = self.currentLensPosition() {
                    let slider_width : Float = Float(self.slider.frame.width)
                    self.lensPositionAtCapturedView.layer.position.x = CGFloat(currentLensPosition * slider_width)
                    self.lensPositionAtCapturedView.isHidden = false
                    self.lensPositionAtCapturedView.setNeedsDisplay()
                    if self.isCalibrating && self.distanceCalStatus >= 0 && self.distanceCalStatus < self.distanceCalSeq.count{
                        self.LPDisplay.text = String(format: "[%2d ] = %1.2f", self.distanceCalSeq[self.distanceCalStatus],
                                                     currentLensPosition)
                    }
                    else {
                        let rms = self.temporalWindow.addSample(value: currentLensPosition)
                        self.LPDisplay.text = String(format: "[%1.2f ] = %1.2f", rms,currentLensPosition)
                    }
                    
                }
                
                //                if self.focusTrackingStatus >= 0 && self.focusTrackingStatus < self.focusScanSeq.count
                //                {
                //                    let timestamp = CMSampleBufferGetPresentationTimeStamp(sampleBuffer!)
                //                    self.updateFramerateAtTimestamp(timestamp)
                //                    print("Frame Time: \(timestamp)")
                //                    self.lockFocusAtLensPosition(CGFloat(self.focusScanSeq[self.focusTrackingStatus]))
                //                    self.focusTrackingStatus += 1
                //                }
                
                let cameraImage = CIImage(cvPixelBuffer: pixelBuffer)
                let gImage = self.convertCIImageToCGImage(inputImage: cameraImage)
                
                var cvTextureOut: CVMetalTexture?
                
                //                var timestamp = CMSampleBufferGetPresentationTimeStamp(sampleBuffer!)
                //                var cs : colorStats_t = self.cvlink.getColorStats(UIImage(cgImage : gImage!), &self.colorStatsROI,&timestamp)
                //                print("\(cs.means.0),  \(cs.means.1), \(cs.means.2)")
                
                
                
                CVMetalTextureCacheCreateTextureFromImage(kCFAllocatorDefault, self.textureCache, pixelBuffer, nil, .bgra8Unorm,
                                                          Int(self.videoDimensions.width),
                                                          Int(self.videoDimensions.height),
                                                          0, &cvTextureOut)
                guard let cvTexture = cvTextureOut, let texture = CVMetalTextureGetTexture(cvTexture) else {
                    print("Failed to create preview texture")
                    return
                }
                
                self.previewView.texture = texture
            }
        }
    }
    
    
    
    // MARK: Photo Output Delegate
    
    func capture(_ captureOutput: AVCapturePhotoOutput, didFinishCaptureForResolvedSettings resolvedSettings: AVCaptureResolvedPhotoSettings, error: Error?) {
        print("|");
    }
    
    func capture(_ captureOutput: AVCapturePhotoOutput, didFinishProcessingPhotoSampleBuffer photoSampleBuffer: CMSampleBuffer?, previewPhotoSampleBuffer: CMSampleBuffer?, resolvedSettings: AVCaptureResolvedPhotoSettings, bracketSettings: AVCaptureBracketedStillImageSettings?, error: Error?) {
        
        if let pixelPhotoBuffer = CMSampleBufferGetImageBuffer(photoSampleBuffer!){
            
            if photoDimensions != resolvedSettings.photoDimensions
            {
                photoDimensions = resolvedSettings.photoDimensions
            }
            
            
            
            print("-")
            
            let timestamp = CMSampleBufferGetPresentationTimeStamp(photoSampleBuffer!)
            self.updateFramerateAtTimestamp(timestamp)
            
            // @note training on photo just taken and possible graphics output
            //        self.syncQueue.sync {
            //            let cameraImage = CIImage(cvPixelBuffer: pixelPhotoBuffer)
            //            let gImage = self.convertCIImageToCGImage(inputImage: cameraImage)
            //            self.cvlink.train(UIImage(cgImage : gImage!), &timestamp);
            //            locationMarkerView.isSelected = true;
            //            locationMarkerView.setNeedsDisplay();
            //        }
            let attachments = CMCopyDictionaryOfAttachments(kCFAllocatorDefault, photoSampleBuffer!, kCMAttachmentMode_ShouldPropagate)
            
            processingQueue.async {
                guard let jpegData = CameraViewController.jpegData(withPixelBuffer: pixelPhotoBuffer, attachments: attachments) else {
                    return
                }
                PHPhotoLibrary.requestAuthorization { status in
                    if status == .authorized {
                        PHPhotoLibrary.shared().performChanges({
                            let creationRequest = PHAssetCreationRequest.forAsset()
                            creationRequest.addResource(with: .photo, data: jpegData, options: nil)
                        }, completionHandler: { success, error in
                            if let error = error {
                                print("Error occurred while saving photo to photo library: \(error)")
                            }
                        }
                        )
                    }
                }
            }
        }
    }
    
    // MARK: Focus View
    
    
    
    
    
    @IBAction func sillyDidChangeValue(_ sender:UISlider) {
        self.lockFocusAtLensPosition(CGFloat(sender.value))
    }
    
    @IBAction func modeSwitchDidChangeValue(_ sender: UISwitch) {
        if sender.isOn {
            self.enableContinuousAutoFocus()
        }
        else{
            self.enableAutoFocus()
        }
        slider.isEnabled = !sender.isOn
    }
    
    
    
    func setInitialValues() {
        if isViewLoaded  {
            let autoFocus = self.isAutoFocusEnabled()
            modeSwitch.isOn = autoFocus
            slider.isEnabled = !autoFocus
            
            
            if let currentLensPosition = self.currentLensPosition() {
                slider.value = currentLensPosition
            }
        }
    }
    
    // MARK: Utilities
    
    private func updateFramerateAtTimestamp(_ timestamp: CMTime) {
        _previousSecondTimestamps.append(timestamp)
        
        let oneSecond = CMTimeMake(1, 1)
        let oneSecondAgo = CMTimeSubtract(timestamp, oneSecond)
        
        while _previousSecondTimestamps[0] < oneSecondAgo {
            _previousSecondTimestamps.remove(at: 0)
        }
        
        if _previousSecondTimestamps.count > 1 {
            let duration = CMTimeGetSeconds(CMTimeSubtract(_previousSecondTimestamps.last!, _previousSecondTimestamps[0]))
            let newRate = Float64(_previousSecondTimestamps.count - 1) / duration
            self.videoFrameRate = newRate
        }
    }
    
    
    private class func jpegData(withPixelBuffer pixelBuffer: CVPixelBuffer, attachments: CFDictionary?) -> Data? {
        let ciContext = CIContext()
        let renderedCIImage = CIImage(cvImageBuffer: pixelBuffer)
        guard let renderedCGImage = ciContext.createCGImage(renderedCIImage, from: renderedCIImage.extent) else {
            print("Failed to create CGImage")
            return nil
        }
        
        guard let data = CFDataCreateMutable(kCFAllocatorDefault, 0) else {
            print("Create CFData error!")
            return nil
        }
        
        guard let cgImageDestination = CGImageDestinationCreateWithData(data, kUTTypeJPEG, 1, nil) else {
            print("Create CGImageDestination error!")
            return nil
        }
        
        CGImageDestinationAddImage(cgImageDestination, renderedCGImage, attachments)
        if CGImageDestinationFinalize(cgImageDestination) {
            return data as Data
        }
        print("Finalizing CGImageDestination error!")
        return nil
    }
    
    
    func performConfiguration(_ block: @escaping (() -> Void)) {
        sessionQueue.async { () -> Void in
            block()
        }
    }
    
    
    func performConfigurationOnCurrentCameraDevice(_ block: @escaping ((_ currentDevice:AVCaptureDevice) -> Void)) {
        if let currentDevice = self.currentCameraDevice {
            performConfiguration { () -> Void in
                var error:NSError?
                do {
                    try currentDevice.lockForConfiguration()
                    block(currentDevice)
                    currentDevice.unlockForConfiguration()
                } catch let error1 as NSError {
                    error = error1
                } catch {
                    fatalError()
                }
            }
        }
    }
    
    
    func configure60FPSDevice (_ captureDevice: AVCaptureDevice) {
        
        for vFormat in captureDevice.formats {
            var ranges = (vFormat as AnyObject).videoSupportedFrameRateRanges as! [AVFrameRateRange]
            if ranges[0].maxFrameRate == 60 {
                performConfigurationOnCurrentCameraDevice { (currentDevice) -> Void in
                    captureDevice.activeFormat = vFormat as! AVCaptureDeviceFormat
                    captureDevice.activeVideoMinFrameDuration = ranges[0].minFrameDuration
                    captureDevice.activeVideoMaxFrameDuration = ranges[0].maxFrameDuration
                }
            }
        }
    }
    
    func configureSlowMoDevice (_ captureDevice: AVCaptureDevice) {
        
        for vFormat in captureDevice.formats {
            var ranges = (vFormat as AnyObject).videoSupportedFrameRateRanges as! [AVFrameRateRange]
            if ranges[0].maxFrameRate == 240 {
                performConfigurationOnCurrentCameraDevice { (currentDevice) -> Void in
                    captureDevice.activeFormat = vFormat as! AVCaptureDeviceFormat
                    captureDevice.activeVideoMinFrameDuration = ranges[0].minFrameDuration
                    captureDevice.activeVideoMaxFrameDuration = ranges[0].maxFrameDuration
                }
            }
        }
    }
    
    
    
    func configureFaceDetection() {
        performConfiguration { () -> Void in
            self.metadataOutput = AVCaptureMetadataOutput()
            self.metadataOutput.setMetadataObjectsDelegate(self as! AVCaptureMetadataOutputObjectsDelegate, queue: self.sessionQueue)
            
            if self.session.canAddOutput(self.metadataOutput) {
                self.session.addOutput(self.metadataOutput)
            }
            
            if (self.metadataOutput.availableMetadataObjectTypes as! [NSString]).contains(AVMetadataObjectTypeFace as NSString) {
                self.metadataOutput.metadataObjectTypes = [AVMetadataObjectTypeFace]
            }
        }
    }
    
    
    func observeValues() {
        currentCameraDevice?.addObserver(self, forKeyPath: "lensPosition", options: .new, context: &lensPositionContext)
        currentCameraDevice?.addObserver(self, forKeyPath: "focusMode", options: .new, context: &focusModeContext)
        currentCameraDevice?.addObserver(self, forKeyPath: "adjustingFocus", options: .new, context: &adjustingFocusContext)
        currentCameraDevice?.addObserver(self, forKeyPath: "FrameRate", options: .new, context: &adjustingFrameRateContext)
    }
    
    
    func unobserveValues() {
        currentCameraDevice?.removeObserver(self, forKeyPath: "lensPosition", context: &lensPositionContext)
        currentCameraDevice?.removeObserver(self, forKeyPath: "focusMode", context: &focusModeContext)
        currentCameraDevice?.removeObserver(self, forKeyPath: "adjustingFocus", context: &adjustingFocusContext)
        currentCameraDevice?.removeObserver(self, forKeyPath: "FrameRate", context: &adjustingFrameRateContext)
    }
    
    
    func showAccessDeniedMessage() {
        
    }
    
    
    func notifyObservers(_ key:String, value:AnyObject) {
        if let lensPositionObservers = controlObservers[key] {
            for obj in lensPositionObservers as [AnyObject] {
                if let observer = obj as? CameraSettingValueObserver {
                    notifyObserver(observer, setting: key, value: value)
                }
            }
        }
    }
    
    
    func notifyObserver<T>(_ observer:T, setting:String, value:AnyObject) where T:CameraSettingValueObserver {
        observer.cameraSetting(setting, valueChanged: value)
    }
    
    
    func currentFrameRate() -> Float? {
        if let frameDuration = currentCameraDevice?.activeVideoMinFrameDuration {
            return 1.0 / Float(CMTimeGetSeconds(frameDuration))
        }
        else {
            return nil
        }
    }
    
    func currentLensPosition() -> Float? {
        return self.currentCameraDevice?.lensPosition
    }
    
    func isAdjustingFocus() -> Bool? {
        let t0 = currentLensPosition ()
        let t1 = currentLensPosition ()
        return abs( t0! - t1!) > 0.01
    }
    
    
    // MARK: Focus
    func enableAutoFocus() {
        performConfigurationOnCurrentCameraDevice { (currentDevice) -> Void in
            if currentDevice.isFocusModeSupported(.autoFocus) {
                currentDevice.focusMode = .autoFocus
            }
        }
    }
    
    
    func isAutoFocusEnabled() -> Bool {
        return currentCameraDevice!.focusMode == .autoFocus
    }
    
    func isAutoFocusModeLockedAtLens() -> Bool {
        return currentCameraDevice!.focusMode == .locked
    }
    
    func enableContinuousAutoFocus() {
        performConfigurationOnCurrentCameraDevice { (currentDevice) -> Void in
            if currentDevice.isFocusModeSupported(.continuousAutoFocus) {
                currentDevice.focusMode = .continuousAutoFocus
            }
        }
    }
    
    
    func isContinuousAutoFocusEnabled() -> Bool {
        return currentCameraDevice!.focusMode == .continuousAutoFocus
    }
    
    func focusPointOfInterest() -> (CGPoint) {
        return (currentCameraDevice?.focusPointOfInterest)!
    }
    
    func setFocusPointOfInterest (_ focusPoint:CGPoint)
    {
        performConfigurationOnCurrentCameraDevice { (currentDevice) -> Void in
            if currentDevice.isFocusPointOfInterestSupported {
                currentDevice.focusPointOfInterest = focusPoint
            }
        }
    }
    
    
    
    func lockFocusAtLensPosition(_ lensPosition:CGFloat) {
        performConfigurationOnCurrentCameraDevice { (currentDevice) -> Void in
            if currentDevice.isFocusModeSupported(.locked) {
                currentDevice.setFocusModeLockedWithLensPosition(Float(lensPosition)) {
                    (time:CMTime) -> Void in
                    // timestamp of the first image buffer with the applied lens position
                    print("Focus applied @ \(time)     \(lensPosition)")
                }
            }
        }
    }
    
    
    func autoUpdatePointOfInterest() {
        var poi:CGPoint = (currentCameraDevice?.focusPointOfInterest)!
        var newy = poi.y + 0.1
        if newy > 1 {
            newy = 0
        }
        poi.y = newy;
        focusMarkerView.center.x = poi.x * self.previewView.layer.bounds.width
        focusMarkerView.center.y = poi.y * self.previewView.layer.bounds.height
        
        resetFocusAndExposure(with: .continuousAutoFocus, exposureMode: .continuousAutoExposure, at: poi, monitorSubjectAreaChange: false)
        if session.isRunning {
            let when = DispatchTime.now() + Double(Int64(1000  * Double(USEC_PER_SEC))) / Double(NSEC_PER_SEC)
            DispatchQueue.main.asyncAfter(deadline: when, execute: {
                self.autoUpdatePointOfInterest()
            })
        }
    }
    
    // Note: autoUpdateLensPosition does not user resetFocus and instead it directly uses the device API
    func autoUpdateLensPosition() {
        if self.focusTrackingStatus < 0 || self.focusTrackingStatus == self.focusScanSeq.count
        {
            return
        }
        
        self.lockFocusAtLensPosition(CGFloat(self.focusScanSeq[self.focusTrackingStatus]))
        if session.isRunning {
            let when = DispatchTime.now() + Double(Int64(500 * Double(USEC_PER_SEC))) / Double(NSEC_PER_SEC)
            DispatchQueue.main.asyncAfter(deadline: when, execute: {
                self.autoUpdateLensPosition()
            })
        }
    }
    
    
    func currentAutoFocusSystem () -> String {
        
        for vFormat in (self.currentCameraDevice?.formats)! {
            let afs = (vFormat as AnyObject).autoFocusSystem
            if afs == AVCaptureAutoFocusSystem.contrastDetection
            {
                return " CTR "
            }
            else if afs == AVCaptureAutoFocusSystem.phaseDetection
            {
                return " PHZ "
            }
        }
        return "   "
    }
    
    func smoothAutoFocusEnabled () -> Bool {
        return (self.currentCameraDevice?.isSmoothAutoFocusEnabled)!
    }
    
    func smoothAutoFocusSupported () -> Bool {
        return self.currentCameraDevice!.isSmoothAutoFocusSupported
    }
    
    func displayCurrentValues() {
        var currentValuesTextComponents = [String]()
        
        if let lensPosition = self.currentLensPosition() {
            currentValuesTextComponents.append(String(format: "F: %.2f", lensPosition))
        }
        
        let afs = self.currentAutoFocusSystem()
        currentValuesTextComponents.append(String(format: "AFS: %@", afs))
        
        if let fps = self.currentFrameRate() {
            currentValuesTextComponents.append(String(format: "FPS: %.3f", fps))
        }
        
        currentInfoLabel.text = currentValuesTextComponents.joined(separator: " - ")
    }
    
}



public func labelWithNumber (label: String, value: Int) -> String {
    return label + String(value)
}

extension Timer {
    /**
     Creates and schedules a one-time `NSTimer` instance.
     
     :param: delay The delay before execution.
     :param: handler A closure to execute after `delay`.
     
     :returns: The newly-created `NSTimer` instance.
     */
    class func schedule(delay: TimeInterval, handler: @escaping (Timer?) -> Void) -> Timer {
        let fireDate = delay + CFAbsoluteTimeGetCurrent()
        let timer = CFRunLoopTimerCreateWithHandler(kCFAllocatorDefault, fireDate, 0, 0, 0, handler)!
        CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, CFRunLoopMode.commonModes)
        return timer
    }
    
    /**
     Creates and schedules a repeating `NSTimer` instance.
     
     :param: repeatInterval The interval between each execution of `handler`. Note that individual calls may be delayed; subsequent calls to `handler` will be based on the time the `NSTimer` was created.
     :param: handler A closure to execute after `delay`.
     
     :returns: The newly-created `NSTimer` instance.
     */
    class func schedule(repeatInterval interval: TimeInterval, handler: @escaping (Timer?) -> Void) -> Timer {
        let fireDate = interval + CFAbsoluteTimeGetCurrent()
        let timer = CFRunLoopTimerCreateWithHandler(kCFAllocatorDefault, fireDate, interval, 0, 0, handler)!
        CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, CFRunLoopMode.commonModes)
        return timer
    }
}

extension UIInterfaceOrientation {
    var videoOrientation: AVCaptureVideoOrientation? {
        switch self {
        case .portrait: return .portrait
        case .portraitUpsideDown: return .portraitUpsideDown
        case .landscapeLeft: return .landscapeLeft
        case .landscapeRight: return .landscapeRight
        default: return nil
        }
    }
}

extension FilterMetalView.Rotation {
    init?(with interfaceOrientation: UIInterfaceOrientation, videoOrientation: AVCaptureVideoOrientation, cameraPosition: AVCaptureDevicePosition) {
        /*
         Calculate the rotation between the videoOrientation and the interfaceOrientation.
         The direction of the rotation depends upon the camera position.
         */
        switch videoOrientation {
        case .portrait:
            switch interfaceOrientation {
            case .landscapeRight:
                if cameraPosition == .front {
                    self = .rotate90Degrees
                }
                else {
                    self = .rotate270Degrees
                }
                
            case .landscapeLeft:
                if cameraPosition == .front {
                    self = .rotate270Degrees
                }
                else {
                    self = .rotate90Degrees
                }
                
            case .portrait:
                self = .rotate0Degrees
                
            case .portraitUpsideDown:
                self = .rotate180Degrees
                
            default: return nil
            }
        case .portraitUpsideDown:
            switch interfaceOrientation {
            case .landscapeRight:
                if cameraPosition == .front {
                    self = .rotate270Degrees
                }
                else {
                    self = .rotate90Degrees
                }
                
            case .landscapeLeft:
                if cameraPosition == .front {
                    self = .rotate90Degrees
                }
                else {
                    self = .rotate270Degrees
                }
                
            case .portrait:
                self = .rotate180Degrees
                
            case .portraitUpsideDown:
                self = .rotate0Degrees
                
            default: return nil
            }
            
        case .landscapeRight:
            switch interfaceOrientation {
            case .landscapeRight:
                self = .rotate0Degrees
                
            case .landscapeLeft:
                self = .rotate180Degrees
                
            case .portrait:
                if cameraPosition == .front {
                    self = .rotate270Degrees
                }
                else {
                    self = .rotate90Degrees
                }
                
            case .portraitUpsideDown:
                if cameraPosition == .front {
                    self = .rotate90Degrees
                }
                else {
                    self = .rotate270Degrees
                }
                
            default: return nil
            }
            
        case .landscapeLeft:
            switch interfaceOrientation {
            case .landscapeLeft:
                self = .rotate0Degrees
                
            case .landscapeRight:
                self = .rotate180Degrees
                
            case .portrait:
                if cameraPosition == .front {
                    self = .rotate90Degrees
                }
                else {
                    self = .rotate270Degrees
                }
                
            case .portraitUpsideDown:
                if cameraPosition == .front {
                    self = .rotate270Degrees
                }
                else {
                    self = .rotate90Degrees
                }
                
            default: return nil
            }
        }
    }
}

