//
//  CompareHelpers.swift
//  UCamera
//
//  Created by Arman Garakani on 7/17/17.
//  Copyright © 2017 Leo Pharma. All rights reserved.
//

import Foundation
import MobileCoreServices

// MARK: dimension compare
// Returns whether two dimensions  are equal.
/// - parameter lhs: The left-hand side value to compare.
/// - parameter rhs: The right-hand side value to compare.
/// - returns: `true` if the two values are equal, `false` otherwise.

extension CMVideoDimensions {
    static  func ==(
        
        lhs: CMVideoDimensions,
        
        rhs: CMVideoDimensions)
        
        -> Bool
        
    {
        
        return lhs.height == rhs.height && lhs.width == rhs.width
    }
    
    static func !=(
        
        lhs: CMVideoDimensions,
        
        rhs: CMVideoDimensions)
        
        -> Bool
        
    {
        return lhs.height != rhs.height || lhs.width != rhs.width
    }
}
