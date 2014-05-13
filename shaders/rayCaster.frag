#version 130

uniform sampler2D u_frontFaceTexture;
uniform sampler2D u_backFaceTexture;
in vec4 v_position;

void main()
{
    vec4 color = texture2D(u_frontFaceTexture, vec2(gl_TexCoord[0]));
    gl_FragColor = color;
    /*gl_FragColor = vec4(0.5 * v_position.x + 0.5, 0.5 * v_position.y + 0.5, 1.0, 1.0);*/
}
