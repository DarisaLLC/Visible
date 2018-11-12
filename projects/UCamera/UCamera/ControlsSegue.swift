

import UIKit

class ControlsSegue: UIStoryboardSegue {

	var hostView:UIView?
	var currentViewController:UIViewController?
	
	override init(identifier: String!, source: UIViewController, destination: UIViewController) {
		super.init(identifier: identifier, source: source, destination: destination)
	}
	
	
	override func perform() {
		if let destinationViewController = destination as? UIViewController {
			
			if let currentControlsViewController = currentViewController {
				currentControlsViewController.willMove(toParentViewController: nil)
				currentControlsViewController.removeFromParentViewController()
				currentControlsViewController.view.removeFromSuperview()
			}

			source.addChildViewController(destinationViewController as UIViewController)
			hostView!.addSubview(destinationViewController.view)
			destinationViewController.view.frame = hostView!.bounds
			destinationViewController.didMove(toParentViewController: source as? UIViewController)
		}
	}
}
