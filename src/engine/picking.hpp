#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace sagomacad::engine {
enum class PickKind { Face, Edge, Body, Sketch };
struct PickRef {
    PickKind kind=PickKind::Face;
    std::string owner;
    int topology_id=0;
    bool operator==(const PickRef&) const = default;
};
struct PickPoint { double x=0,y=0,depth=0; };
struct PickTriangle { std::array<PickPoint,3> points; PickRef ref; };
struct PickSegment { std::array<PickPoint,2> points; PickRef ref; };
struct PickFilter { bool faces=true,edges=true,bodies=false,sketches=true; };
struct PickHit { PickRef ref; double depth=0; };
// Software ID/depth attachment shared by native and offscreen viewports.
class IdBuffer {
public:
    void build(int width,int height,std::vector<PickTriangle>,std::vector<PickSegment>,PickFilter={},double edge_radius=4);
    std::optional<PickRef> at(int x,int y) const;
    std::vector<PickHit> all(int x,int y) const;
private:
    int width_=0,height_=0;
    double radius_=4;
    PickFilter filter_;
    std::vector<PickTriangle> triangles_;
    std::vector<PickSegment> segments_;
    std::vector<PickRef> refs_;
    std::vector<std::uint32_t> ids_;
    std::vector<double> depths_;
};
void select_pick(std::vector<PickRef>& selection,const std::optional<PickRef>& hit,bool additive);
}
