#version 330 core
layout(location=0) in vec3 endpoint_a;
layout(location=1) in vec3 endpoint_b;
uniform mat4 camera;
uniform vec4 screen;
void main() {
    vec4 a=camera*vec4(endpoint_a,1),b=camera*vec4(endpoint_b,1);
    float za=a.z+a.w,zb=b.z+b.w;
    if(za<0.0&&zb<0.0){gl_Position=vec4(0,0,-2,1);return;}
    if(za<0.0)a=mix(a,b,za/(za-zb));
    else if(zb<0.0)b=mix(b,a,zb/(zb-za));
    vec2 half_screen=screen.xy*0.5;
    vec2 delta=(b.xy/max(b.w,1e-6)-a.xy/max(a.w,1e-6))*half_screen;
    float len=length(delta);
    vec2 direction=len>1e-6?delta/len:vec2(1,0);
    vec2 normal=vec2(-direction.y,direction.x);
    const float ends[6]=float[6](0,1,1,0,1,0);
    const float sides[6]=float[6](-1,-1,1,-1,1,1);
    float t=ends[gl_VertexID],side=sides[gl_VertexID];
    vec4 base=mix(a,b,t);
    vec2 offset=(normal*side+direction*(2.0*t-1.0))*screen.z*0.5;
    gl_Position=vec4(base.xy+offset/half_screen*base.w,base.z,base.w);
}
