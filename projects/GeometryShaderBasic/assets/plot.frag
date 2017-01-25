#version 330

//#ifndef ( VIEW_SIZE )
#define VIEW_SIZE 1024
//#endif


uniform float  uDataValue;
uniform int uDataIndex;

layout (std140) uniform DataWindow
{
    float uDataWindow[VIEW_SIZE];
};

uniform ivec2 ciWindowSize; // viewport resolution in pixels
uniform float ciElapsedSeconds; // shader playback time in seconds




//
in vec4		Color;
in vec3		Normal;
in vec2     Position;

layout (location = 0) out vec4			oColor;


float fn(in float indexf) {
    // function to be plotted...
    //  return 0.3 * sin(x * 9.0 + ciElapsedSeconds * 9.0) / (0.41 *(0.1 + abs(x))) * 1.0;
    highp int index = int(indexf);
    index = index % VIEW_SIZE;
    return uDataWindow[index];
}

//float fd(in float x) {
//    return  texture(uText0, x);
//}


float grid(in float step, in float position, in float pixelWidth) {
#define ALTERNATIONS 10.0
    
    float   m           = mod(position, step),
    distance    = min(m, step - m),
    intensity   = clamp(0.75 * pixelWidth / distance, 0.0, 1.0),
    alternation = mod(floor((position + 0.5 * step) / step), ALTERNATIONS);
    
    return intensity * (1.0 - (min(alternation, ALTERNATIONS - alternation) / (0.6 * ALTERNATIONS)));
}

void diffuseImage ( out vec4 fragColor, in vec2 fragCoord)
{
    vec3 normal = normalize( -Normal );
    float diffuse = max( dot( normal, vec3( 0, 0, -1 ) ), 0 );
    fragColor = Color * diffuse;
    
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    // little eye candy, let's move in graph for better throwing up
    float   timeScale   = 0.1,
    time        = ciElapsedSeconds * timeScale,
    viewSize    = (0.7 + 0.5 * sin(0.0f)) * 5.0,
    centerX     = sin(time*2.31) * 5.0;
    
    // defines the viewport bounds of the displayed graph
    float   left        = -viewSize + centerX,
    right       =  viewSize + centerX,
    top         =  viewSize,
    bottom      = -viewSize;
    
    // convert fragment's pixel position to graph space location
    float   locationX   = left   + (right - left) * fragCoord.x / ciWindowSize.x,
    locationY   = bottom + (top - bottom) * fragCoord.y / ciWindowSize.y;
    
    // calculate width of a pixel in graph space
    float   pixelWidth  = (right - left) / ciWindowSize.x;
    
    // calculate two points left and right of current graph space location
    float   x1          = locationX - pixelWidth, y1 = fn(x1),
    x2          = locationX + pixelWidth, y2 = fn(x2);
    
    // get distance from point at (locationX, locationY)
    // to tangent given by (x1, y1) and (x2, y2)
    float   distance    = abs((y2 - y1) * locationX - (x2 - x1) * locationY + x2*y1 - y2*x1 ) /
    sqrt((y2-y1)*(y2-y1) + (x2-x1)*(x2-x1));
    
    float   thickness   = 1.75,
    graphColor  = 0.8 * (1.0 - clamp(distance / (thickness * pixelWidth), 0.0, 1.0)),
    gridColor   = 0.5 * grid(1.0, locationY, pixelWidth) + 0.5 * grid(1.0, locationX, pixelWidth);
    
    fragColor = vec4(graphColor,
                     graphColor + gridColor,
                     graphColor,
                     1.0);
}

//void main( void )
//{
//    vec3 normal = normalize( -Normal );
//    float diffuse = max( dot( normal, vec3( 0, 0, -1 ) ), 0 );
//    oColor = Color * diffuse;
//}

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

