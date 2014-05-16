#version 130

uniform mat4 u_mvp;

in vec4 a_position;
out vec3 texCoord;

void main() {
    texCoord = a_position.xyz;
    gl_Position = u_mvp * a_position;
}
