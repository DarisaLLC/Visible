//
//  cpGLProgram.m
//  cpVision
//

#import "cpGLProgram.h"


NSString * const cpGLProgramAttributeVertex = @"a_position";
NSString * const cpGLProgramAttributeTextureCoord = @"a_texture";
NSString * const cpGLProgramAttributeNormal = @"a_normal";

@interface cpGLProgram ()
{
    GLuint _program;
    GLuint _vertShader;
    GLuint _fragShader;
    
    NSString *_vertexShaderName;
    NSString *_fragmentShaderName;
    NSMutableArray *_attributes;
}

@end

@implementation cpGLProgram

- (id)initWithVertexShaderName:(NSString *)vertexShaderName fragmentShaderName:(NSString *)fragmentShaderName;
{
    self = [super init];
    if (self) {
        _program = glCreateProgram();
        
        _vertexShaderName = vertexShaderName;
        if (![self _compileShader:&_vertShader type:GL_VERTEX_SHADER file:vertexShaderName]) {
            NSLog(@"failed to compile vertex shader");
        }
        
        _fragmentShaderName = fragmentShaderName;
        if (![self _compileShader:&_fragShader type:GL_FRAGMENT_SHADER file:fragmentShaderName]) {
            NSLog(@"failed to compile fragment shader");
        }
        
        glAttachShader(_program, _vertShader);
        glAttachShader(_program, _fragShader);
        
        _attributes = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void)dealloc
{
    if (_vertShader) {
        glDeleteShader(_vertShader);
    }
    
    if (_fragShader) {
        glDeleteShader(_fragShader);
    }

    if (_program) {
        glDeleteProgram(_program);
        _program = 0;
    }
}


- (BOOL)_compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file
{
    GLint status;
    const GLchar *source;
    
    source = (GLchar *)[[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] UTF8String];
    if (!source) {
        NSLog(@"failed to load vertex shader");
        return NO;
    }
    
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);
    
#if defined(DEBUG)
    
    GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc((unsigned long)logLength);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        NSLog(@"Shader compile log:\n%s", log);
        free(log);
    }
    
#endif
    
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glDeleteShader(*shader);
        return NO;
    }
    
    return YES;
}

#pragma mark -

- (void)addAttribute:(NSString *)attributeName
{
    if (![_attributes containsObject:attributeName]) {
        [_attributes addObject:attributeName];
        GLuint index = [self attributeLocation:attributeName];
        glBindAttribLocation(_program, index, [attributeName UTF8String]);
    }
}

- (GLuint)attributeLocation:(NSString *)attributeName
{
    return (GLuint)[_attributes indexOfObject:attributeName];
}

- (int)uniformLocation:(NSString *)uniformName
{
    return glGetUniformLocation(_program, [uniformName UTF8String]);
}

- (BOOL)link
{
    GLint status;
    
    glLinkProgram(_program);
    glValidateProgram(_program);
    
    glGetProgramiv(_program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        NSLog(@"failed to link program, %d", _program);
        return NO;
    }
    
    if (_vertShader) {
        glDeleteShader(_vertShader);
        _vertShader = 0;
    }
    
    if (_fragShader) {
        glDeleteShader(_fragShader);
        _fragShader = 0;
    }
    
    return NO;
}

- (void)use
{
    glUseProgram(_program);
}

@end
