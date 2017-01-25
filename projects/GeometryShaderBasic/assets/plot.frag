

uniform ivec2 ciWindowSize; // viewport resolution in pixels
uniform float ciElapsedSeconds; // shader playback time in seconds
uniform


const int COUNT = 5;                 //Graph Count
const float HORIZONTAL_UNITS = 15.0; //Horizontal Span (domain units)
const float INFINITY = 10000.0;
const float VERTICAL_UNITS = float(COUNT);
const float FUNC_A = 0.25; // A constant expression used in the wave function. It control how "spikey" they are.
float g_animTime = 0.0;

float genRdm( vec2 p )
{
    //(Numbers are arbitrary, this is pseudo-random)
    return fract(sin(p.x*347.1+p.y*283.1)*987643.21234);
}

// -----------------------------------------------------------
// The 4 corners of the bilinear interpolation (view from top)
//
//       (1,0)              (1,1)
// 1    c-----------f------d
// |    |           .      |
// |    |           .      |
// |    |           .      |
//t.y   g...........v......h
// |    |           .      |
// |    |           .      |
// |    |           .      |
// |    |           .      |
// |    |(0,0)      .      |(1,0)
// 0    a-----------e------b      ->x
//
//
//      0__________t.x_____1
//
// -----------------------------------------------
// And the ease function below (value view, with tangent)
//
// Here, the ease function to               ......h
// interpolate from g to h.           ......
// The value delta between        ....
// h and g is indeed         / ...
// a scaling multiplier     /..
// applied on top          /.
// of the ease            /.
// function            ../
// derivative      .... /
//           ......
//   g.......
//
#define CUBIC   1
#define QUINTIC 2
#define SINE    3
#define EASE_FUNC QUINTIC
vec3 bilinearNoise( in vec2 p, float fTime )
{
    const float PI = 3.14159;
    p.xy += fTime;
    vec2 x = fract( p );
    vec2 i = p-x;
#if(EASE_FUNC==CUBIC)
    //Option 1 : Cubic ease function
    vec2 t = x*x*(3.0-2.0*x); //ease function
    vec2 d_xy = 6.0*x*(1.0-x);//derivative
#elif(EASE_FUNC==QUINTIC)
    //Option 1 : Quintic ease function
    vec2 t = (6.*x*x-15.0*x+10.)*x*x*x; //ease function
    vec2 d_xy = (30.*x*x-60.*x+30.)*x*x;  //derivative
#elif(EASE_FUNC==SINE)
    //Option 3 : Trigonometric ease function
    vec2 t = 0.5+0.5*sin(-PI/2.0+x*PI);   //ease function
    vec2 d_xy = 0.5*PI*cos(-PI/2.0+x*PI); //derivative
#endif
    
    //bilinear interpolation (abcd = 4 corners)
    float a = genRdm( i + vec2(0.0,0.0) );
    float b = genRdm( i + vec2(1.0,0.0) );
    float c = genRdm( i + vec2(0.0,1.0) );
    float d = genRdm( i + vec2(1.0,1.0) );
    
    //Note : g and h could be factorized out.
    float e = a+(b-a)*t.x; //Horizontal interpolation 1 (e)
    float f = c+(d-c)*t.x; //Horizontal interpolation 2 (f)
    float v = e*(1.-t.y)+f*(t.y); //Vertical interpolation (v)
    
    //All this could be factorized and be made more compact.
    //Step by step is much easier to understand, however.
    float g = a+(c-a)*t.y; //Vertical interpolation (g)
    float h = b+(d-b)*t.y; //Vertical interpolation (h)
    
    //Noise delta scaling (the lookup value slope along x/y axis).
    float sx = h-g; //dv/dx scaling
    float sy = f-e; //dv/dy scaling
    
    return vec3(v,
                d_xy.x*sx,
                d_xy.y*sy);
    
    //return [h,dh/dx,dh/dy]
}
//The 2D function shown in the middle graph.
//This fonction was created to be used as a base for water waves.
float warpedGrid01(vec2 uv, float fTime)
{
    float a = FUNC_A;
    uv += bilinearNoise(uv,fTime).x;
    float gx = 0.5+sin(2.0*uv.x)*0.5;
    float hx = a/(gx+a);
    float gy = 0.5+sin(2.0*uv.y)*0.5;
    float hy = a/(gy+a);
    return (hx+hy)*0.5;
}

