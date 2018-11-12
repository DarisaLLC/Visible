//
//  imageDistance.swift
//  UCamera
//
//  Created by Arman Garakani on 7/10/17.
//  Copyright © 2017 Leo Pharma. All rights reserved.
//

import Foundation

///  Struct for calculating distance from lens position using built in calibration data or theoretical model

// 35mm equivalent Focal Length = actual Focal Length * crop factor
// actual focal length = 35mm equivalent Focal Length / crop fator
//



public func lensPosition (focalLength: Float, objectMM: Float, baseMM: Float) -> Float {

    func internalNorm (focalLength: Float, objectMM: Float) -> Float {
        assert(focalLength > 0.0)
        let oneOverF = 1 / focalLength
        let oneOverD = 1 / (objectMM + 2*focalLength)
        return 1.0 / (oneOverF - oneOverD)
    }
    
    return ((internalNorm(focalLength: focalLength, objectMM: -objectMM) -
        internalNorm(focalLength: focalLength, objectMM: -baseMM))) /
        (focalLength - internalNorm(focalLength: focalLength, objectMM: -baseMM))
}


public func objectDistance (focalLength: Float, lensPosition: Float,baseMM: Float) -> Float {
    
    func internalNorm (focalLength: Float, objectMM: Float) -> Float {
        assert(focalLength > 0.0)
        let oneOverF = 1 / focalLength
        let oneOverD = 1 / (objectMM + 2*focalLength)
        return 1.0 / (oneOverF - oneOverD)
    }
    
    let normBase = lensPosition * ( focalLength -
            internalNorm(focalLength: focalLength, objectMM: -baseMM )) +
            internalNorm(focalLength: focalLength, objectMM: -baseMM )
    return -(-2*focalLength*focalLength + normBase * focalLength) /
        (focalLength - normBase)
}


/*
 for i in 1..<50 {
 let di = objectDistance ( focalLength: 0.4, lensPosition: Float(i) / 50)
 print ("{\(Float(i) / 50), \(di)},")
 }

 {0.02, 10.1959},
 {0.04, 10.4},
 {0.06, 10.6128},
 {0.08, 10.8348},
 {0.1, 11.0667},
 {0.12, 11.3091},
 {0.14, 11.5628},
 {0.16, 11.8286},
 {0.18, 12.1073},
 {0.2, 12.4},
 {0.22, 12.7077},
 {0.24, 13.0316},
 {0.26, 13.373},
 {0.28, 13.7333},
 {0.3, 14.1143},
 {0.32, 14.5176},
 {0.34, 14.9454},
 {0.36, 15.4},
 {0.38, 15.8839},
 {0.4, 16.4},
 {0.42, 16.9517},
 {0.44, 17.5428},
 {0.46, 18.1778},
 {0.48, 18.8615},
 {0.5, 19.6},
 {0.52, 20.4},
 {0.54, 21.2695},
 {0.56, 22.2182},
 {0.58, 23.2571},
 {0.6, 24.4},
 {0.62, 25.6631},
 {0.64, 27.0666},
 {0.66, 28.6353},
 {0.68, 30.4},
 {0.7, 32.4},
 {0.72, 34.6858},
 {0.74, 37.323},
 {0.76, 40.3999},
 {0.78, 44.0363},
 {0.8, 48.4001},
 {0.82, 53.7335},
 {0.84, 60.3997},
 {0.86, 68.9712},
 {0.88, 80.3998},
 {0.9, 96.4001},
 {0.92, 120.401},
 {0.94, 160.402},
 {0.96, 240.396},
 {0.98, 480.392}, 
 for i in 1..<50 {
 let di = imageDistance(objectMM: Float(i), focalLength: 0.4)
 print ("\(i): \(di)")
 }
 : -15.0
 2: -4.99999
 3: -2.6923
 4: -1.66666
 5: -1.08696
 6: -0.714284
 7: -0.454545
 8: -0.263157
 9: -0.116277
 10: 0.0
 11: 0.0943404
 12: 0.172416
 13: 0.238096
 14: 0.294118
 15: 0.342464
 16: 0.384616
 17: 0.421686
 18: 0.454545
 19: 0.483872
 20: 0.510204
 21: 0.533981
 22: 0.555555
 23: 0.575221
 24: 0.59322
 25: 0.609757
 26: 0.624999
 27: 0.639098
 28: 0.652175
 29: 0.664336
 30: 0.675675
 31: 0.686275
 32: 0.696203
 33: 0.705521
 34: 0.714286
 35: 0.722544
 36: 0.730338
 37: 0.737703
 38: 0.744681
 39: 0.751293
 40: 0.757575
 41: 0.763546
 42: 0.769232
 43: 0.774648
 44: 0.779818
 45: 0.784753
 46: 0.789474
 47: 0.79399
 48: 0.79832
 49: 0.802468
 
 
 */
