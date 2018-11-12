//
//  SwViewController.swift
//  CVOpenStitch
//
//  Created by Foundry on 04/06/2014.
//  Copyright (c) 2014 Foundry. All rights reserved.
//

import UIKit

class SwViewController: UIViewController, UIScrollViewDelegate {
    
    @IBOutlet var spinner:UIActivityIndicatorView!
    @IBOutlet var imageView:UIImageView?
    @IBOutlet var scrollView:UIScrollView!
    
    override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
        super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
        // Custom initialization
    }
    
    required init?(coder aDecoder: NSCoder)
    {
        super.init(coder: aDecoder)
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // Do any additional setup after loading the view.
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        stitch()
    }
    
    func stitch() {
        self.spinner.startAnimating()
        DispatchQueue.global().async {
            
            let image1 = UIImage(named:"I0.jpg")
            let image2 = UIImage(named:"I1.jpg")
            let image3 = UIImage(named:"I2.jpg")
            let image4 = UIImage(named:"I3.jpg")
            let image5 = UIImage(named:"I4.jpg")
            
            let imageArray:[UIImage?] = [image1,image2,image3,image4, image5]
            
            //            let stitchedImage:UIImage = CVWrapper.stitch(with: imageArray as! [UIImage]) as UIImage
            let stitchedImage:UIImage = CVWrapper.cvProduceOutline(image1!)
            
            DispatchQueue.main.async {
                NSLog("stichedImage %@", stitchedImage)
                let imageView:UIImageView = UIImageView.init(image: stitchedImage)
                self.imageView = imageView
                self.scrollView.addSubview(self.imageView!)
                self.scrollView.backgroundColor = UIColor.black
                self.scrollView.contentSize = self.imageView!.bounds.size
                self.scrollView.maximumZoomScale = 4.0
                self.scrollView.minimumZoomScale = 0.5
                self.scrollView.delegate = self
                self.scrollView.contentOffset = CGPoint(x: -(self.scrollView.bounds.size.width - self.imageView!.bounds.size.width)/2.0, y: -(self.scrollView.bounds.size.height - self.imageView!.bounds.size.height)/2.0)
                NSLog("scrollview \(self.scrollView.contentSize)")
                self.spinner.stopAnimating()
            }
        }
    }
    
    
    func viewForZooming(in scrollView:UIScrollView) -> UIView? {
        return self.imageView!
    }
    
}