//****************************************
// Signal 1 (topmost)
float F(float x)
{
    //f(x) = noise(x).x
    return bilinearNoise(vec2(x,0),g_animTime).x;
}
float dF(float x)
{
    //f'(x) = noise(x).y
    //vec3[h,dx,dy]=waveNoise(x)
    return bilinearNoise(vec2(x,0),g_animTime).y; //waveNoise(x).y=dh/dx
}
//****************************************
// Signal 2 (second from the top)
float G(float x)
{
    //g(x) = 0.5 + sin(2f(x))/2
    vec2 uv = vec2(x,0);
    uv += bilinearNoise(uv,g_animTime).x;
    return 0.5+sin(2.0*uv.x)*0.5;
}
float dG(float x)
{
    //Theory : g'(x) = cos(2f(x))*(1+n'(x))
    vec2 uv = vec2(x,0);
    uv += bilinearNoise(uv,g_animTime).x;
    return cos(2.0*uv.x)*(1.0+dF(x));
}
//****************************************
// Signal 3 (third from the top)
float H(float x)
{
    float a = FUNC_A;
    //h(x) = a/(g(x)+a)
    vec2 uv = vec2(x,0);
    uv += bilinearNoise(uv,g_animTime).x;
    float gx = 0.5+sin(2.0*uv.x)*0.5;
    float hx = a/(gx+a);
    return hx;
}
float dH(float x)
{
    //Theory : h'(x) = -g'(x)*a/(g(x)+a)^2
    float a = FUNC_A;
    float g = G(x);
    //Application :
    return -dG(x)*a/((G(x)+a)*(G(x)+a));
}
//****************************************
// Signal 4 (third from the top) : basic example
float Func4(float x)
{
    return 0.5+0.5*sin(x);
}
float dFunc4(float x)
{
    return 0.5*cos(x);
}
//****************************************
// Signal 5 (third from the top) : basic example
float Func5(float x)
{
    return 0.5+0.5*sin(4.*x);
}
float dFunc5(float x)
{
    return 0.5*4.0*cos(4.*x);
}

//Simple utility function which returns the distance from point "p" to a given line segment defined by 2 points [a,b]
float distanceToLineSeg(vec2 p, vec2 a, vec2 b)
{
    //e = capped [0,1] orthogonal projection of ap on ab
    //       p
    //      /
    //     /
    //    a--e-------b
    vec2 ap = p-a;
    vec2 ab = b-a;
    vec2 e = a+clamp(dot(ap,ab)/dot(ab,ab),0.0,1.0)*ab;
    return length(p-e);
}

//Returns the domain (x) position at pixel px
float getDomainValue(float px)
{
    //The fract(x/1000.0)*1000.0 is there to prevent precision issues with large numbers.
    return fract((iGlobalTime)/1000.0)*1000.0 + px*HORIZONTAL_UNITS;
}

//When the analytic derivative is calculated properly, this function returns
//the distance to a small segment which must be tangeant to the function signal.
//(If the derivative computation is wrong, displayed tangent will not match the slope)
float distToAnalyticDerivatives(vec2 p)
{
    const float TAN_LEN = 0.05; //The segment (tangent) half-length
    float imageHeight = iResolution.y/iResolution.x;
    
    //Conversion factor from computation space to image space (Horizontal & Vert)
    vec2 imgScaling = vec2(1.0/HORIZONTAL_UNITS,imageHeight/VERTICAL_UNITS);
    
    //px = position (uv.x) at which the derivative is evaluated.
    float px = 0.5; //0.5 = image center
    if(iMouse.z > 0.1)
    {
        px = iMouse.x/iResolution.x;
    }
    float x = getDomainValue(px);
    
    vec2 V_Dx[COUNT];
    V_Dx[0] = vec2(F(x),dF(x));
    V_Dx[1] = vec2(G(x),dG(x));
    V_Dx[2] = vec2(H(x),dH(x));
    V_Dx[3] = vec2(Func4(x),dFunc4(x));
    V_Dx[4] = vec2(Func5(x),dFunc5(x));
    
    float minDist = INFINITY;
    for(int i=0; i < COUNT; ++i)
    {
        //Signal height, image space [0-imageHeight]
        float signalHeight = imgScaling.y*float(COUNT-1-i);
        
        //Computation space value and tangent
        float functionValue = V_Dx[i].x; //f(x)
        vec2 functionTan    = vec2(1.0,V_Dx[i].y); //vec2 = [dx,df'(x)], in other words, the variation of f(x) per unit of x
        
        //Image space value and tangent
        vec2 imageValue = vec2(px,signalHeight+functionValue*imgScaling.y);
        vec2 imageTangent = normalize(functionTan*imgScaling)*TAN_LEN;
        
        minDist = min(minDist,distanceToLineSeg(p,imageValue-imageTangent,imageValue+imageTangent));
    }
    return minDist;
}

