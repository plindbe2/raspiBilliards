uniform float u_time;
uniform vec2 u_centerPosition;
uniform mat4 u_MVP;
attribute vec2 a_startPosition;
attribute vec2 a_endPosition;

void main( void )
{
    gl_Position.xy = a_startPosition +
        (u_time * a_endPosition);
    gl_Position.xy += u_centerPosition;
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
    gl_Position = u_MVP * gl_Position;
    for ( int i = 0 ; i < 2 ; ++i ) {
        float divisor = (gl_Position[i] + 2.0) / 4.0;
        if ( divisor > 1.0 ) {
            float divisorFrac = divisor - float(int(divisor));
            gl_Position[i] = divisorFrac * 4.0;
            gl_Position[i] -= 2.0;
        }
        if ( divisor < -0.0 ) {
            float divisorFrac = divisor + float(int(divisor));
            divisorFrac = 1.0 + divisorFrac;
            gl_Position[i] = divisorFrac * 4.0;
            gl_Position[i] -= 2.0;
        }
    }
    gl_PointSize = 10.0;
}
