#include "ui/render/gl_viewport.hpp"
#include "viewport_shaders.hpp"
#include <imgui.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace sagomacad::ui {
namespace {
PFNGLGENVERTEXARRAYSPROC pGenVertexArrays=nullptr;
PFNGLBINDVERTEXARRAYPROC pBindVertexArray=nullptr;
PFNGLDELETEVERTEXARRAYSPROC pDeleteVertexArrays=nullptr;
PFNGLGENBUFFERSPROC pGenBuffers=nullptr;
PFNGLBINDBUFFERPROC pBindBuffer=nullptr;
PFNGLBUFFERDATAPROC pBufferData=nullptr;
PFNGLDELETEBUFFERSPROC pDeleteBuffers=nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC pEnableVertexAttribArray=nullptr;
PFNGLVERTEXATTRIBPOINTERPROC pVertexAttribPointer=nullptr;
PFNGLCREATESHADERPROC pCreateShader=nullptr;
PFNGLSHADERSOURCEPROC pShaderSource=nullptr;
PFNGLCOMPILESHADERPROC pCompileShader=nullptr;
PFNGLGETSHADERIVPROC pGetShaderiv=nullptr;
PFNGLGETSHADERINFOLOGPROC pGetShaderInfoLog=nullptr;
PFNGLDELETESHADERPROC pDeleteShader=nullptr;
PFNGLCREATEPROGRAMPROC pCreateProgram=nullptr;
PFNGLATTACHSHADERPROC pAttachShader=nullptr;
PFNGLLINKPROGRAMPROC pLinkProgram=nullptr;
PFNGLGETPROGRAMIVPROC pGetProgramiv=nullptr;
PFNGLGETPROGRAMINFOLOGPROC pGetProgramInfoLog=nullptr;
PFNGLDELETEPROGRAMPROC pDeleteProgram=nullptr;
PFNGLUSEPROGRAMPROC pUseProgram=nullptr;
PFNGLGETUNIFORMLOCATIONPROC pGetUniformLocation=nullptr;
PFNGLUNIFORMMATRIX4FVPROC pUniformMatrix4fv=nullptr;
PFNGLUNIFORM4FPROC pUniform4f=nullptr;
PFNGLGENFRAMEBUFFERSPROC pGenFramebuffers=nullptr;
PFNGLBINDFRAMEBUFFERPROC pBindFramebuffer=nullptr;
PFNGLDELETEFRAMEBUFFERSPROC pDeleteFramebuffers=nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC pFramebufferTexture2D=nullptr;
PFNGLGENRENDERBUFFERSPROC pGenRenderbuffers=nullptr;
PFNGLBINDRENDERBUFFERPROC pBindRenderbuffer=nullptr;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC pRenderbufferStorageMultisample=nullptr;
PFNGLFRAMEBUFFERRENDERBUFFERPROC pFramebufferRenderbuffer=nullptr;
PFNGLDELETERENDERBUFFERSPROC pDeleteRenderbuffers=nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC pCheckFramebufferStatus=nullptr;
PFNGLBLITFRAMEBUFFERPROC pBlitFramebuffer=nullptr;
PFNGLVERTEXATTRIBDIVISORPROC pVertexAttribDivisor=nullptr;
PFNGLDRAWARRAYSINSTANCEDPROC pDrawArraysInstanced=nullptr;
bool load() {
    pVertexAttribDivisor=reinterpret_cast<PFNGLVERTEXATTRIBDIVISORPROC>(SDL_GL_GetProcAddress("glVertexAttribDivisor"));if(!pVertexAttribDivisor)return false;
    pDrawArraysInstanced=reinterpret_cast<PFNGLDRAWARRAYSINSTANCEDPROC>(SDL_GL_GetProcAddress("glDrawArraysInstanced"));if(!pDrawArraysInstanced)return false;
    pGenVertexArrays=reinterpret_cast<PFNGLGENVERTEXARRAYSPROC>(SDL_GL_GetProcAddress("glGenVertexArrays")); if(!pGenVertexArrays)return false;
    pBindVertexArray=reinterpret_cast<PFNGLBINDVERTEXARRAYPROC>(SDL_GL_GetProcAddress("glBindVertexArray")); if(!pBindVertexArray)return false;
    pDeleteVertexArrays=reinterpret_cast<PFNGLDELETEVERTEXARRAYSPROC>(SDL_GL_GetProcAddress("glDeleteVertexArrays")); if(!pDeleteVertexArrays)return false;
    pGenBuffers=reinterpret_cast<PFNGLGENBUFFERSPROC>(SDL_GL_GetProcAddress("glGenBuffers")); if(!pGenBuffers)return false;
    pBindBuffer=reinterpret_cast<PFNGLBINDBUFFERPROC>(SDL_GL_GetProcAddress("glBindBuffer")); if(!pBindBuffer)return false;
    pBufferData=reinterpret_cast<PFNGLBUFFERDATAPROC>(SDL_GL_GetProcAddress("glBufferData")); if(!pBufferData)return false;
    pDeleteBuffers=reinterpret_cast<PFNGLDELETEBUFFERSPROC>(SDL_GL_GetProcAddress("glDeleteBuffers")); if(!pDeleteBuffers)return false;
    pEnableVertexAttribArray=reinterpret_cast<PFNGLENABLEVERTEXATTRIBARRAYPROC>(SDL_GL_GetProcAddress("glEnableVertexAttribArray")); if(!pEnableVertexAttribArray)return false;
    pVertexAttribPointer=reinterpret_cast<PFNGLVERTEXATTRIBPOINTERPROC>(SDL_GL_GetProcAddress("glVertexAttribPointer")); if(!pVertexAttribPointer)return false;
    pCreateShader=reinterpret_cast<PFNGLCREATESHADERPROC>(SDL_GL_GetProcAddress("glCreateShader")); if(!pCreateShader)return false;
    pShaderSource=reinterpret_cast<PFNGLSHADERSOURCEPROC>(SDL_GL_GetProcAddress("glShaderSource")); if(!pShaderSource)return false;
    pCompileShader=reinterpret_cast<PFNGLCOMPILESHADERPROC>(SDL_GL_GetProcAddress("glCompileShader")); if(!pCompileShader)return false;
    pGetShaderiv=reinterpret_cast<PFNGLGETSHADERIVPROC>(SDL_GL_GetProcAddress("glGetShaderiv")); if(!pGetShaderiv)return false;
    pGetShaderInfoLog=reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(SDL_GL_GetProcAddress("glGetShaderInfoLog")); if(!pGetShaderInfoLog)return false;
    pDeleteShader=reinterpret_cast<PFNGLDELETESHADERPROC>(SDL_GL_GetProcAddress("glDeleteShader")); if(!pDeleteShader)return false;
    pCreateProgram=reinterpret_cast<PFNGLCREATEPROGRAMPROC>(SDL_GL_GetProcAddress("glCreateProgram")); if(!pCreateProgram)return false;
    pAttachShader=reinterpret_cast<PFNGLATTACHSHADERPROC>(SDL_GL_GetProcAddress("glAttachShader")); if(!pAttachShader)return false;
    pLinkProgram=reinterpret_cast<PFNGLLINKPROGRAMPROC>(SDL_GL_GetProcAddress("glLinkProgram")); if(!pLinkProgram)return false;
    pGetProgramiv=reinterpret_cast<PFNGLGETPROGRAMIVPROC>(SDL_GL_GetProcAddress("glGetProgramiv")); if(!pGetProgramiv)return false;
    pGetProgramInfoLog=reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(SDL_GL_GetProcAddress("glGetProgramInfoLog")); if(!pGetProgramInfoLog)return false;
    pDeleteProgram=reinterpret_cast<PFNGLDELETEPROGRAMPROC>(SDL_GL_GetProcAddress("glDeleteProgram")); if(!pDeleteProgram)return false;
    pUseProgram=reinterpret_cast<PFNGLUSEPROGRAMPROC>(SDL_GL_GetProcAddress("glUseProgram")); if(!pUseProgram)return false;
    pGetUniformLocation=reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(SDL_GL_GetProcAddress("glGetUniformLocation")); if(!pGetUniformLocation)return false;
    pUniformMatrix4fv=reinterpret_cast<PFNGLUNIFORMMATRIX4FVPROC>(SDL_GL_GetProcAddress("glUniformMatrix4fv")); if(!pUniformMatrix4fv)return false;
    pUniform4f=reinterpret_cast<PFNGLUNIFORM4FPROC>(SDL_GL_GetProcAddress("glUniform4f")); if(!pUniform4f)return false;
    pGenFramebuffers=reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(SDL_GL_GetProcAddress("glGenFramebuffers")); if(!pGenFramebuffers)return false;
    pBindFramebuffer=reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(SDL_GL_GetProcAddress("glBindFramebuffer")); if(!pBindFramebuffer)return false;
    pDeleteFramebuffers=reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(SDL_GL_GetProcAddress("glDeleteFramebuffers")); if(!pDeleteFramebuffers)return false;
    pFramebufferTexture2D=reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(SDL_GL_GetProcAddress("glFramebufferTexture2D")); if(!pFramebufferTexture2D)return false;
    pGenRenderbuffers=reinterpret_cast<PFNGLGENRENDERBUFFERSPROC>(SDL_GL_GetProcAddress("glGenRenderbuffers")); if(!pGenRenderbuffers)return false;
    pBindRenderbuffer=reinterpret_cast<PFNGLBINDRENDERBUFFERPROC>(SDL_GL_GetProcAddress("glBindRenderbuffer")); if(!pBindRenderbuffer)return false;
    pRenderbufferStorageMultisample=reinterpret_cast<PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC>(SDL_GL_GetProcAddress("glRenderbufferStorageMultisample")); if(!pRenderbufferStorageMultisample)return false;
    pFramebufferRenderbuffer=reinterpret_cast<PFNGLFRAMEBUFFERRENDERBUFFERPROC>(SDL_GL_GetProcAddress("glFramebufferRenderbuffer")); if(!pFramebufferRenderbuffer)return false;
    pDeleteRenderbuffers=reinterpret_cast<PFNGLDELETERENDERBUFFERSPROC>(SDL_GL_GetProcAddress("glDeleteRenderbuffers")); if(!pDeleteRenderbuffers)return false;
    pCheckFramebufferStatus=reinterpret_cast<PFNGLCHECKFRAMEBUFFERSTATUSPROC>(SDL_GL_GetProcAddress("glCheckFramebufferStatus")); if(!pCheckFramebufferStatus)return false;
    pBlitFramebuffer=reinterpret_cast<PFNGLBLITFRAMEBUFFERPROC>(SDL_GL_GetProcAddress("glBlitFramebuffer")); if(!pBlitFramebuffer)return false;
    return true;
}
std::array<float,16> matrix(const Camera& c,int w,int h) {
    const auto r=c.right(),u=c.up(),f=c.forward(),e=c.eye();
    constexpr double clip_near=.01,clip_far=1e9;
    const double a=(clip_far+clip_near)/(clip_far-clip_near),b=-2*clip_far*clip_near/(clip_far-clip_near),fy=2.414213562373095,fx=fy*double(h)/w;
    auto dot=[](auto x,auto y){return x.x*y.x+x.y*y.y+x.z*y.z;};
    return {float(fx*r.x),float(fy*u.x),float(a*f.x),float(f.x),
            float(fx*r.y),float(fy*u.y),float(a*f.y),float(f.y),
            float(fx*r.z),float(fy*u.z),float(a*f.z),float(f.z),
            float(-fx*dot(r,e)),float(-fy*dot(u,e)),float(b-a*dot(f,e)),float(-dot(f,e))};
}
struct Vertex {float x,y,z,nx,ny,nz;bool operator==(const Vertex&)const=default;};
}
struct GlViewport::Impl {
    const std::vector<mesh::Surface>* scene=nullptr;
    Camera queued_camera;
    Theme queued_theme{};
    std::string queued_revision;
    int queued_width=0,queued_height=0;
    float queued_scale=1;
    struct Body {
        unsigned vbo=0,lines=0;
        std::vector<Vertex> vertices;
        std::vector<float> edges;
    };
    std::vector<Body> bodies;
    unsigned vao=0,solid=0,line=0,fbo=0,resolve=0,texture=0,depth=0,mscolor=0;
    int width=0,height=0,samples=0;
    int triangle_count=0,line_count=0;
    std::string revision,error;
    std::size_t uploads=0;
    unsigned program(const char* vs,const char* fs) {
        unsigned shaders[2]{};
        for(int i=0;i<2;++i){shaders[i]=pCreateShader(i?GL_FRAGMENT_SHADER:GL_VERTEX_SHADER);const char* source=i?fs:vs;pShaderSource(shaders[i],1,&source,nullptr);pCompileShader(shaders[i]);int ok=0;pGetShaderiv(shaders[i],GL_COMPILE_STATUS,&ok);if(!ok){char log[4096]{};pGetShaderInfoLog(shaders[i],4096,nullptr,log);error=log;for(auto shader:shaders)if(shader)pDeleteShader(shader);return 0;}}
        auto result=pCreateProgram();for(auto shader:shaders){pAttachShader(result,shader);pDeleteShader(shader);}pLinkProgram(result);int ok=0;pGetProgramiv(result,GL_LINK_STATUS,&ok);if(!ok){char log[4096]{};pGetProgramInfoLog(result,4096,nullptr,log);error=log;pDeleteProgram(result);return 0;}return result;
    }
    void release_target(){
        if(fbo)pDeleteFramebuffers(1,&fbo);if(resolve)pDeleteFramebuffers(1,&resolve);
        if(depth)pDeleteRenderbuffers(1,&depth);if(mscolor)pDeleteRenderbuffers(1,&mscolor);if(texture)glDeleteTextures(1,&texture);
        fbo=resolve=depth=mscolor=texture=0;width=height=0;
    }
    bool target(int w,int h){
        if(w==width&&h==height)return true;
        release_target();
        pGenFramebuffers(1,&resolve);pBindFramebuffer(GL_FRAMEBUFFER,resolve);
        glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        pFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
        if(pCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){error="Viewport resolve framebuffer incomplete";return false;}
        pGenFramebuffers(1,&fbo);pBindFramebuffer(GL_FRAMEBUFFER,fbo);
        int max_samples=0;glGetIntegerv(GL_MAX_SAMPLES,&max_samples);samples=std::min(4,max_samples);
        pGenRenderbuffers(1,&mscolor);pBindRenderbuffer(GL_RENDERBUFFER,mscolor);pRenderbufferStorageMultisample(GL_RENDERBUFFER,samples,GL_RGBA8,w,h);pFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,mscolor);
        pGenRenderbuffers(1,&depth);pBindRenderbuffer(GL_RENDERBUFFER,depth);pRenderbufferStorageMultisample(GL_RENDERBUFFER,samples,GL_DEPTH_COMPONENT24,w,h);pFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
        if(pCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){error="Viewport MSAA framebuffer incomplete";return false;}
        width=w;height=h;return true;
    }
};
GlViewport::GlViewport():impl_(std::make_unique<Impl>()){}
GlViewport::~GlViewport(){auto& p=*impl_;if(!pDeleteProgram)return;p.release_target();for(auto& b:p.bodies){pDeleteBuffers(1,&b.vbo);pDeleteBuffers(1,&b.lines);}pDeleteVertexArrays(1,&p.vao);if(p.solid)pDeleteProgram(p.solid);if(p.line)pDeleteProgram(p.line);}
bool GlViewport::initialize(){auto& p=*impl_;if(p.vao)return p.solid&&p.line;if(!load()){p.error="OpenGL 3.3 viewport functions unavailable";return false;}if(p.solid)pDeleteProgram(p.solid);if(p.line)pDeleteProgram(p.line);p.solid=p.program(shaders::solid_vert,shaders::solid_frag);p.line=p.program(shaders::line_vert,shaders::line_frag);if(!p.solid||!p.line)return false;pGenVertexArrays(1,&p.vao);return true;}
bool GlViewport::render(const std::vector<mesh::Surface>& surfaces,const std::string& revision,const Camera& camera,int w,int h,const Theme& theme,float scale){
    auto& p=*impl_;if(w<=0||h<=0||w>16384||h>16384){p.error="Invalid viewport dimensions";return false;}if(!initialize())return false;
    int old_fbo=0,old_read_fbo=0,old_viewport[4]{},old_program=0,old_vao=0,old_buffer=0,old_texture=0,old_renderbuffer=0,old_depth_func=0;
    float old_clear[4]{},old_line_width=1;GLboolean old_depth_mask=GL_TRUE;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&old_read_fbo);glGetIntegerv(GL_RENDERBUFFER_BINDING,&old_renderbuffer);glGetIntegerv(GL_DEPTH_FUNC,&old_depth_func);glGetBooleanv(GL_DEPTH_WRITEMASK,&old_depth_mask);glGetFloatv(GL_COLOR_CLEAR_VALUE,old_clear);glGetFloatv(GL_LINE_WIDTH,&old_line_width);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&old_fbo);glGetIntegerv(GL_VIEWPORT,old_viewport);glGetIntegerv(GL_CURRENT_PROGRAM,&old_program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&old_vao);glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&old_buffer);glGetIntegerv(GL_TEXTURE_BINDING_2D,&old_texture);
    const bool depth=glIsEnabled(GL_DEPTH_TEST),blend=glIsEnabled(GL_BLEND),scissor=glIsEnabled(GL_SCISSOR_TEST),cull=glIsEnabled(GL_CULL_FACE);
    auto restore=[&]{pBindFramebuffer(GL_DRAW_FRAMEBUFFER,old_fbo);pBindFramebuffer(GL_READ_FRAMEBUFFER,old_read_fbo);pBindRenderbuffer(GL_RENDERBUFFER,old_renderbuffer);glViewport(old_viewport[0],old_viewport[1],old_viewport[2],old_viewport[3]);pUseProgram(old_program);pBindVertexArray(old_vao);pBindBuffer(GL_ARRAY_BUFFER,old_buffer);glBindTexture(GL_TEXTURE_2D,old_texture);glDepthFunc(old_depth_func);glDepthMask(old_depth_mask);glClearColor(old_clear[0],old_clear[1],old_clear[2],old_clear[3]);glLineWidth(old_line_width);if(!depth)glDisable(GL_DEPTH_TEST);if(blend)glEnable(GL_BLEND);if(scissor)glEnable(GL_SCISSOR_TEST);if(cull)glEnable(GL_CULL_FACE);};
    if(!p.target(w,h)){restore();return false;}
    pBindVertexArray(p.vao);
    if(p.revision!=revision||p.uploads==0){
        while(p.bodies.size()>surfaces.size()){auto& b=p.bodies.back();pDeleteBuffers(1,&b.vbo);pDeleteBuffers(1,&b.lines);p.bodies.pop_back();}
        for(std::size_t index=0;index<surfaces.size();++index){
            if(index==p.bodies.size()){p.bodies.emplace_back();pGenBuffers(1,&p.bodies.back().vbo);pGenBuffers(1,&p.bodies.back().lines);}
            auto& body=p.bodies[index];const auto& surface=surfaces[index];
            std::vector<Vertex> triangles;std::vector<float> lines;
            triangles.reserve(surface.geometry.triangles.size()*3);
            for(const auto& t:surface.geometry.triangles)for(auto v:t.vertices)triangles.push_back({float(v.x),float(v.y),float(v.z),float(t.normal.x),float(t.normal.y),float(t.normal.z)});
            for(const auto& edge:surface.geometry.edges)for(std::size_t i=1;i<edge.points.size();++i)for(auto v:{edge.points[i-1],edge.points[i]}){lines.push_back(float(v.x));lines.push_back(float(v.y));lines.push_back(float(v.z));}
            if(body.vertices!=triangles||body.edges!=lines){
                pBindBuffer(GL_ARRAY_BUFFER,body.vbo);pBufferData(GL_ARRAY_BUFFER,triangles.size()*sizeof(Vertex),triangles.data(),GL_STATIC_DRAW);
                pBindBuffer(GL_ARRAY_BUFFER,body.lines);pBufferData(GL_ARRAY_BUFFER,lines.size()*sizeof(float),lines.data(),GL_STATIC_DRAW);
                body.vertices=std::move(triangles);body.edges=std::move(lines);++p.uploads;
            }
        }
        p.revision=revision;
    }
    pBindFramebuffer(GL_FRAMEBUFFER,p.fbo);glViewport(0,0,w,h);glDisable(GL_SCISSOR_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_TRUE);glClearColor(theme.viewport.r,theme.viewport.g,theme.viewport.b,theme.viewport.a);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    const auto m=matrix(camera,w,h);
    auto uniforms=[&](unsigned program,Color c){pUseProgram(program);pUniformMatrix4fv(pGetUniformLocation(program,"camera"),1,GL_FALSE,m.data());pUniform4f(pGetUniformLocation(program,"body"),c.r,c.g,c.b,c.a);};
    pVertexAttribDivisor(0,0);pVertexAttribDivisor(1,0);
    uniforms(p.solid,theme.solid);
    const auto f=camera.forward();pUniform4f(pGetUniformLocation(p.solid,"toward_eye"),float(-f.x),float(-f.y),float(-f.z),0);
    for(const auto& body:p.bodies){pBindBuffer(GL_ARRAY_BUFFER,body.vbo);pEnableVertexAttribArray(0);pVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);pEnableVertexAttribArray(1);pVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(3*sizeof(float)));glDrawArrays(GL_TRIANGLES,0,int(body.vertices.size()));}
    uniforms(p.line,theme.solid_edge);pUniform4f(pGetUniformLocation(p.line,"screen"),float(w),float(h),theme.render.edge_px*scale,0);
    pVertexAttribDivisor(0,1);pVertexAttribDivisor(1,1);
    for(const auto& body:p.bodies){pBindBuffer(GL_ARRAY_BUFFER,body.lines);pVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),nullptr);pVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));pDrawArraysInstanced(GL_TRIANGLES,0,6,int(body.edges.size()/6));}
    pBindFramebuffer(GL_READ_FRAMEBUFFER,p.fbo);pBindFramebuffer(GL_DRAW_FRAMEBUFFER,p.resolve);pBlitFramebuffer(0,0,w,h,0,0,w,h,GL_COLOR_BUFFER_BIT,GL_NEAREST);
    restore();return true;
}
unsigned GlViewport::texture()const{return impl_->texture;}
bool GlViewport::enqueue(ImDrawList* list,const std::vector<mesh::Surface>& scene,const std::string& revision,const Camera& camera,Viewport v,const Theme& theme,float scale){
    auto& p=*impl_;
    if((p.width!=int(v.width)||p.height!=int(v.height))&&!render(scene,revision,camera,int(v.width),int(v.height),theme,scale))return false;
    p.scene=&scene;p.queued_revision=revision;p.queued_camera=camera;p.queued_theme=theme;p.queued_width=int(v.width);p.queued_height=int(v.height);p.queued_scale=scale;
    list->AddCallback([](const ImDrawList*,const ImDrawCmd* cmd){auto* self=static_cast<GlViewport*>(cmd->UserCallbackData);auto& p=*self->impl_;self->render(*p.scene,p.queued_revision,p.queued_camera,p.queued_width,p.queued_height,p.queued_theme,p.queued_scale);},this);
    list->AddCallback(ImDrawCallback_ResetRenderState,nullptr);
    list->AddImage(static_cast<ImTextureID>(p.texture),{float(v.x),float(v.y)},{float(v.x+v.width),float(v.y+v.height)},{0,1},{1,0});
    return true;
}
const std::string& GlViewport::error()const{return impl_->error;}
std::size_t GlViewport::uploads()const{return impl_->uploads;}
int GlViewport::samples()const{return impl_->samples;}
}
