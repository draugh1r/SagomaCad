#pragma once
#include <string>

namespace sagomacad::ui {
struct State {
    std::string theme = "light";
    bool browser = true;
    bool properties = false;
    bool timeline = true;
    bool sketch_mode = false;
    int width = 1440;
    int height = 900;
};
}
