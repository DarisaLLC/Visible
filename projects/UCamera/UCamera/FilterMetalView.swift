/*
	Copyright (C) 2016 Apple Inc. All Rights Reserved.
	See LICENSE.txt for this sample’s licensing information
	
	Abstract:
	Metal preview view.
*/

import Foundation

import CoreMedia
import Metal
import MetalKit

class FilterMetalView: MTKView
{
	enum Rotation : Int {
		case rotate0Degrees
		case rotate90Degrees
		case rotate180Degrees
		case rotate270Degrees
	}
	
	var mirroring = false {
		didSet {
			setNeedsDisplay()
		}
	}
	
	var rotation: Rotation = .rotate0Degrees {
		didSet {
			setNeedsDisplay()
		}
	}

	var setDeferDrawingUntilNewTexture = false
	
	var texture: MTLTexture? {
		didSet {
			setDeferDrawingUntilNewTexture = false
			setNeedsDisplay()
		}
	}
	
	private var textureWidth: Int = 0
	
	private var textureHeight: Int = 0
	
	private var textureMirroring = false
	
	private var textureRotation: Rotation = .rotate0Degrees
	
	private var sampler: MTLSamplerState
	
	private var pipelineState: MTLRenderPipelineState
	
	private var commandQueue: MTLCommandQueue
	
	private var vertexCoordBuffer: MTLBuffer!
	
	private var textCoordBuffer: MTLBuffer!
	
	private var internalBounds: CGRect!
	
	private var textureTranform: CGAffineTransform?
	
	func texturePointForView(point: CGPoint) -> CGPoint? {
		var result: CGPoint?
		guard let transform = textureTranform else {
			return result
		}
		let transformPoint = point.applying(transform)
		if (CGRect(origin: CGPoint.zero, size: CGSize(width: textureWidth, height: textureHeight)).contains(transformPoint)) {
			result = transformPoint
		}
//		else {
//			print("Invalid point \(point) result point \(transformPoint)")
//		}
		return result
	}
	
	func viewPointForTexture(point: CGPoint) -> CGPoint? {
		var result: CGPoint?
		guard let transform = textureTranform?.inverted() else {
			return result
		}
		let transformPoint = point.applying(transform)
		if (internalBounds.contains(transformPoint)) {
			result = transformPoint
		}
		else {
			print("Invalid point \(point) result point \(transformPoint)")
		}
		return result
	}
	
	private func setupTransform(width: Int, height: Int, mirroring: Bool, rotation: Rotation) {
		var scaleX: Float = 1.0
		var scaleY: Float = 1.0
		var resizeAspect: Float = 1.0
		
		internalBounds = self.bounds
		textureWidth = width
		textureHeight = height
		textureMirroring = mirroring
		textureRotation = rotation
		
		if (textureWidth > 0 && textureHeight > 0) {
			switch textureRotation {
				case .rotate0Degrees, .rotate180Degrees:
					scaleX = Float(internalBounds.width/CGFloat(textureWidth))
					scaleY = Float(internalBounds.height/CGFloat(textureHeight))
				
				case .rotate90Degrees, .rotate270Degrees:
					scaleX = Float(internalBounds.width/CGFloat(textureHeight))
					scaleY = Float(internalBounds.height/CGFloat(textureWidth))
			}
		}
		// Resize Aspect
		resizeAspect = min(scaleX, scaleY)
		if (scaleX < scaleY) {
			scaleY = scaleX / scaleY
			scaleX = 1.0
		}
		else {
			scaleX = scaleY / scaleX
			scaleY = 1.0
		}
		
		if (textureMirroring) {
			scaleX = scaleX * -1.0
		}

		// Vertex coordinate takes the gravity into account
		let vertexData:[Float] = [
			-scaleX,  -scaleY, 0.0, 1.0,
			scaleX, -scaleY, 0.0, 1.0,
			-scaleX,  scaleY, 0.0, 1.0,
			scaleX, scaleY, 0.0, 1.0,
			]
		vertexCoordBuffer = device!.makeBuffer(bytes: vertexData, length: vertexData.count * MemoryLayout<Float>.size, options: [])

		// Texture coordinate takes the rotation into account
		var textData: [Float]
		switch textureRotation {
		case .rotate0Degrees:
			textData = [
				0.0, 1.0,
				1.0, 1.0,
				0.0, 0.0,
				1.0, 0.0,
			]
			
		case .rotate180Degrees:
			textData = [
				1.0, 0.0,
				0.0, 0.0,
				1.0, 1.0,
				0.0, 1.0,
			]
			
		case .rotate90Degrees:
			textData = [
				1.0, 1.0,
				1.0, 0.0,
				0.0, 1.0,
				0.0, 0.0,
			]
			
		case .rotate270Degrees:
			textData = [
				0.0, 0.0,
				0.0, 1.0,
				1.0, 0.0,
				1.0, 1.0,
			]
		}
		textCoordBuffer = device?.makeBuffer(bytes: textData, length: textData.count * MemoryLayout<Float>.size, options: [])
		
		// Calculate the transform from texture coordinates to view coordinates 
		var transform = CGAffineTransform.identity
		if (textureMirroring) {
			transform = transform.concatenating(CGAffineTransform(scaleX: -1, y: 1))
			transform = transform.concatenating(CGAffineTransform(translationX: CGFloat(textureWidth), y: 0))
		}
		switch textureRotation {
			case .rotate0Degrees:
				transform = transform.concatenating(CGAffineTransform(rotationAngle: CGFloat(0)))
			
			case .rotate180Degrees:
				transform = transform.concatenating(CGAffineTransform(rotationAngle: CGFloat(M_PI)))
				transform = transform.concatenating(CGAffineTransform(translationX: CGFloat(textureWidth), y: CGFloat(textureHeight)))
			
			case .rotate90Degrees:
				transform = transform.concatenating(CGAffineTransform(rotationAngle: CGFloat(M_PI)/2))
				transform = transform.concatenating(CGAffineTransform(translationX: CGFloat(textureHeight), y: 0))
			
			case .rotate270Degrees:
				transform = transform.concatenating(CGAffineTransform(rotationAngle: 3*CGFloat(M_PI)/2))
				transform = transform.concatenating(CGAffineTransform(translationX: 0, y: CGFloat(textureWidth)))
		}
		transform = transform.concatenating(CGAffineTransform(scaleX: CGFloat(resizeAspect), y: CGFloat(resizeAspect)))
		let tranformRect = CGRect(origin: .zero, size: CGSize(width: textureWidth, height: textureHeight)).applying(transform)
		let tx = (internalBounds.size.width - tranformRect.size.width)/2
		let ty = (internalBounds.size.height - tranformRect.size.height)/2
		transform = transform.concatenating(CGAffineTransform(translationX: tx, y: ty))
		textureTranform = transform.inverted()
	}
	
