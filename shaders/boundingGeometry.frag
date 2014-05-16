#version 130

in vec3 texCoord;

void main() {
    gl_FragColor = vec4(0.5 * texCoord + 0.5,1.0);
}
