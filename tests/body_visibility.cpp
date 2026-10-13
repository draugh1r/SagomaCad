#include "engine/command.hpp"
#include <stdexcept>
#define require(condition) do { if (!(condition)) throw std::runtime_error(#condition); } while (false)
int main() {
    sagomacad::engine::Engine engine;
    const auto added=engine.execute("primitive.add",{{"kind","sphere"},{"dimensions",{{"radius","10 mm"}}}});
    require(added.ok);
    const auto id=added.value["body_id"].get<std::string>();
    require(engine.execute("body.visibility",{{"body",id},{"visible",false}}).ok);
    require(!engine.session().document.bodies.front().visible);
    require(engine.execute("edit.undo").ok);
    require(engine.session().document.bodies.front().visible);
    require(engine.execute("edit.redo").ok);
    require(!engine.session().document.bodies.front().visible);
    require(!engine.execute("body.visibility",{{"body",id},{"visible","yes"}}).ok);
}
