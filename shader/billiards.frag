precision mediump float;
uniform vec4 u_color;
uniform int u_useTexture;
uniform sampler2D s_texture;

void main()
{
    vec4 texColor;
    texColor = texture2D( s_texture, gl_PointCoord );
    if (u_useTexture == 1) {
        gl_FragColor = vec4( u_color ) * texColor;
    } else {
        // TODO: Don't use constant color.
        gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
}
