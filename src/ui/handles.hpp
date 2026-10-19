#pragma once
#include "ui/layout.hpp"
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
namespace sagomacad::ui {
class Handles {
public:
    void clear();
    void point(std::string_view,double x,double y);
    void rect(std::string_view,LayoutRect,bool enabled=true);
    void count(std::string_view,double);
    std::optional<std::pair<double,double>> at(std::string_view) const;
    bool enabled(std::string_view) const;
    nlohmann::json json() const;
private:
    struct Entry {std::string kind;LayoutRect rect{};double value=0;bool enabled=true;};
    std::map<std::string,Entry> entries_;
};
void bind_handles(Handles*);
void publish_handle(std::string_view,double x,double y);
void publish_rect(std::string_view,LayoutRect,bool enabled=true);
void publish_count(std::string_view,double);
void publish_item(std::string_view,bool enabled=true);
}
