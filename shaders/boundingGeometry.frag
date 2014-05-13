#version 130

in vec4 v_position;
uniform sampler2D u_frontFaceTexture;

void main() {
    gl_FragColor = 0.5 * v_position + 0.5;
    /*gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);*/
}
