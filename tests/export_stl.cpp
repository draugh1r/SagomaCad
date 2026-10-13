#include "engine/command.hpp"
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <array>
#include <cstdint>

int main() {
    using namespace sagomacad;
    engine::Engine engine;
    assert(engine.execute("param.set",{{"name","larghezza"},{"expression","40 mm"},{"unit","mm"}}).ok);
    const auto added=engine.execute("primitive.add",{{"kind","box"},{"dimensions",{{"width","larghezza"},{"depth","larghezza"},{"height","20 mm"}}}});
    assert(added.ok);
    const auto path=std::filesystem::temp_directory_path()/"sagomacad-m2-export-test.stl";
    const auto exported=engine.execute("export.stl",{{"path",path.string()},{"stage","exact"},{"tolerance",0.05}});
    assert(exported.ok);
    std::ifstream file(path,std::ios::binary); assert(file);
    file.seekg(80); std::uint32_t count=0; file.read(reinterpret_cast<char*>(&count),4); assert(count>=12);
    double volume=0;
    for (std::uint32_t i=0;i<count;++i) {
        std::array<float,12> fields{}; std::uint16_t attributes=0;
        file.read(reinterpret_cast<char*>(fields.data()),48); file.read(reinterpret_cast<char*>(&attributes),2); assert(file);
        auto xyz=[&](int j)->std::array<double,3>{return {fields[j],fields[j+1],fields[j+2]};};
        const auto a=xyz(3),b=xyz(6),c=xyz(9);
        volume+=(a[0]*(b[1]*c[2]-b[2]*c[1])+a[1]*(b[2]*c[0]-b[0]*c[2])+a[2]*(b[0]*c[1]-b[1]*c[0]))/6.0;
    }
    assert(std::abs(std::abs(volume)-32000.0)<1.0);
    file.close(); std::filesystem::remove(path);
}
