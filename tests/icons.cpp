#include "ui/icons.hpp"
#include <iostream>
#include <stdexcept>
using namespace sagomacad::ui;
void check(bool v,const char* msg){if(!v)throw std::runtime_error(msg);}
int main(int argc,char** argv) try {
    check(argc==2,"data path");unsigned uploads=0,deletes=0;
    {
        Icons icons(argv[1],[&](const IconBitmap& b){check(!b.rgba.empty(),"upload bitmap");return ++uploads;},[&](unsigned){++deletes;});
        const auto masters=icons.masters();check(masters.size()>=268,"all masters loaded");
        for(const auto& id:masters)for(bool light:{true,false}) {
            const auto image=icons.raster(id,IconAccent::Solid,light,24);
            check(!image.fallback&&image.rgba.size()==24*24*4,"master rasterizes");
            bool visible=false;for(size_t i=3;i<image.rgba.size();i+=4)visible|=image.rgba[i]>0;
            if(!visible)throw std::runtime_error("empty master: "+id);
        }
        auto light=icons.raster("extrude",IconAccent::Solid,true,32);
        auto dark=icons.raster("extrude",IconAccent::Solid,false,32);
        check(light.rgba!=dark.rgba,"theme changes pixels");
        check(light.rgba!=icons.raster("extrude",IconAccent::Sketch,true,32).rgba,"accent changes pixels");
        check(light.rgba!=icons.raster("extrude",IconAccent::Solid,true,32,true).rgba,"disabled roles");
        check(icons.raster("surface_extrude",IconAccent::Solid,true,32).rgba==icons.raster("extrude",IconAccent::Surface,true,32).rgba,"alias accent");
        auto first=icons.texture("extrude",IconAccent::Solid,true,32);
        check(first==icons.texture("extrude",IconAccent::Solid,true,32)&&uploads==1,"texture cache hit");
        check(first!=icons.texture("extrude",IconAccent::Solid,false,32),"theme cache key");
        check(first!=icons.texture("extrude",IconAccent::Sketch,true,32),"accent cache key");
        check(first!=icons.texture("extrude",IconAccent::Solid,true,64),"DPI cache key");
        check(first!=icons.texture("extrude",IconAccent::Solid,true,32,true),"disabled cache key");
        check(icons.raster("missing",IconAccent::Solid,true,16).fallback,"missing icon fallback");
        check(icons.raster("extrude",IconAccent::Solid,true,9999999).rgba.empty(),"pixel limit");
        // Knockout beneath the foreground must expose real alpha, not a painted white background.
        auto knockout=icons.resolve_svg("new",IconAccent::Solid,true,64);
        check(knockout.find("sagoma_gap_")!=std::string::npos,"gap compiled as mask");
        const auto badge=icons.raster("new",IconAccent::Solid,true,64);
        auto alpha=[&](unsigned x,unsigned y){return badge.rgba[(y*64+x)*4+3];};
        check(alpha(44,45)==0,"plus exposes transparency through earlier artwork");
        check(alpha(43,33)<32,"gap erases the document outside the badge foreground");
        check(alpha(44,34)<32,"GAP stroke also erases its own fill");
        check(alpha(47,39)>200,"later foreground survives knockout");
        std::cout<<masters.size()<<" masters in both themes; cache, aliases and roles verified\n";
    }
    check(uploads==deletes,"textures released");
    Icons absent(std::string(argv[1])+".missing");
    auto fallback=absent.raster("extrude",IconAccent::Solid,true,16);
    check(fallback.fallback&&fallback.rgba.size()==16*16*4,"missing palette placeholder");
    return 0;
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
