precision mediump float;
uniform vec4 u_color;
uniform int u_useTexture;
uniform sampler2D s_texture;

void main()
{
    if (u_useTexture == 1) {
        vec4 texColor;
        texColor = texture2D( s_texture, gl_PointCoord );
        gl_FragColor = vec4( u_color ) * texColor;
    } else {
        gl_FragColor = vec4(u_color);
    }
}
