precision mediump float;
uniform vec4 u_color;
uniform sampler2D s_texture;
varying vec2 v_texCoords;

void main()
{
    gl_FragColor = texture2D( s_texture, v_texCoords );
}
