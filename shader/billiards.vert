uniform float u_time;
uniform mat4 u_MVP;
attribute vec2 a_startPosition;
attribute vec2 a_texCoords;
varying vec2 v_texCoords;

void main( void )
{
    gl_Position.xy = a_startPosition;
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
    gl_Position = u_MVP * gl_Position;

    v_texCoords = a_texCoords;
}
