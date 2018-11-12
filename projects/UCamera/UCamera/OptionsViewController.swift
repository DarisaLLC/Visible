

import UIKit

class OptionsViewController: UIViewController, CameraControlsViewControllerProtocol {

	var cameraController:CameraController?
	
	@IBOutlet var bracketedCaptureSwitch:UISwitch!
	
	override func viewDidLoad() {
		if let cameraController = cameraController {
			bracketedCaptureSwitch.isOn = cameraController.enableBracketedCapture
		}
	}
	
	@IBAction func bracketedCaptureSwitchValueChanged(_ sender:UISwitch) {
		cameraController?.enableBracketedCapture = sender.isOn
	}
	
}
