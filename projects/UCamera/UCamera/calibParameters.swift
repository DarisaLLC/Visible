//
//  calibParameters.swift
//  Camera
//
//  Created by Arman Garakani on 6/28/17.
//  Copyright © 2017 Leo Pharma. All rights reserved.
//
import Foundation

enum calibParametersError: Error {
    case missingKey
    case invalidType
}

@objc class calibParameters : NSObject {
    // Focal distance & principal point.
    let fx: Float
    let fy: Float
    let cx: Float
    let cy: Float
    
    // Distortion.
    let k1: Float
    let k2: Float
    let r1: Float
    let r2: Float
    
    // Camera focal distance [0.0, 1.0f]
    let f: Float
    
    @objc public required init(
        fx: Float,
        fy: Float,
        cx: Float,
        cy: Float,
        k1: Float,
        k2: Float,
        r1: Float,
        r2: Float,
        f: Float)
    {
        self.fx = fx
        self.fy = fy
        self.cx = cx
        self.cy = cy
        self.k1 = k1
        self.k2 = k2
        self.r1 = r1
        self.r2 = r2
        self.f = f
    }
    
    /**
     Loads the calibration parameters from a file.
     */
    @objc open static func loadFromFile() throws -> calibParameters {
        
        let data = try Data(contentsOf: getParametersFileURL(), options: NSData.ReadingOptions())
        let json = try JSONSerialization.jsonObject(  
            with: data,
            options: JSONSerialization.ReadingOptions()
            ) as! [String: AnyObject]
        
        let fetch: (String) throws -> Float = { (key) throws in
            guard let val = json[key] as? Float else {
                throw calibParametersError.missingKey
            }
            return val
        }
        
        return calibParameters(
            fx: try fetch("fx"),
            fy: try fetch("fy"),
            cx: try fetch("cx"),
            cy: try fetch("cy"),
            k1: try fetch("k1"),
            k2: try fetch("k2"),
            r1: try fetch("r1"),
            r2: try fetch("r2"),
            f:  try fetch("f")
        )
    }
    
    /**
     Saves the calibration file.
     */
    @objc open static func saveToFile(_ params: calibParameters) {
        try? (try! JSONSerialization.data(withJSONObject: [
            "fx": params.fx,
            "fy": params.fy,
            "cx": params.cx,
            "cy": params.cy,
            "k1": params.k1,
            "k2": params.k2,
            "r1": params.r1,
            "r2": params.r2,
            "f":  params.f
            ],
                                          options: JSONSerialization.WritingOptions()
            )).write(to: getParametersFileURL(), options: [.atomic])
    }
    
    /**
     Returns the URL to the file storing the parameters.
     */
    static func getParametersFileURL() -> URL {
        return FileManager.default.urls(
            for: .documentDirectory,
            in: .userDomainMask
            )[0].appendingPathComponent("camera_calib_params.json")
    }
}
