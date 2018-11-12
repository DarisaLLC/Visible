

import UIKit
import AVFoundation

class PreviewLayerView : UIView {
	override class var layerClass : AnyClass {
		return AVCaptureVideoPreviewLayer.self
	}
}



class PreviewLayerViewController: UIViewController, CameraPreviewViewController {
	
	var previewLayer:AVCaptureVideoPreviewLayer! {
		get {
			return view.layer as! AVCaptureVideoPreviewLayer
		}
	}
	
	var cameraController:CameraController? {
		didSet {
			cameraController?.previewLayer = previewLayer
		}
	}

	override func loadView() {
		view = PreviewLayerView()
	}
	
	override func viewDidLoad() {
		super.viewDidLoad()
	}
}
