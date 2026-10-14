#include "ui/parameter_editor.hpp"
#include "ui/theme.hpp"
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace sagomacad;
static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template<std::size_t N> static void fill(std::array<char,N>& target,const char* value) {
    std::snprintf(target.data(),target.size(),"%s",value);
}
int main() {
    try {
        engine::Engine engine;
        ui::State state;
        state.parameter_editing=true;
        fill(state.parameter_expression,"larghezza = 40 mm");
        require(ui::submit_parameter(state,engine),"assegnazione con Nome vuoto");
        require(engine.session().document.parameters[0].name=="larghezza","nome non estratto");
        state.parameter_editing=true;
        state.editing_parameter_name="larghezza";
        fill(state.parameter_name,"larghezza");
        fill(state.parameter_unit,"mm");
        fill(state.parameter_expression,"larghezza = 40");
        require(ui::submit_parameter(state,engine),"il numero usa l'unità della colonna");
        require(state.parameter_error.empty() &&
                std::abs(engine::evaluated_parameters_json(engine.session().document)[0]["value"].get<double>()-40)<1e-9,
                "scorciatoia con unità dichiarata");
        fill(state.parameter_name,"foro");
        fill(state.parameter_unit,"");
        fill(state.parameter_expression,"foro = larghezza / 4");
        require(ui::submit_parameter(state,engine),"assegnazione con nome coincidente");
        require(std::abs(engine::evaluated_parameters_json(engine.session().document)[1]["value"].get<double>()-10)<1e-9,
                "valore collegato");
        state.parameter_editing=true;
        fill(state.parameter_name,"foro");
        fill(state.parameter_expression,"altro = 2 mm");
        require(!ui::submit_parameter(state,engine),"nomi incoerenti accettati");
        require(state.parameter_error.find("non coincide")!=std::string::npos,"errore scorciatoia non mostrato");
        fill(state.parameter_expression,"1 / 0");
        const auto expected=engine.execute("param.set",{{"name","foro"},{"expression","1 / 0"}});
        require(!expected.ok,"divisione per zero accettata dal comando");
        require(!ui::submit_parameter(state,engine),"divisione per zero accettata dalla UI");
        require(state.parameter_error==expected.error,"messaggio del comando non arriva alla UI");
        fill(state.parameter_expression,"larghezza / 2");
        state.editing_parameter_name="foro";
        const auto preview=ui::preview_parameter(state,engine);
        require(preview.value && std::abs(*preview.value-20)<1e-9,"valore in anteprima non aggiornato");
        fill(state.parameter_comment,"Raggio nominale");
        require(ui::submit_parameter(state,engine),"correzione del parametro");
        require(state.parameter_error.empty(),"errore non ripulito dopo successo");
        require(engine.session().document.parameters[1].comment=="Raggio nominale","commento non aggiornato");
        const auto& h=ui::parameter_table_labels;
        require(std::string(h.name)=="Nome" && std::string(h.unit)=="Unità" &&
                std::string(h.expression)=="Espressione" && std::string(h.value)=="Valore" &&
                std::string(h.comment)=="Commento","intestazioni tabella");
        require(ui::light.table_header_text.r!=ui::dark.table_header_text.r &&
                ui::light.table_header.r!=ui::dark.table_header.r &&
                ui::light.error_cell.r!=ui::dark.error_cell.r,"token tema chiaro/scuro");
        state.parameter_editing=true;
        state.editing_parameter_name="larghezza";
        fill(state.parameter_name,"base");
        fill(state.parameter_expression,"80 mm");
        fill(state.parameter_unit,"mm");
        const auto before_rename=engine.session().document;
        const auto renamed_preview=ui::preview_parameter(state,engine);
        require(renamed_preview.error.empty() && renamed_preview.value && *renamed_preview.value==80,
                "anteprima rinomina deve mantenere i riferimenti");
        require(engine.session().document==before_rename,"anteprima modifica il documento");
        fill(state.parameter_name,"foro");
        const auto duplicate_preview=ui::preview_parameter(state,engine);
        require(!duplicate_preview.error.empty(),"anteprima accetta un nome duplicato");
        require(!ui::submit_parameter(state,engine) && state.parameter_error==duplicate_preview.error,
                "anteprima e conferma devono mostrare lo stesso errore");
        std::cout<<"Editor parametri: errori UI, scorciatoia, commento e temi verificati\n";
        return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
