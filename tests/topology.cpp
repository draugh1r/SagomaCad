#include "kernel/topology.hpp"
#include <cmath>
#include <iostream>
#include <numbers>
#include <set>
#include <stdexcept>
using namespace sagomacad;
using namespace kernel;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
template<class T>T ok(Result<T> r){check(bool(r),r.error);return *r.value;}
void close(double a,double b){check(std::abs(a-b)<1e-5,"mismatch "+std::to_string(a)+" / "+std::to_string(b));}
Shape box(double w,double h){return ok(name_topology(ok(make_box(w,40,h)),"base"));}
doc::TopologyRef reference(const Shape& s,const std::string& name){for(const auto& r:ok(topology(s)))if(r.name==name)return {"body",r.name,r.signature};throw std::runtime_error("missing "+name);}
int main(){try{
    auto a=box(40,20);auto top=reference(a,"base/z_max");auto rows=ok(topology(a));std::set<std::string> names;std::vector<doc::TopologyRef> vertical;
    for(const auto& r:rows){check(!r.name.empty(),"name missing");check(names.insert(r.name).second,"duplicate name "+r.name);if(r.signature.kind=="edge"&&r.signature.type=="line"&&std::abs(r.signature.direction[2])>.999)vertical.push_back({"body",r.name,r.signature});}
    check(rows.size()==26&&vertical.size()==4,"box topology counts");
    // Serialized refs contain no kernel pointers or enumeration IDs.
    auto saved=nlohmann::json(top).get<doc::TopologyRef>();
    for(auto size:{std::pair(40.,20.),std::pair(65.,35.)}){
        auto changed=box(size.first,size.second);auto resolved=ok(resolve(changed,saved,"face"));close(resolved.signature.center[2],size.second);
        auto frame=ok(face_frame(changed,resolved.id));close(frame.origin.z,size.second);
        auto swept=ok(extrude(ok(face_shape(changed,resolved.id)),{0,0,5}));auto mass=ok(mass_properties(swept));close(mass.volume,size.first*40*5);close(mass.minimum.z,size.second);
        std::vector<int> edges;for(const auto& ref:vertical){auto e=ok(resolve(changed,ref,"edge"));close(e.signature.measure,size.second);edges.push_back(e.id);}
        auto rounded=ok(fillet(changed,edges,3,"round"));auto m=ok(mass_properties(rounded));close(m.volume,(size.first*40-(4-std::numbers::pi)*9)*size.second);
        int faces=0;for(const auto& r:ok(topology(rounded)))if(r.signature.kind=="face")++faces;check(faces==10,"fillet face count");
        auto chamfered=ok(chamfer(changed,edges,2,"chamfer"));close(ok(mass_properties(chamfered)).volume,(size.first*40-8)*size.second);
        check(!fillet(changed,edges,1000,"impossible"),"impossible fillet must fail");
    }
    auto cutter=ok(name_topology(ok(translate(ok(make_box(40,40,10)),0,0,10)),"cut"));auto cut=ok(subtract(a,cutter));
    auto gone=resolve(cut,saved,"face");check(!gone&&gone.error.find("eliminata")!=std::string::npos,"deleted top must not attach to new top: "+gone.error);
    auto fallback=saved;fallback.name="base/legacy_unknown";check(bool(resolve(a,fallback,"face")),"unique signature fallback");
    fallback.signature.center[2]+=1;check(!resolve(a,fallback,"face"),"unmatched fallback rejected");
    auto pocket=ok(name_topology(ok(translate(ok(make_box(5,5,25)),10,10,0)),"pocket"));auto holed=ok(subtract(a,pocket));
    check(ok(resolve(holed,saved,"face")).name==saved.name,"Modified history preserves top alias");
    auto divider=ok(name_topology(ok(translate(ok(make_box(5,40,30)),15,0,0)),"divider"));
    auto split=ok(subtract(a,divider));auto ambiguous=resolve(split,saved,"face");
    check(!ambiguous&&ambiguous.error.find("ambiguo")!=std::string::npos,"split reference must fail instead of choosing a descendant");
    auto translated=ok(translate(a,3,4,5));close(ok(resolve(translated,saved,"face")).signature.center[2],25);
    std::cout<<"Topology: 26 stable box entities, upstream changes, face sweep, R3 on four edges, chamfer volumes, deleted face error and signature fallback OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
