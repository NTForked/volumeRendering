#version 130

in vec4 a_position;
uniform mat4 u_mvp;

out vec4 v_position;
out vec2 quadTex;

void main()
{
    quadTex = a_position.xy;
    v_position = a_position;

    gl_Position = a_position;
}
