#define SHADERTOY

//#if defined(GL_ES) || defined(GL_SHADING_LANGUAGE_VERSION)
#define _in(T) const in T
#define _inout(T) inout T
#define _out(T) out T
#define _begin(type) type (
#define _end )
#define _mutable(T) T
#define _constant(T) const T
#define mul(a, b) (a) * (b)
precision mediump float;
//#endif


uniform vec3    iResolution;
uniform float iGlobalTime;


uniform ivec2 ciWindowSize; // viewport resolution in pixels
uniform float ciElapsedSeconds; // shader playback time in seconds

in vec2     Position;

out vec4			oColor;

#if defined(__cplusplus) || defined(SHADERTOY)
#define u_res iResolution
#define u_time iGlobalTime
#define u_mouse iMouse
#endif

// Noise function by iq from https://www.shadertoy.com/view/4sfGzS
float hash(in float n)
{
    return fract(sin(n)*753.5453123);
}

float noise_iq(in vec3 x)
{
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f*f*(3.0 - 2.0*f);
    
#if 1
    float n = p.x + p.y*157.0 + 113.0*p.z;
    return mix(mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
                   mix( hash(n+157.0), hash(n+158.0),f.x),f.y),
               mix(mix( hash(n+113.0), hash(n+114.0),f.x),
                   mix( hash(n+270.0), hash(n+271.0),f.x),f.y),f.z);
#else
    vec2 uv = (p.xy + vec2(37.0, 17.0)*p.z) + f.xy;
    vec2 rg = texture2D(iChannel0, (uv + 0.5) / 256.0, -100.0).yx;
    return mix(rg.x, rg.y, f.z);
#endif
}

#define noise(x) noise_iq(x)
#define snoise(x) (noise_iq(x) * 2. - 1.)

float fbm(  _in(vec3) pos,
          _in(float) lacunarity
          ){
    vec3 p = pos;
    float gain = .5;
    float t = 0.;
    
    for (int i = 0; i < 7; i++) {
        t += gain * noise(p);
        p *= lacunarity;
        gain *= .5;
    }
    
    return t;
}

#define SCALE 1.
#define D (.0125 * SCALE)

vec3 plot(
          _in(float) f,
          _in(float) x,
          _in(vec3) color
          ){
    float y = smoothstep (f-D, f+D, x);
    y *= 1.- y;
    
    return y * color * 5.;
}

void mainImage(
               _out(vec4) fragColor,
#ifdef SHADERTOY
               vec2 fragCoord
#else
               _in(vec2) fragCoord
#endif
               ){
    // go from [0..resolution] to [0..1]
    vec2 t = fragCoord.xy / u_res.xy;
#ifdef HLSL
    t.y = 1. - t.y;
#endif
    
    // optional: center around origin by going to [-1, +1] and scale
    t = (t * 2. - 1.) * SCALE;
    
    // plot the axes
    vec3 col = vec3(0, 0, 0);
    col += plot(0., t.y, vec3(1, 1, 1));
    col += plot(t.x, 0., vec3(1, 1, 1));
    
    // plot custom functions
    t.x += u_time * SCALE;
    col += plot(sin(t.x), t.y, vec3(1, 0, 0));
    col += plot(noise(vec3(t.x, 0, 0)), t.y, vec3(0, 1, 1));
    col += plot(snoise(vec3(t.x, 0, 0)), t.y, vec3(0, 0, 1));
    col += plot(fbm(vec3(t.x, 0, 0) * 1., 2.), t.y, vec3(0, 1, 0));
    
    // output
    fragColor = vec4 (col, 1);
}

void main(void)
{
    vec4 color;
    vec2 fixe;
    fixe.x = Position.x * ciWindowSize.x;
    fixe.y = Position.y * ciWindowSize.y;
    mainImage(color, fixe);
    oColor = color;
    //  diffuseImage (oColor, vec2(0,0));
}

