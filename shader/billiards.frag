precision mediump float;
uniform vec4 u_color;
uniform sampler2D s_texture;

void main()
{
    gl_FragColor = texture2D( s_texture, gl_PointCoord );
}
