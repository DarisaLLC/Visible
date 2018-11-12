

import UIKit
//import Fabric
//import Crashlytics

@UIApplicationMain
class AppDelegate: UIResponder, UIApplicationDelegate {
    
    var window: UIWindow?
    
    
    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplicationLaunchOptionsKey: Any]?) -> Bool {
        // Register Crashlytics, Twitter, and Digits with Fabric.
//        Fabric.with([Crashlytics.self()])
        
        return true
    }
    
}

