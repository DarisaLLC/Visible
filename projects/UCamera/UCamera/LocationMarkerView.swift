//
//  LocationMarkerView.swift
//  Camera2
//
//  Created by Arman Garakani on 6/4/17.
//  Copyright © 2017 apple. All rights reserved.
//


import UIKit
import CoreGraphics


class LocationMarkerView:   UIView
{
    
    @IBInspectable var mainColor: UIColor = UIColor.blue
        {
        didSet { print("mainColor was set here") }
    }
    @IBInspectable var ringColor: UIColor = UIColor.orange
        {
        didSet { print("bColor was set here") }
    }
    @IBInspectable var ringThickness: CGFloat = 4
        {
        didSet { print("ringThickness was set here") }
    }
    
    @IBInspectable var isSelected: Bool = false
    
    override func draw(_ rect: CGRect)
    {
        let dotPath = UIBezierPath(ovalIn:rect)
        let shapeLayer = CAShapeLayer()
        shapeLayer.path = dotPath.cgPath
        shapeLayer.fillColor = mainColor.cgColor
        layer.addSublayer(shapeLayer)
        self.layer.opacity = 0.3
        if (isSelected) { drawRingFittingInsideView(rect: rect) }
    }
    
    internal func drawRingFittingInsideView(rect: CGRect)->()
    {
        let hw:CGFloat = ringThickness/2
        let circlePath = UIBezierPath(ovalIn: rect.insetBy(dx: hw,dy: hw) )
        let shapeLayer = CAShapeLayer()
        shapeLayer.path = circlePath.cgPath
        shapeLayer.fillColor = UIColor.clear.cgColor
        shapeLayer.strokeColor = ringColor.cgColor
        shapeLayer.lineWidth = ringThickness
        self.layer.opacity = 0.3
        layer.addSublayer(shapeLayer)
    }
}
