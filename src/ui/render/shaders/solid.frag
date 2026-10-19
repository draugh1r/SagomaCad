#version 330 core
in vec3 n;
in vec3 world;
uniform vec4 body;
uniform vec4 toward_eye;
uniform vec4 section_plane;
out vec4 color;
vec3 linear_color(vec3 c) {
    return mix(c/12.92,pow((c+0.055)/1.055,vec3(2.4)),step(vec3(0.04045),c));
}
vec3 display_color(vec3 c) {
    c=clamp(c,0.0,1.0);
    return mix(c*12.92,1.055*pow(c,vec3(1.0/2.4))-0.055,step(vec3(0.0031308),c));
}
void main() {
    if(length(section_plane.xyz)>0.5 && dot(section_plane.xyz,world)>section_plane.w)discard;
    if(length(n)<0.0001) { color=body; return; }
    if(!gl_FrontFacing && length(section_plane.xyz)>0.5) {
        float hatch=step(0.22,fract((gl_FragCoord.x+gl_FragCoord.y)/9.0));
        color=vec4(display_color(linear_color(body.rgb)*mix(0.6,1.0,hatch)),body.a);return;
    }
    vec3 v=normalize(toward_eye.xyz);
    vec3 normal=normalize(n);
    if(dot(normal,v)<0.0)normal=-normal;
    vec3 key=normalize(v*0.8+vec3(-0.3,0.2,0.6));
    float diffuse=max(dot(normal,key),0.0);
    float fill=max(dot(normal,v),0.0);
    float specular=pow(max(dot(normal,normalize(key+v)),0.0),40.0)*0.25;
    float intensity=0.42+0.38*diffuse+0.25*fill;
    color=vec4(display_color(linear_color(body.rgb)*intensity+vec3(specular)),body.a);
}
