uniform float u_time;
uniform mat4 u_MVP;
uniform float u_pointSize;
attribute vec2 a_startPosition;
//attribute vec2 a_endPosition;

void main( void )
{
    //gl_Position.xy = a_startPosition +
    //    (u_time * a_endPosition);
    gl_Position.xy = a_startPosition;
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
    gl_Position = u_MVP * gl_Position;

    gl_PointSize = u_pointSize;
}
