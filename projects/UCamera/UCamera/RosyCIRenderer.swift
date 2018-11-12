/*
	Copyright (C) 2016 Apple Inc. All Rights Reserved.
	See LICENSE.txt for this sample’s licensing information
	
	Abstract:
	Rosy colored filter renderer implemented with CI.
*/

import Foundation
import UIKit
import CoreMedia
import CoreVideo
import CoreImage

class RosyCIRenderer : FilterRenderer {
	private var ciContext: CIContext?
	
	private var rosyFilter: CIFilter?
	
	private var colorSpace: CGColorSpace?
	
	private var bufferPool: CVPixelBufferPool?
	
	private(set) var outputFormatDescription: CMFormatDescription?
	
	private(set) var inputFormatDescription: CMFormatDescription?
	
	private func allocateOutputBufferPool(with formatDescription: CMFormatDescription, outputRetainedBufferCountHint: Int) {
		let inputMediaSubType = CMFormatDescriptionGetMediaSubType(formatDescription)
		if (inputMediaSubType != kCVPixelFormatType_32BGRA) {
			assertionFailure("Invalid input pixel buffer type \(inputMediaSubType)")
			return;
		}
		
		let inputDimensions = CMVideoFormatDescriptionGetDimensions(formatDescription)
		var pixelBufferAttributes: [String : Any] = [
			kCVPixelBufferPixelFormatTypeKey as String: UInt(inputMediaSubType),
			kCVPixelBufferWidthKey as String: Int(inputDimensions.width),
			kCVPixelBufferHeightKey as String: Int(inputDimensions.height),
			kCVPixelFormatOpenGLESCompatibility as String: true,
			kCVPixelBufferIOSurfacePropertiesKey as String: [:]
		]
		// Get pixel buffer attributes and color space from the input format description
		var cgColorSpace = CGColorSpaceCreateDeviceRGB()
		if let inputFormatDescriptionExtension = CMFormatDescriptionGetExtensions(formatDescription) as Dictionary? {
			let colorPrimaries = inputFormatDescriptionExtension[kCVImageBufferColorPrimariesKey]
			
			if let colorPrimaries = colorPrimaries {
				var colorSpaceProperties: [String : AnyObject] = [kCVImageBufferColorPrimariesKey as String: colorPrimaries]
				
				if let yCbCrMatrix = inputFormatDescriptionExtension[kCVImageBufferYCbCrMatrixKey] {
					colorSpaceProperties[kCVImageBufferYCbCrMatrixKey as String] = yCbCrMatrix
				}
				
				if let transferFunction = inputFormatDescriptionExtension[kCVImageBufferTransferFunctionKey] {
					colorSpaceProperties[kCVImageBufferTransferFunctionKey as String] = transferFunction
				}
				
				pixelBufferAttributes[kCVBufferPropagatedAttachmentsKey as String] = colorSpaceProperties
			}
			
			if let cvColorspace = inputFormatDescriptionExtension[kCVImageBufferCGColorSpaceKey] {
				cgColorSpace = cvColorspace as! CGColorSpace
			}
			else if (colorPrimaries as? String) == (kCVImageBufferColorPrimaries_P3_D65 as String) {
				cgColorSpace = CGColorSpace(name: CGColorSpace.displayP3)!
			}
		}
		
		let poolAttributes = [kCVPixelBufferPoolMinimumBufferCountKey as String: outputRetainedBufferCountHint]
		var cvPixelBufferPool: CVPixelBufferPool?
		// Create a pixel buffer pool with the same pixel attributes as the input format description
		CVPixelBufferPoolCreate(kCFAllocatorDefault, poolAttributes as NSDictionary?, pixelBufferAttributes as NSDictionary?, &cvPixelBufferPool)
		guard let pixelBufferPool = cvPixelBufferPool else {
			assertionFailure("Failed to create the pixel buffer pool")
			return
		}
		// Preallocate buffers in the pool, since this is for real-time display/capture
		var pixelBuffers = [CVPixelBuffer]()
		var error: CVReturn = kCVReturnSuccess
		let auxAttributes = [kCVPixelBufferPoolAllocationThresholdKey as String: outputRetainedBufferCountHint] as NSDictionary

		var pixelBuffer: CVPixelBuffer?
		while (error == kCVReturnSuccess) {
			error = CVPixelBufferPoolCreatePixelBufferWithAuxAttributes(kCFAllocatorDefault, pixelBufferPool, auxAttributes, &pixelBuffer)
			if let pixelBuffer = pixelBuffer {
				pixelBuffers.append(pixelBuffer)
			}
			pixelBuffer = nil
		}
		pixelBuffers.removeAll()
		
		error = CVPixelBufferPoolCreatePixelBufferWithAuxAttributes(kCFAllocatorDefault, pixelBufferPool, auxAttributes, &pixelBuffer)
		if let pixelBuffer = pixelBuffer {
			var pixelBufferFormatDescription: CMFormatDescription?
			CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault, pixelBuffer, &pixelBufferFormatDescription)
			outputFormatDescription = pixelBufferFormatDescription
		}
		pixelBuffer = nil
		
		bufferPool = pixelBufferPool
		colorSpace = cgColorSpace
		inputFormatDescription = formatDescription
	}
	
	func prepare(with inputFormatDescription: CMFormatDescription, outputRetainedBufferCountHint: Int) {
		reset()
		allocateOutputBufferPool(with: inputFormatDescription, outputRetainedBufferCountHint: outputRetainedBufferCountHint)

		ciContext = CIContext()
		rosyFilter = CIFilter(name: "CIColorMatrix")
	//	rosyFilter!.setValue(CIVector(x: 0, y: 0, z: 0, w: 0), forKey: "inputGVector")
	}
	
	func reset() {
		ciContext = nil
		rosyFilter = nil
		colorSpace = nil
		bufferPool = nil
		outputFormatDescription = nil
		inputFormatDescription = nil
	}
	
	func render(pixelBuffer: CVPixelBuffer) -> CVPixelBuffer? {
		guard let rosyFilter = rosyFilter, let bufferPool = bufferPool, let ciContext = ciContext else {
			assertionFailure("Invalid state: Not prepared")
			return nil
		}
		let sourceImage = CIImage(cvImageBuffer: pixelBuffer)
		rosyFilter.setValue(sourceImage, forKey: kCIInputImageKey)
		guard let filteredImage = rosyFilter.value(forKey: kCIOutputImageKey) as? CIImage else {
			print("CIFilter failed to render image")
			return nil
		}
		
		var pbuf: CVPixelBuffer?
		CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, bufferPool, &pbuf)
		guard let outputPixelBuffer = pbuf else {
			print("Allocation failure")
			return nil
		}
		// Render the filtered image out to a pixel buffer (no locking needed as CIContext's render method will do that)
		ciContext.render(filteredImage, to: outputPixelBuffer, bounds: filteredImage.extent, colorSpace: colorSpace)
		return outputPixelBuffer
	}
}
