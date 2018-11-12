//
//  Central.swift
//  Camera2
//
//  Created by Arman Garakani on 6/4/17.
//  Copyright © 2017 apple. All rights reserved.
//

import Foundation

import Foundation
import Metal

class Central {
    
    let scaleFactor : Float = 1.2
    var minNeighbors : Int32 = 3
    let histogramEqualization : Bool = false
    let initialScale : Float = 0.5
    var step : Int32 = 2
    
    let minSize : Float = 360
    let maxSize : Float = 720
    
    var scaledWidth : Int
    var scaledHeight : Int
    var width : Int
    var height : Int
    
    var drawingPipeline : MetalComputePipeline! = nil
    
    static let sharedInstance = Central ()
    
   
    var CentralSetDrawDispatchArguments : MTLComputePipelineState
    var CentralDraw : MTLComputePipelineState
    
    var maxNumRects = 1500
    
    init ()
    {
        self.CentralSetDrawDispatchArguments = Context.makeComputePipeline(name: "CentralSetDrawDispatchArguments")
        self.CentralDraw = Context.makeComputePipeline(name: "CentralDraw")
        self.width = 1
        self.height = 2
        scaledWidth = Int(Float(width) * initialScale)
        scaledHeight = Int(Float(height) * initialScale)
        
    }

    func setSize(width: Int, height: Int) {
        
        // get the dimensions and scaled dimensions
        self.width = width
        self.height = height
        scaledWidth = Int(Float(width) * initialScale)
        scaledHeight = Int(Float(height) * initialScale)
        
        self.drawingPipeline = self.createDrawPipeline()
        
    }
    
     
    
}
