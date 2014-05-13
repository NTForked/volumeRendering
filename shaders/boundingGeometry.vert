#version 130

uniform mat4 u_mvp;
in vec4 a_position;
out vec4 v_position;

void main() {
    v_position = a_position;
    gl_Position = u_mvp * a_position;
}
