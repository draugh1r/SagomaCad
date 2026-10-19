#version 330 core
in vec2 ndc;
uniform mat4 camera;
uniform vec4 eye,view_right,view_up,view_forward,screen;
uniform vec4 plane_origin,plane_u,plane_v;
uniform vec4 background,minor_color,major_color,axis_x,axis_y,widths;
uniform vec4 level0,level1,level2;
out vec4 color;
float stroke(float coordinate,float derivative,float width){
    float distance=abs(coordinate)/max(derivative,1e-8);
    return 1.0-smoothstep(max(0.0,width*.5-.5),width*.5+.5,distance);
}
vec3 layer(vec3 base,vec2 xy,vec4 level){
    vec2 cell=xy/level.x;
    vec2 derivative=max(fwidth(cell),vec2(1e-8));
    vec2 centered=fract(cell+.5)-.5;
    float opacity=level.y+level.z;
    float major=level.z/max(opacity,1e-8);
    float width=mix(widths.x,widths.y,major);
    float coverage=max(stroke(centered.x,derivative.x,width),stroke(centered.y,derivative.y,width));
    vec4 tint=mix(minor_color,major_color,major);
    float resolved=1.0-smoothstep(.1,.5,max(derivative.x,derivative.y));
    return mix(base,tint.rgb,coverage*opacity*tint.a*resolved);
}
void main(){
    vec3 ray=view_forward.xyz+(ndc.x*screen.x/screen.y*view_right.xyz+ndc.y*view_up.xyz)/2.414213562373095;
    vec3 normal=cross(plane_u.xyz,plane_v.xyz);
    float denominator=dot(ray,normal);
    if(abs(denominator)<1e-7)discard;
    float t=dot(plane_origin.xyz-eye.xyz,normal)/denominator;
    if(t<.01)discard;
    vec3 hit=eye.xyz+t*ray;
    vec3 local=hit-plane_origin.xyz;
    vec2 xy=vec2(dot(local,plane_u.xyz),dot(local,plane_v.xyz));
    float fade=smoothstep(.02,.18,abs(dot(normalize(ray),normal)))*exp(-length(local)/(screen.w*8.0));
    vec3 grid=layer(background.rgb,xy,level0);
    grid=layer(grid,xy,level1);grid=layer(grid,xy,level2);
    vec2 d=max(fwidth(xy),vec2(1e-8));
    grid=mix(grid,axis_x.rgb,stroke(xy.y,d.y,widths.z)*axis_x.a);
    grid=mix(grid,axis_y.rgb,stroke(xy.x,d.x,widths.z)*axis_y.a);
    color=vec4(mix(background.rgb,grid,fade),background.a);
    vec4 clip=camera*vec4(hit,1);
    float depth=clip.z/clip.w*.5+.5;
    gl_FragDepth=min(1.0,depth+max(abs(dFdx(depth)),abs(dFdy(depth)))*2.0+8.0/16777216.0);
}
