uniform sampler2D u_samplerY;
uniform sampler2D u_samplerUV;
varying highp vec2 v_texture;

const mediump vec4 shadowColor = vec4(1.0,0.0,0.64,0.6);
const mediump vec4 overexposureColor = vec4(1.0,0.99,0.71,0.07);
const mediump vec4 blurColor = vec4(1.0,0.87,0.31,0.22);

void main()
{
    mediump vec3 yuv;
    mediump vec3 rgb;
    mediump vec4 pixelColor, maskColor;
    
    yuv.x = texture2D(u_samplerY, v_texture).r;
    yuv.yz = texture2D(u_samplerUV, v_texture).rg - vec2(0.5, 0.5);
    
    // BT.709, the standard for HDTV
    rgb = mat3(      1,       1,      1,
               0, -.18732, 1.8556,
               1.57481, -.46813,      0) * yuv;
    
    pixelColor = vec4(rgb, 1.0);
    
    // mean of the log (RGB) --
    //
    mediump vec3 l_rgb = log(rgb);
    mediump float s = max(dot(l_rgb, vec3(1.0/3.0)), 0.3);
    
    mediump float luminance = yuv[0];
    if (luminance >= 1.0 || s > luminance)
    {
        if (luminance >= 1.0) maskColor = overexposureColor;
        else maskColor = shadowColor;
        pixelColor = vec4(mix(rgb, maskColor.rgb, 0.2), 1.0);
    }
    
    gl_FragColor = pixelColor;
    
    
}