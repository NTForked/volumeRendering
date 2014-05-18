#version 130

in vec4 a_position;
uniform mat4 u_mvp;
out vec2 quadTex;

void main()
{
    quadTex = a_position.xy;

    gl_Position = a_position;
}
