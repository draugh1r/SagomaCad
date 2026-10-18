#pragma once
#include "kernel/shape.hpp"
#include <TopoDS_Shape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <algorithm>
#include <set>
namespace sagomacad::kernel {
struct NamedShape { std::string name; TopoDS_Shape shape; };
struct Shape::Impl {
    TopoDS_Shape shape;
    std::vector<NamedShape> names;
    std::set<std::string> deleted;
};
struct ShapeAccess {
    static const auto& get(const Shape& s){return s.impl_;}
    static Shape make(TopoDS_Shape s,std::vector<NamedShape> names={},std::set<std::string> deleted={}){
        return Shape(std::make_shared<Shape::Impl>(std::move(s),std::move(names),std::move(deleted)));
    }
};
// Optional cleanup: on failure preserve the boolean result and its naming.
Shape unify_same_domain(const Shape&) noexcept;
// Preserve aliases and explicit deletion through OCCT's own operation history.
// A split retains the same alias on each descendant: resolution then reports
// ambiguity, rather than silently choosing a different face by enumeration.
template<class Builder> void propagate(Builder& builder,const auto& source,auto& result){
    result.deleted.insert(source.deleted.begin(),source.deleted.end());
    TopTools_IndexedMapOfShape present;TopExp::MapShapes(result.shape,present);
    TopTools_IndexedMapOfShape inputs;TopExp::MapShapes(source.shape,inputs);
    for(const auto& n:source.names){
        if(!inputs.Contains(n.shape))continue;
        bool kept=false;
        auto add=[&](const TopoDS_Shape& s){if(s.ShapeType()!=n.shape.ShapeType()||!present.Contains(s))return;
            if(std::none_of(result.names.begin(),result.names.end(),[&](const auto& r){return r.name==n.name&&r.shape.IsSame(s);}))result.names.push_back({n.name,s});kept=true;};
        if(present.Contains(n.shape))add(n.shape);
        for(const auto& s:builder.Modified(n.shape))add(s);
        for(const auto& s:builder.Generated(n.shape))add(s);
        if(!kept&&builder.IsDeleted(n.shape))result.deleted.insert(n.name);
    }
}
}
