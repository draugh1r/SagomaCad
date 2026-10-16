#pragma once
#include "doc/document.hpp"
#include "kernel/shape.hpp"
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace sagomacad::regen {
struct FeatureState {
    std::string id;
    std::string error;
    bool cached=false;
};
struct BodyState {
    std::string id;
    std::optional<kernel::Shape> exact;
    std::optional<kernel::Shape> final;
    std::string error;
};
struct Snapshot {
    std::vector<FeatureState> features;
    std::vector<BodyState> bodies;
    std::size_t cache_hits=0;
    std::vector<std::string> hidden_sketches;
};
bool sketch_visible(const doc::Document&,const Snapshot&,const std::string& id,const std::string& editing={});
class Regenerator {
public:
    struct Cached { std::optional<kernel::Shape> shape; std::string error; };
    const Snapshot& evaluate(const doc::Document& document);
private:
    struct Step {std::string key;FeatureState feature;std::vector<BodyState> changed_bodies;};
    std::vector<Step> steps_;
    Snapshot snapshot_;
};
} // namespace sagomacad::regen
