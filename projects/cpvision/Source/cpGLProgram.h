//
//  cpGLProgram.h
//  cpVision
//

#import <Foundation/Foundation.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

// common attribute names
extern NSString * const cpGLProgramAttributeVertex;
extern NSString * const cpGLProgramAttributeTextureCoord;
extern NSString * const cpGLProgramAttributeNormal;

// inspired by Jeff LaMarche, https://github.com/jlamarche/iOS-OpenGLES-Stuff
@interface cpGLProgram : NSObject

- (id)initWithVertexShaderName:(NSString *)vertexShaderName fragmentShaderName:(NSString *)fragmentShaderName;

- (void)addAttribute:(NSString *)attributeName;

- (GLuint)attributeLocation:(NSString *)attributeName;
- (int)uniformLocation:(NSString *)uniformName;

- (BOOL)link;
- (void)use;

@end
