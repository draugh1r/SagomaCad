#pragma once
#include <string>

namespace sagomacad::ui {
struct State {
    std::string theme = "dark";
    bool browser = true;
    bool properties = true;
    bool timeline = true;
    int width = 1440;
    int height = 900;
};
}
