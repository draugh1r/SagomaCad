#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
uniform mat4 camera;
out vec3 n;
out vec3 world;
void main() { world=position; n=normal; gl_Position=camera*vec4(position,1); }
