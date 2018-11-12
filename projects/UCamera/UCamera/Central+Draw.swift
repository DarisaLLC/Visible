//
//  Camera2+Draw.swift
//  Camera2
//
//  Created by Arman Garakani on 6/4/17.
//  Copyright © 2017 apple. All rights reserved.
//

import Foundation
import Metal

extension Central {
    
    func createDrawPipeline() -> MetalComputePipeline {
        
  
        
        let pDraw = MetalComputeShader(name: "DrawingDraw", pipeline: "RectangleDraw")
        pDraw.addBuffer(Float32(1.0))
        pDraw.addTextureAtInput(0) // this basically references a texture thats being inputed into the pipeline
        let drawPipeline = MetalComputePipeline(name: "DrawingPipeline")
        drawPipeline.addShader(pDraw)
        
        return drawPipeline
    }
    
    func draw(texture: Texture, commandBuffer: MTLCommandBuffer) {
        self.drawingPipeline.setInputTextures([texture])
        self.drawingPipeline.encode(commandBuffer)
    }
    
    
}
