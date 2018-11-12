//
//  MovingRMS.swift
//  UCamera
//
//  Created by Arman Garakani on 7/20/17.
//  Copyright © 2017 Leo Pharma. All rights reserved.
//

import Foundation

class MovingRMS {
    var samples: Array<Float>
    var sampleCount = 0
    var period = 5
    
    init(period: Int = 5) {
        self.period = period
        samples = Array<Float>()
    }
    
    var rms: Float {
        let squares = samples.map {value in value * value}
        let sum: Float = squares.reduce(0, +)
        
        if period > samples.count {
            return sqrt(sum / Float(samples.count))
        } else {
            return sqrt(sum / Float(period))
        }
    }
    
    func addSample(value: Float) -> Float {
        self.sampleCount += 1
        var pos = Int(fmodf(Float(sampleCount), Float(period)))
        
        if pos >= samples.count {
            samples.append(value)
        } else {
            samples[pos] = value
        }
        
        return rms
    }
}
