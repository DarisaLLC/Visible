/*
	Copyright (C) 2016 Apple Inc. All Rights Reserved.
	See LICENSE.txt for this sample’s licensing information
	
	Abstract:
	Filter renderer protocol.
*/

import Foundation
import CoreMedia

protocol FilterRenderer {
	//	Prepare resources.
	//	The outputRetainedBufferCountHint tells out of place renderers how many of their output buffers will be held onto by the downstream pipeline at one time.
	//	This can be used by the renderer to size and preallocate their pools.
	func prepare(with inputFormatDescription: CMFormatDescription, outputRetainedBufferCountHint: Int)
	
	// Release resources.
	func reset()
	
	// The format description of the output pixel buffers.
	var outputFormatDescription: CMFormatDescription? { get }
	
	// The format description of the input pixel buffers.
	var inputFormatDescription: CMFormatDescription? { get }
	
	// Render pixel buffer.
	func render(pixelBuffer: CVPixelBuffer) -> CVPixelBuffer?
}
