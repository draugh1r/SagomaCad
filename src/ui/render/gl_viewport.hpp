#pragma once
#include "ui/camera.hpp"
#include "ui/render/grid.hpp"
#include "ui/theme.hpp"
#include "mesh/surface.hpp"
#include <memory>
#include <string>
#include <vector>
struct ImDrawList;

namespace sagomacad::ui {
// Owns GL objects; destroy before the owning SDL context. Scene uploads are independent of camera updates.
class GlViewport {
public:
    struct Style {bool faces=true,edges=true,silhouettes=true;};
    struct Layer {
        mesh::Surface surface;
        Color face=transparent,edge=transparent;
        float width=light.render.edge_px;
        bool grid_occluder=false,lit=false;
    };
    GlViewport();
    ~GlViewport();
    GlViewport(const GlViewport&)=delete;
    bool initialize();
    bool render(const std::vector<mesh::Surface>& surfaces,const std::string& revision,
                const Camera&,int width,int height,const Theme&,float scale=1);
    bool enqueue(ImDrawList*,const std::vector<mesh::Surface>&,const std::string& revision,
                 const Camera&,Viewport,const Theme&,float scale=1);
    unsigned texture() const;
    const std::string& error() const;
    std::size_t uploads() const;
    int samples() const;
    void set_style(Style);
    void set_grid(GridPlane);
    void set_layers(std::vector<Layer>);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
