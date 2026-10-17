#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

namespace sagomacad::ui {
enum class IconAccent { Solid, Sketch, Constraint, Construction, Mesh, Surface, Neutral };
std::string_view accent_name(IconAccent accent);
struct IconBitmap {
    unsigned pixels=0;
    std::vector<std::uint8_t> rgba;
    bool fallback=false;
};
// Owns GL textures through injected upload/delete callbacks. Raster tests need no GL context.
class Icons {
public:
    using Upload = std::function<unsigned(const IconBitmap&)>;
    using Delete = std::function<void(unsigned)>;
    Icons(const std::string& data_path, Upload upload={}, Delete destroy={});
    ~Icons();
    Icons(const Icons&)=delete;
    Icons& operator=(const Icons&)=delete;
    IconBitmap raster(std::string_view id, IconAccent accent, bool light, unsigned pixels, bool disabled=false) const;
    unsigned texture(std::string_view id, IconAccent accent, bool light, unsigned pixels, bool disabled=false);
    std::vector<std::string> masters() const;
    std::string resolve_svg(std::string_view id, IconAccent accent, bool light, unsigned pixels, bool disabled=false) const;
private:
    nlohmann::json data_;
    Upload upload_;
    Delete destroy_;
    std::map<std::string,unsigned> textures_;
};
}
