#include "engine/command.hpp"
#include "regen/regenerator.hpp"
#include "kernel/shape.hpp"
#include "ui/feature_editor.hpp"
#include <cmath>
#include <cstdio>
#include <stdexcept>
#define require(condition) do { if (!(condition)) throw std::runtime_error(#condition); } while (false)

static double volume(const sagomacad::doc::Document& document) {
    sagomacad::regen::Regenerator regen;
    const auto& snapshot=regen.evaluate(document);
    require(!snapshot.bodies.empty() && snapshot.bodies.front().final.has_value());
    const auto result=sagomacad::kernel::mass_properties(*snapshot.bodies.front().final);
    require(result);
    return result.value->volume;
}
int main() {
    using namespace sagomacad;
    engine::Engine engine;
    require(engine.execute("param.set",{{"name","larghezza"},{"expression","40 mm"},{"unit","mm"}}).ok);
    const auto added=engine.execute("primitive.add",{{"kind","box"},{"dimensions",{{"width","larghezza"},{"depth","larghezza"},{"height","20 mm"}}}});
    require(added.ok);
    const auto id=added.value["feature_id"].get<std::string>();
    require(std::abs(volume(engine.session().document)-32000.0)<1e-6);
    const auto later=engine.execute("primitive.add",{{"kind","sphere"},{"dimensions",{{"radius","2 mm"}}}});
    require(later.ok);
    const auto original=engine.session().document;
    const auto original_steps=engine.session().history.size();
    ui::State ui;
    require(ui::begin_feature_edit(ui,original,id));
    require(std::string(ui.primitive_dimensions[0].data())=="larghezza");
    std::snprintf(ui.primitive_dimensions[0].data(),ui.primitive_dimensions[0].size(),"60 mm");
    const auto preview=ui::feature_preview(ui,original);
    require(preview.rollback==1);
    require(std::abs(volume(preview)-48000.0)<1e-6);
    require(engine.session().document==original && engine.session().history.size()==original_steps);
    ui.primitive_editor_open=false;
    require(ui::feature_preview(ui,original)==original);
    require(engine.execute("feature.edit",{{"feature_id",id},{"dimensions",{{"width","60 mm"}}}}).ok);
    require(std::abs(volume(engine.session().document)-48000.0)<1e-6);
    require(engine.session().history.size()==original_steps+1);
    require(engine.execute("edit.undo").ok);
    require(engine.session().document==original && std::abs(volume(engine.session().document)-32000.0)<1e-6);
    require(engine.execute("edit.redo").ok);
    require(std::abs(volume(engine.session().document)-48000.0)<1e-6);
    require(!engine.execute("feature.edit",{{"feature_id",id},{"dimensions",{{"width","0 mm"}}}}).ok);
    require(engine.execute("feature.suppress",{{"feature_id",id},{"suppressed",true}}).ok);
    require(engine.session().document.features.at(id).suppressed);
    require(engine.execute("edit.undo").ok && !engine.session().document.features.at(id).suppressed);
    require(engine.execute("feature.delete",{{"feature_id",id}}).ok);
    require(!engine.session().document.features.contains(id) && engine.session().document.timeline.size()==1);
    require(engine.execute("edit.undo").ok && engine.session().document.features.contains(id));

    const auto before_failed_edit=engine.session().document;
    const auto steps_before_failed_edit=engine.session().history.size();
    require(!engine.execute("primitive.edit",{{"feature_id",id},{"dimensions",{{"width","90 mm"}}},{"name",42}}).ok);
    require(engine.session().document==before_failed_edit);
    require(engine.session().history.size()==steps_before_failed_edit);

    require(engine.execute("param.set",{{"old_name","larghezza"},{"name","lato"},{"expression","40 mm"}}).ok);
    require(engine.session().document.features.at(id).data["dimensions"]["depth"]=="lato");
    require(std::abs(volume(engine.session().document)-48000.0)<1e-6);
    const auto before_delete=engine.session().document;
    require(!engine.execute("param.delete",{{"name","lato"}}).ok);
    require(engine.session().document==before_delete);
    require(engine.execute("edit.undo").ok);
    require(engine.session().document.features.at(id).data["dimensions"]["depth"]=="larghezza");
}
