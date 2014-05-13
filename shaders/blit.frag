#version 130

uniform sampler2D u_texture;
in vec2 v_texcoord;

void main()
{
    gl_FragColor = texture(u_texture, v_texcoord);
}