	required init(frame: CGRect) {
		let device = MTLCreateSystemDefaultDevice()!
		let defaultLibrary = device.newDefaultLibrary()!
		let pipelineDescriptor = MTLRenderPipelineDescriptor()
		pipelineDescriptor.sampleCount = 1
		pipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
		pipelineDescriptor.depthAttachmentPixelFormat = .invalid
		pipelineDescriptor.vertexFunction = defaultLibrary.makeFunction(name: "texturedQuadVertex")
		pipelineDescriptor.fragmentFunction = defaultLibrary.makeFunction(name: "texturedQuadFragment")
		
		// To determine how our textures are sampled, we create a sampler descriptor, which
		// will be used to ask for a sampler state object from our device below.
		let samplerDescriptor = MTLSamplerDescriptor()
		samplerDescriptor.sAddressMode = .clampToZero
		samplerDescriptor.tAddressMode = .clampToZero
		samplerDescriptor.minFilter = .linear
		samplerDescriptor.magFilter = .linear
		self.sampler = device.makeSamplerState(descriptor: samplerDescriptor)
		self.pipelineState = try! device.makeRenderPipelineState(descriptor: pipelineDescriptor)
		self.commandQueue = device.makeCommandQueue()
		
		super.init(frame: frame, device: device)
		
		self.colorPixelFormat = .bgra8Unorm
	}
	
	required init(coder: NSCoder) {
		let device = MTLCreateSystemDefaultDevice()!
		let defaultLibrary = device.newDefaultLibrary()!
		let pipelineDescriptor = MTLRenderPipelineDescriptor()
		pipelineDescriptor.sampleCount = 1
		pipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
		pipelineDescriptor.depthAttachmentPixelFormat = .invalid
		pipelineDescriptor.vertexFunction = defaultLibrary.makeFunction(name: "texturedQuadVertex")
		pipelineDescriptor.fragmentFunction = defaultLibrary.makeFunction(name: "texturedQuadFragment")
		
		// To determine how our textures are sampled, we create a sampler descriptor, which
		// will be used to ask for a sampler state object from our device below.
		let samplerDescriptor = MTLSamplerDescriptor()
		samplerDescriptor.sAddressMode = .clampToZero
		samplerDescriptor.tAddressMode = .clampToZero
		samplerDescriptor.minFilter = .linear
		samplerDescriptor.magFilter = .linear
		self.sampler = device.makeSamplerState(descriptor: samplerDescriptor)
		self.pipelineState = try! device.makeRenderPipelineState(descriptor: pipelineDescriptor)
		self.commandQueue = device.makeCommandQueue()
		
		super.init(coder: coder)
		
		self.device = device
		self.colorPixelFormat = .bgra8Unorm
	}

	override func draw(_ rect: CGRect) {
		if setDeferDrawingUntilNewTexture {
			return
		}
		guard let drawable = currentDrawable, let currentRenderPassDescriptor = currentRenderPassDescriptor, let texture = texture else {
			return
		}
		
		if (texture.width != textureWidth || texture.height != textureHeight ||
			self.bounds != internalBounds ||
			mirroring != textureMirroring ||
			rotation != textureRotation) {
			setupTransform(width: texture.width, height: texture.height, mirroring: mirroring, rotation: rotation)
		}
		let commandBuffer = commandQueue.makeCommandBuffer()
		let commandEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: currentRenderPassDescriptor)
		
		commandEncoder.setRenderPipelineState(pipelineState)
		commandEncoder.setVertexBuffer(vertexCoordBuffer, offset: 0, at: 0)
		commandEncoder.setVertexBuffer(textCoordBuffer, offset: 0, at: 1)
		commandEncoder.setFragmentTexture(texture, at: 0)
		commandEncoder.setFragmentSamplerState(sampler, at: 0)
		commandEncoder.drawPrimitives(type: .triangleStrip, vertexStart: 0, vertexCount: 4)
		commandEncoder.endEncoding()
		commandBuffer.present(drawable)
		commandBuffer.commit()
	}
}
       
