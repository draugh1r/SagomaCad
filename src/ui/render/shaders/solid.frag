#version 330 core
in vec3 n;
in vec3 world;
uniform vec4 body;
out vec4 color;
void main() { color=vec4(body.rgb*(0.65+0.35*max(0.0,dot(normalize(n),normalize(vec3(.4,-.3,.8))))),body.a); }
