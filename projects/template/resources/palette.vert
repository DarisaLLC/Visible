#version 330 core

in vec4 ciPosition;
in vec2 ciTexCoord0;
uniform mat4 ciModelViewProjection;
smooth out vec2 vTexCoord;

void main()
{
    gl_Position = ciModelViewProjection * ciPosition;
    vTexCoord = vec2(ciTexCoord0.x, 1.0 - ciTexCoord0.y);
}
