#version 330 core
layout(location=0) in vec3 position;
uniform mat4 camera;
void main() { gl_Position=camera*vec4(position,1); }
