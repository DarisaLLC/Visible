
import Foundation
import UIKit
import GLKit
import CoreImage
import OpenGLES




class GLPreviewViewController: UIViewController, CameraPreviewViewController, CameraFramesDelegate {
    
    
    var cameraController:CameraController? {
        didSet {
            cameraController?.framesDelegate = self
        }
    }
    
    fileprivate var glContext:EAGLContext?
    fileprivate var ciContext:CIContext?
    fileprivate var renderBuffer:GLuint = GLuint()
    
    fileprivate var filter = CIFilter(name:"CIPhotoEffectMono")
    
    fileprivate var glView:GLKView {
        get {
            return view as! GLKView
        }
    }
    
    override func loadView() {
        self.view = GLKView()
    }
    
    
    fileprivate func overlayImageMotionCenter(_ image: CIImage, motionCenter: CGPoint, rectColor: UIColor, pad: CGFloat) -> CIImage! {
        var overlay = CIImage(color: CIColor(color: rectColor))
        overlay = overlay.cropping(to: image.extent)
        
        let tl = CGPoint (x: motionCenter.x - pad, y: motionCenter.y - pad)
        let tr = CGPoint(x: motionCenter.x + pad, y: motionCenter.y - pad)
        let bl = CGPoint(x: motionCenter.x - pad, y: motionCenter.y + pad)
        let br = CGPoint(x: motionCenter.x + pad, y: motionCenter.y + pad)
        
        overlay = overlay.applyingFilter("CIPerspectiveTransformWithExtent", withInputParameters: ["inputExtent":     CIVector(cgRect: image.extent),
                                                                                                   "inputTopLeft":    CIVector(cgPoint: tl),
                                                                                                   "inputTopRight":   CIVector(cgPoint: tr),
                                                                                                   "inputBottomLeft": CIVector(cgPoint: bl),
                                                                                                   "inputBottomRight": CIVector(cgPoint: br)])
        return overlay.compositingOverImage(image)
    }

    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        glContext = EAGLContext(api: .openGLES2)
        
        glView.context = glContext!
        glView.transform = CGAffineTransform(rotationAngle: CGFloat(Double.pi/2))
        if let window = glView.window {
            glView.frame = window.bounds
        }
        
        ciContext = CIContext(eaglContext: glContext!)
        
    }
    
    
    
    // MARK: CameraControllerDelegate
    
    func cameraController(_ cameraController: CameraController, didOutputImage image: CIImage, didDetectLocs locs:Array<(id:Int,point:CGPoint)>)
    {
        if glContext != EAGLContext.current() {
            EAGLContext.setCurrent(glContext)
        }
        
        glView.bindDrawable()
        
        if locs.count > 0
        {
            // Blue rectangle at the center
            let mc = CGPoint(x: image.extent.midX, y: image.extent.midY)
             let rc = UIColor(red:0.2, green:0.2, blue:0.86, alpha:0.5)
             let live_pad = CGFloat(96)
            let outputImage = self.overlayImageMotionCenter(image, motionCenter: mc, rectColor:rc, pad: live_pad)
            
//            The fromRect parameter is the portion of the image to draw in the filtered image’s coordinate space.
//            The inRect parameter is the rectangle in the coordinate space of the GL context into which the image should be drawn.
//            If you want to respect the aspect ratio of the image, you may need to do some math to compute the appropriate inRect.
//            
            ciContext?.draw(outputImage!, in:image.extent, from: image.extent)
            
            if locs.count >= 1
            {
                // Two lens positions: x is set and y is current
                let mc = CGPoint(x: image.extent.midX, y: image.extent.midY)
                let rc = UIColor(red:0.86, green:0.2, blue:0.2, alpha:0.5)
                let live_pad = CGFloat((locs.first?.point.x)! * 48)
                let output2Image = self.overlayImageMotionCenter(outputImage!, motionCenter: mc, rectColor:rc, pad: live_pad)
                
                ciContext?.draw(output2Image!, in:image.extent, from: image.extent)
                
                let rrc = UIColor(red:0.2, green:0.8, blue:0.2, alpha:0.8)
                let live_pad2 = CGFloat((locs.first?.point.y)! * 72)
                let output3Image = self.overlayImageMotionCenter(output2Image!, motionCenter: mc, rectColor:rrc, pad: live_pad2)

                ciContext?.draw(output3Image!, in:image.extent, from: image.extent)
                
                if locs.count == 2
                {
                    let rrc = UIColor(red:1.0, green:0.8, blue:0.2, alpha:0.5)
                    let live_pad2 = CGFloat(48.0)
                    let output4Image = self.overlayImageMotionCenter(output2Image!, motionCenter: (locs.last?.point)!, rectColor:rrc, pad: live_pad2)
                    
                    ciContext?.draw(output4Image!, in:image.extent, from: image.extent)

                }
            }
            
        }
        
        glView.display()
    }
}
