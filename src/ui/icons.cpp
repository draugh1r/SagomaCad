#include "ui/icons.hpp"
#include <lunasvg.h>
#include <fstream>
#include <iostream>
#include <regex>
#include <algorithm>
#include <cctype>

namespace sagomacad::ui {
std::string_view accent_name(IconAccent a) {
    switch(a) {
        case IconAccent::Sketch:return "sketch";
        case IconAccent::Constraint:return "constraint";
        case IconAccent::Construction:return "construction";
        case IconAccent::Mesh:return "mesh";
        case IconAccent::Surface:return "surface";
        default:return "solid";
    }
}
Icons::Icons(const std::string& path,Upload upload,Delete destroy):upload_(std::move(upload)),destroy_(std::move(destroy)) {
    try { std::ifstream input(path); input>>data_; }
    catch(const std::exception& e) { std::cerr<<"Icon data: "<<e.what()<<'\n'; }
}
Icons::~Icons() { if(destroy_)for(const auto& [_,texture]:textures_)if(texture)destroy_(texture); }
std::vector<std::string> Icons::masters() const {
    std::vector<std::string> result;
    if(data_.contains("masters"))for(auto it=data_["masters"].begin();it!=data_["masters"].end();++it)result.push_back(it.key());
    return result;
}
std::string Icons::resolve_svg(std::string_view requested,IconAccent accent,bool light,unsigned pixels,bool disabled) const {
    if(!data_.contains("masters"))return {};
    std::string id(requested),ramp(accent_name(accent));
    const auto& palette=data_.at("palette");
    if(data_.at("aliases").contains(id)) {
        const auto& alias=data_["aliases"][id];
        if(alias.is_string())id=alias.get<std::string>();
        else {id=alias.at("master").get<std::string>();ramp=alias.value("accent",ramp);}
    }
    const auto& masters=data_.at("masters");
    std::string key=id;
    if(!masters.contains(key)) {
        key.clear();
        for(auto family:{"small/","model/","flat/","workspace/"}) {
            if(std::string_view(family)=="small/"&&pixels>24)continue;
            const std::string candidate=std::string(family)+id+".svg";
            if(masters.contains(candidate)){key=candidate;break;}
        }
    }
    if(key.empty())return {};
    std::map<std::string,std::string> colors;
    const char* theme=light?"light":"dark";
    const auto& roles=palette.at("roles");
    for(auto it=roles.begin();it!=roles.end();++it) {
        if(it.key()=="gap"||it.key()=="line")continue;
        const auto& role=it.value();
        std::string value;
        if(disabled) {
            const std::string muted=it.key().starts_with("active-")?"passive-"+it.key().substr(7):it.key()=="ink"?"guide":it.key();
            value=roles.at(muted).at(theme).get<std::string>();
        }
        else if(palette.at("accents").contains(ramp)&&palette["accents"][ramp].contains(it.key()))
            value=palette["accents"][ramp][it.key()].at(theme).get<std::string>();
        else value=role.at(theme).get<std::string>();
        colors[role.at("source").get<std::string>()]=value;
    }
    colors["currentColor"]=roles.at(disabled?"guide":"line").at(theme).get<std::string>();
    const std::string source=masters.at(key).get<std::string>();
    // A single pass prevents replacement output being interpreted as another source role.
    const std::regex paints("#[0-9a-fA-F]{6}|currentColor");
    std::string result;size_t offset=0;
    for(auto it=std::sregex_iterator(source.begin(),source.end(),paints);it!=std::sregex_iterator();++it) {
        const auto& m=*it;result.append(source,offset,m.position()-offset);
        std::string paint=m.str();std::transform(paint.begin(),paint.end(),paint.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(m.str()=="currentColor")paint=m.str();
        auto found=colors.find(paint);result+=found==colors.end()?m.str():found->second;
        offset=m.position()+m.length();
    }
    result.append(source,offset,std::string::npos);return result;
}
IconBitmap Icons::raster(std::string_view id,IconAccent accent,bool light,unsigned pixels,bool disabled) const {
    IconBitmap result;
    if(pixels<1||pixels>512)return result;
    result.pixels=pixels;
    try {
        auto svg=resolve_svg(id,accent,light,pixels,disabled);
        auto doc=svg.empty()?nullptr:lunasvg::Document::loadFromData(svg);
        if(!doc) {
            std::cerr<<"Icon unavailable: "<<id<<'\n';result.fallback=true;
            svg=resolve_svg("placeholder",accent,light,pixels,disabled);
            doc=svg.empty()?nullptr:lunasvg::Document::loadFromData(svg);
        }
        if(doc) {
            auto bitmap=doc->renderToBitmap(pixels,pixels);bitmap.convertToRGBA();
            if(bitmap.data()) {
                result.rgba.resize(pixels*pixels*4);
                for(unsigned y=0;y<pixels;++y)std::copy_n(bitmap.data()+y*bitmap.stride(),pixels*4,result.rgba.data()+y*pixels*4);
                return result;
            }
        }
    } catch(const std::exception& e) {std::cerr<<"Icon "<<id<<": "<<e.what()<<'\n';}
    result.fallback=true;result.rgba.assign(pixels*pixels*4,0);
    // Even absent/corrupt palette data has a visible placeholder, using a neutral mask.
    for(unsigned y=0;y<pixels;++y)for(unsigned x=0;x<pixels;++x)if(x==y||x+y==pixels-1) {
        auto* p=result.rgba.data()+(y*pixels+x)*4;p[0]=p[1]=p[2]=light?64:220;p[3]=255;
    }
    return result;
}
unsigned Icons::texture(std::string_view id,IconAccent accent,bool light,unsigned pixels,bool disabled) {
    if(!upload_||pixels<1||pixels>512)return 0;
    const auto key=std::string(id)+":"+std::to_string(int(accent))+":"+std::to_string(light)+":"+std::to_string(pixels)+":"+std::to_string(disabled);
    if(auto it=textures_.find(key);it!=textures_.end())return it->second;
    const auto bitmap=raster(id,accent,light,pixels,disabled);
    auto value=upload_(bitmap);if(value)textures_[key]=value;return value;
}
}
