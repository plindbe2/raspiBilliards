precision mediump float;
uniform vec4 u_color;
uniform int u_useTexture;
uniform sampler2D s_texture;

void main()
{
    if (u_useTexture == 1) {
        gl_FragColor = texture2D( s_texture, gl_PointCoord );
    } else {
        gl_FragColor = vec4(u_color);
    }
}
