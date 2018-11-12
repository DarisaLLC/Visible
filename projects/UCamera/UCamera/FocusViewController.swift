
import UIKit
import AVFoundation

class FocusViewController: UIViewController, CameraControlsViewControllerProtocol, CameraSettingValueObserver {

	@IBOutlet var modeSwitch:UISwitch!
	@IBOutlet var slider:UISlider!
	
	var cameraController:CameraController? {
		willSet {
			if let cameraController = cameraController {
                cameraController.unregisterObserver(self, property: CameraControlObservableSettingLensPosition)
                cameraController.unregisterObserver(self, property: CameraControlObservableSettingBlobMatch)
                cameraController.unregisterObserver(self, property: CameraControlObservableSettingFocusMode)
			}
		}
		didSet {
			if let cameraController = cameraController {
                cameraController.registerObserver(self, property: CameraControlObservableSettingLensPosition)
                cameraController.registerObserver(self, property: CameraControlObservableSettingBlobMatch)
                cameraController.registerObserver(self, property: CameraControlObservableSettingFocusMode)
			}
		}
	}
	
	override func viewDidLoad() {
		setInitialValues()
	}
	
	
	@IBAction func sliderDidChangeValue(_ sender:UISlider) {
		cameraController?.lockFocusAtLensPosition(CGFloat(sender.value))
	}
	
	
	@IBAction func modeSwitchValueChanged(_ sender:UISwitch) {
		if sender.isOn {
			cameraController?.enableContinuousAutoFocus()
		}
		else {
            cameraController?.enableAutoFocus()
		}
		slider.isEnabled = !sender.isOn
	}

	
	func cameraSetting(_ setting:String, valueChanged value:AnyObject) {
		if setting == CameraControlObservableSettingLensPosition {
			if let lensPosition = value as? Float {
				slider.value = lensPosition
            }
        }
        if setting == CameraControlObservableSettingFocusMode {
            if let autoFocus = value as? Bool {
                modeSwitch.isOn = autoFocus
                slider.isEnabled = !autoFocus
            }
        }

	}

	
	func setInitialValues() {
		if isViewLoaded && cameraController != nil {
			if let autoFocus = cameraController?.isAutoFocusEnabled() {
				modeSwitch.isOn = autoFocus
				slider.isEnabled = !autoFocus
			}
			
			if let currentLensPosition = cameraController?.currentLensPosition() {
				slider.value = currentLensPosition
			}
		}
	}
}