//Returns the vertical distance of point p to graph line.
float distanceToGraphValue(vec2 p)
{
    //Vertical span for each signal
    float vSpan = float(COUNT)*(iResolution.x/iResolution.y);
    //x pos at pixel p.x
    float x = getDomainValue(p.x);
    
    float graphValues[COUNT];
    graphValues[0] = F(x);
    graphValues[1] = G(x);
    graphValues[2] = H(x);
    graphValues[3] = Func4(x);
    graphValues[4] = Func5(x);
    
    float minDist = INFINITY;
    for(int i=0; i < COUNT; ++i)
    {
        float vOffset = float(COUNT-1-i)/vSpan;
        vec2 p = vec2(p.x,(p.y-vOffset)*vSpan);
        minDist = min(minDist,abs(p.y-graphValues[i]));
    }
    return minDist;
}

//Returns the [x,y] gradient magnitude. All signals are processed.
vec2 absGrad(vec2 p, float d0)
{
    //Note : using max(d+,d-) derivative to reduce artifacts.
    const float fEps = 0.001;
    float dxp = abs(distanceToGraphValue(p+vec2(fEps,0))-d0);
    float dyp = abs(distanceToGraphValue(p+vec2(0,fEps))-d0);
    float dxm = abs(distanceToGraphValue(p-vec2(fEps,0))-d0);
    float dym = abs(distanceToGraphValue(p-vec2(0,fEps))-d0);
    return vec2(max(dxp,dxm),max(dyp,dym))/fEps;
}

//Note : the valueGradient could be computed at the end, just like the normal is computed
//       using the global terrain/map function. In fact, the entire graph program could probably be written
//       just like a distance field.
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    //uv : x:[0,1]
    vec2 uv = fragCoord.xy / iResolution.xx;
    
    //Vertical span for a single signal
    float vSpan = VERTICAL_UNITS*(iResolution.x/iResolution.y);
    g_animTime = iGlobalTime;
    
    //<Distance To Graph Value>
    float d = distanceToGraphValue(uv); //Vertical Distance
    vec2 aGrad = absGrad(uv,d); //Gradient
    d = d/length(aGrad);        //Slope compensation, to keep the line width constant.
    //</Distance To Graph Value>
    
    //<Distance To Derivative Line>
    float der = distToAnalyticDerivatives(uv);
    //</Distance To Graph Value>
    
    //<Background + graph window separation>
    float dSigWin = abs(fract((uv.y)*vSpan+0.5)-0.5);
    float cSigWin = smoothstep(0.0, 0.005*VERTICAL_UNITS, dSigWin);
    vec3 cBack = mix(vec3(0,0,0.8),vec3(1),cSigWin);
    //</Background + graph window separation>
    
    //Darken the background along plot lines
    cBack *= smoothstep(0.0, 0.003, d); //Close = 0(Black), Far(>.003) = 1(White)
    
    //Mix derivative Color with whatever is behind
    float derivativeLineIntensity = 1.0-smoothstep(0.0, 0.003, der);
    cBack = mix(cBack,vec3(1,0.2,0.2),derivativeLineIntensity);
    
    //Return Final Color
    fragColor = vec4(cBack, 1);
}
