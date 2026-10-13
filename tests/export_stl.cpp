#include "engine/command.hpp"
#include <stdexcept>
#define require(condition) do { if (!(condition)) throw std::runtime_error(#condition); } while (false)
#include <cmath>
#include <filesystem>
#include <fstream>
#include <array>
#include <cstdint>

static double volume_from_stl(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary); require(file);
    file.seekg(80); std::uint32_t count=0; file.read(reinterpret_cast<char*>(&count),4); require(count>=12);
    require(std::filesystem::file_size(path)==84u+50u*count);
    double volume=0;
    for (std::uint32_t i=0;i<count;++i) {
        std::array<float,12> fields{}; std::uint16_t attributes=0;
        file.read(reinterpret_cast<char*>(fields.data()),48); file.read(reinterpret_cast<char*>(&attributes),2); require(file);
        auto xyz=[&](int j)->std::array<double,3>{return {fields[j],fields[j+1],fields[j+2]};};
        const auto a=xyz(3),b=xyz(6),c=xyz(9);
        volume+=(a[0]*(b[1]*c[2]-b[2]*c[1])+a[1]*(b[2]*c[0]-b[0]*c[2])+a[2]*(b[0]*c[1]-b[1]*c[0]))/6.0;
    }
    return std::abs(volume);
}
int main() {
    using namespace sagomacad;
    engine::Engine engine;
    require(engine.execute("param.set",{{"name","larghezza"},{"expression","40 mm"},{"unit","mm"}}).ok);
    const auto added=engine.execute("primitive.add",{{"kind","box"},{"dimensions",{{"width","larghezza"},{"depth","larghezza"},{"height","20 mm"}}}});
    require(added.ok);
    const auto requested=std::filesystem::temp_directory_path()/"sagomacad-m2-export-test";
    std::filesystem::path path;
    const auto export_box=[&](const char* stage) {
        const auto result=engine.execute("export.stl",{{"path",requested.string()},{"stage",stage},{"tolerance",0.05}});
        require(result.ok);
        path=result.value.at("path").get<std::string>();
        require(path.filename()==requested.filename().string()+".stl");
        require(path.extension()==".stl");
        require(path.parent_path().filename()=="export");
        require(std::filesystem::exists(path));
        require(!std::filesystem::exists(requested));
    };
    export_box("exact"); require(std::abs(volume_from_stl(path)-32000.0)<1.0);
    require(engine.execute("param.set",{{"name","larghezza"},{"expression","60 mm"},{"unit","mm"}}).ok);
    export_box("final"); require(std::abs(volume_from_stl(path)-72000.0)<1.0);
    require(!engine.execute("export.stl",{{"path",requested.string()},{"tolerance",0.0}}).ok);
    std::filesystem::remove(path);
}
