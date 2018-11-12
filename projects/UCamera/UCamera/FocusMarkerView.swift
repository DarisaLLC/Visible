//
//  FocusMarkerView.swift
//  Camera2
//
//  Created by Arman Garakani on 6/4/17.
//  Copyright © 2017 apple. All rights reserved.
//

import Foundation
import UIKit
import CoreGraphics

class FocusMarkerView: UIView
{
    @IBInspectable var cornerRadius: CGFloat {
        get {
            return layer.cornerRadius
        }
        set {
            layer.cornerRadius = cornerRadius
            layer.masksToBounds = cornerRadius != 0
            setNeedsDisplay()
        }
    }
    
    
}
