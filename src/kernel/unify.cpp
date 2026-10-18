#include "kernel/shape_impl.hpp"
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <BRepTools_History.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <Standard_Failure.hxx>

namespace sagomacad::kernel {
namespace {
struct UnifyHistory {
    const BRepTools_History& history;
    const auto& Modified(const TopoDS_Shape& s) {return history.Modified(s);}
    const auto& Generated(const TopoDS_Shape& s) {return history.Generated(s);}
    bool IsDeleted(const TopoDS_Shape& s) {return history.IsRemoved(s);}
};
}
Shape unify_same_domain(const Shape& source) noexcept {
    try {
        const auto& input=ShapeAccess::get(source);
        if(!input||input->shape.IsNull())return source;
        ShapeUpgrade_UnifySameDomain unify(input->shape,true,true,false);
        unify.SetSafeInputMode(true);
        unify.Build();
        // This OCCT API has no IsDone(). Build() is checked by its result,
        // collected history and BRep validation; boolean builders check IsDone.
        if(unify.Shape().IsNull()||unify.History().IsNull()||!BRepCheck_Analyzer(unify.Shape()).IsValid())return source;
        auto result=ShapeAccess::make(unify.Shape());
        UnifyHistory history{*unify.History()};
        propagate(history,*input,*ShapeAccess::get(result));
        return result;
    }catch(const Standard_Failure&){return source;}
    catch(const std::exception&){return source;}
    catch(...){return source;}
}
}
