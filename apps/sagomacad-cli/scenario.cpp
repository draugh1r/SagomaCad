#include "scenario.hpp"
#include "shell.hpp"
#include "ui/toolbar_catalog.hpp"
#include "regen/regenerator.hpp"
#include <fstream>
#include <iostream>
#include <cmath>

namespace {
using namespace sagomacad;
using J=nlohmann::json;
struct Runner {
    engine::Engine engine;
    Shell shell;
    std::filesystem::path output;
    J saved=J::object();
    Runner(int w,int h,bool light,std::filesystem::path path):shell(w,h,1,true,light),output(std::move(path)){
        if(!shell.valid())throw std::runtime_error(shell.error());shell.bind_engine(&engine);call("ui.frame");call("ui.frame");
    }
    J call(std::string method,J p=J::object()){
        auto r=shell.control(method,p);if(r.contains("error"))throw std::runtime_error(method+": "+r["error"].dump());return r.at("result");
    }
    J resolve(J value,unsigned depth=0){
        if(depth>64)throw std::runtime_error("Reference nesting exceeds 64");
        if(value.is_object()&&value.size()==1&&value.contains("$ref"))return saved.at(J::json_pointer(value.at("$ref").get<std::string>()));
        if(value.is_array()||value.is_object())for(auto& child:value)child=resolve(child,depth+1);return value;
    }
    J point(const J& p){if(p.is_array()){if(p.size()!=2)throw std::runtime_error("Screen point needs two numbers");return {{"x",p[0]},{"y",p[1]}};}if(!p.is_object())throw std::runtime_error("Invalid point");return p;}
    J xy(const J& p){return call("ui.at",point(p));}
    void pointer(std::string kind,J p){p["kind"]=kind;call("ui.pointer",{{"events",J::array({p})}});}
    void start(const std::string& id){
        const std::string handle="toolbar:"+id;
        auto has=[&](const std::string& name){return shell.inspect()["handles"].contains(name);};
        if(!has(handle)){
            bool found=false;
            for(const auto& tab:ui::toolbar_catalog())for(const auto& group:tab.groups)for(const auto& command:group.commands)if(command.id==id){
                if(has("tab:"+std::string(tab.label)))call("ui.click",{{"handle","tab:"+std::string(tab.label)}});
                if(!has(handle)&&has("menu:"+std::string(group.label)))call("ui.click",{{"handle","menu:"+std::string(group.label)}});
                if(has(handle))found=true;
            }
            if(!found){for(const auto& command:ui::file_menu_commands())if(command.id==id)call("ui.click",{{"handle","menu:File"}});}
        }
        call("ui.click",{{"handle",handle}});
    }
    void expect(const J& spec){
        if(!spec.is_object()||spec.empty())throw std::runtime_error("expect needs nonempty object");
        auto inspect=shell.inspect();
        auto check=[&](const std::string& key,const J& actual,const J& wanted){if(actual!=wanted)throw std::runtime_error(key+": expected "+wanted.dump()+", got "+actual.dump());};
        J sketch;
        for(auto it=spec.begin();it!=spec.end();++it){const auto key=it.key();const auto& wanted=it.value();
            if(key=="tolerance")continue;
            if(key=="bodies"||key=="volume"||key=="errors"){
                const auto snapshot=regen::Regenerator{}.evaluate(engine.session().document);
                if(key=="bodies")check(key,std::count_if(snapshot.bodies.begin(),snapshot.bodies.end(),[](const auto& b){return b.final.has_value();}),wanted);
                else if(key=="errors")check(key,std::count_if(snapshot.features.begin(),snapshot.features.end(),[](const auto& f){return !f.error.empty();}),wanted);
                else {double actual=0;for(const auto& body:snapshot.bodies)if(body.final)actual+=call("engine.execute",{{"command","model.mass_properties"},{"params",{{"body",body.id}}}}).at("volume_mm3").get<double>();double target=wanted.get<double>(),tolerance=spec.value("tolerance",1e-5);if(!std::isfinite(target)||!std::isfinite(tolerance)||tolerance<0||std::abs(actual-target)>tolerance)throw std::runtime_error("volume: expected "+wanted.dump()+", got "+std::to_string(actual));}
            }else if(key=="dof"||key=="entities"||key=="dimensions"||key=="regions"){
                if(sketch.is_null())sketch=call("engine.execute",{{"command","sketch.inspect"}});
                check(key,key=="dof"?sketch.at("degrees_of_freedom"):J(sketch.at(key).size()),wanted);
            }else if(key=="active_sketch")check(key,!engine.session().active_sketch.empty(),wanted);
            else if(key=="features")check(key,engine.session().document.features.size(),wanted);
            else if(key=="rollback")check(key,engine.session().document.rollback,wanted);
            else if(key=="selection")check(key,engine.session().selection.size(),wanted);
            else if(key=="camera_settled")check(key,!shell.state().view_transition.active,wanted);
            else if(key=="dialog"){
                std::string actual=inspect["primitive"]["open"].get<bool>()?"primitive":inspect["model"]["open"].get<bool>()?"model":inspect["surface"]["open"].get<bool>()?"surface":inspect["parameters_open"].get<bool>()?"parameters":inspect["sketch"]["plane_picker"].get<bool>()?"plane":"none";check(key,actual,wanted);
            }else if(key=="handle") {if(!inspect["handles"].contains(wanted.get<std::string>()))throw std::runtime_error("Missing handle: "+wanted.dump());}
            else if(key=="value") {const auto& a=wanted.at("actual");const auto& b=wanted.at("equals");if(wanted.contains("tolerance")){double tolerance=wanted.at("tolerance");if(!a.is_number()||!b.is_number()||!std::isfinite(tolerance)||tolerance<0||std::abs(a.get<double>()-b.get<double>())>tolerance)throw std::runtime_error("value: expected "+b.dump()+", got "+a.dump());}else check(key,a,b);}
            else if(key=="ui") {for(auto q=wanted.begin();q!=wanted.end();++q)check(q.key(),inspect.at(J::json_pointer(q.key())),q.value());}
            else throw std::runtime_error("Unknown expectation: "+key);
        }
    }
    void step(J action){
        if(!action.is_object()||action.size()!=1)throw std::runtime_error("Each step needs exactly one action");action=resolve(action);
        const auto key=action.begin().key();const auto p=action.begin().value();
        if(key=="start")start(p.get<std::string>());
        else if(key=="key")call("ui.key",p.is_string()?J{{"key",p}}:p);
        else if(key=="text")call("ui.type",{{"text",p}});
        else if(key=="click"||key=="double"){auto q=point(p);if(key=="double")q["count"]=2;call("ui.click",q);}
        else if(key=="move")pointer("move",point(p));
        else if(key=="drag"){
            auto from=xy(p.at("from")),to=xy(p.at("to"));const auto count=p.value("frames",12);if(count<1||count>600)throw std::runtime_error("drag frames outside 1..600");
            pointer("move",from);pointer("down",from);
            for(int i=1;i<=count;++i){double t=double(i)/count;pointer("move",{{"x",from["x"].get<double>()*(1-t)+to["x"].get<double>()*t},{"y",from["y"].get<double>()*(1-t)+to["y"].get<double>()*t}});}pointer("up",to);
        }else if(key=="hold"){
            auto q=xy(p.at("at"));const double seconds=p.value("seconds",.65);if(!std::isfinite(seconds)||seconds<=0||seconds>10)throw std::runtime_error("hold seconds outside (0,10]");
            pointer("move",q);pointer("down",q);for(double left=seconds;left>1e-6;left-=std::min(left,1.))call("ui.frame",{{"seconds",std::min(left,1.)}});pointer("up",q);
        }else if(key=="call"){
            const auto method=p.at("method").get<std::string>();J result;
            if(method=="ui.screenshot"){shot(p.at("params").at("path").get<std::string>());result=J::object();}
            else result=call(method,p.value("params",J::object()));
            if(p.contains("save"))saved[p.at("save").get<std::string>()]=result;
        }else if(key=="expect")expect(p);
        else if(key=="until"){
            const int count=p.value("frames",120);if(count<1||count>600)throw std::runtime_error("until frames outside 1..600");std::string error;
            for(int i=0;i<count;++i){try{expect(p.at("expect"));return;}catch(const std::exception& e){error=e.what();}call("ui.frame");}throw std::runtime_error("until timed out: "+error);
        }else if(key=="shot")shot(p.get<std::string>());
        else if(key=="note")std::cout<<(p.is_string()?p.get<std::string>():p.dump())<<'\n';
        else if(key=="pending")throw std::runtime_error("pending: "+p.dump());
        else throw std::runtime_error("Unknown action: "+key);
    }
    void shot(const std::string& filename){
        const std::filesystem::path path(filename);if(path.empty()||path.is_absolute()||path.has_root_name())throw std::runtime_error("shot must be relative");for(const auto& part:path)if(part=="..")throw std::runtime_error("shot cannot escape output directory");
        std::filesystem::create_directories((output/path).parent_path());std::string error;if(!shell.screenshot((output/path).string(),error))throw std::runtime_error(error);
    }
};
}
int scenario_cli(int argc,char** argv){
    try{
        if(argc!=4||std::string(argv[2])!="run")throw std::runtime_error("Usage: sagomacad-cli scenario run file.json");
        const std::filesystem::path path(argv[3]);if(std::filesystem::file_size(path)>4*1024*1024)throw std::runtime_error("Scenario exceeds 4 MiB");std::ifstream input(path);auto spec=J::parse(input);
        // Snapshot fixtures predate scenarios; run each original call without rewriting them.
        if(spec.is_array()){
            J actions=J::array();for(const auto& entry:spec){if(!entry.is_array()||entry.size()!=2||!entry[0].is_string()||!entry[1].is_object())throw std::runtime_error("Invalid legacy call");actions.push_back({{"call",{{"method",entry[0]},{"params",entry[1]}}}});}spec={{"steps",actions}};
        }
        auto steps=spec.at("steps");if(!steps.is_array()||steps.empty()||steps.size()>10000)throw std::runtime_error("steps must contain 1..10000 actions");
        auto size=spec.value("size",J::array({1440,900}));if(!size.is_array()||size.size()!=2||!size[0].is_number_integer()||!size[1].is_number_integer()||size[0]<320||size[0]>4096||size[1]<240||size[1]>4096)throw std::runtime_error("Invalid size");
        auto themes=spec.value("themes",J::array({"light"}));if(!themes.is_array()||themes.empty()||themes.size()>2)throw std::runtime_error("Invalid themes");
        for(const auto& theme:themes){if(theme!="light"&&theme!="dark")throw std::runtime_error("Unknown theme");Runner runner(size[0],size[1],theme=="light",std::filesystem::current_path()/"scenario-evidence"/path.stem()/theme.get<std::string>());
            for(size_t i=0;i<steps.size();++i)try{runner.step(steps[i]);}catch(const std::exception& e){throw std::runtime_error(path.filename().string()+" ["+theme.get<std::string>()+"] step "+std::to_string(i+1)+" "+steps[i].dump()+": "+e.what());}
        }
        std::cout<<path.filename().string()<<": OK\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
