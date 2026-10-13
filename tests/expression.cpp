#include "doc/expression.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace sagomacad::doc;

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
static bool near(double a, double b) { return std::abs(a - b) < 1e-9; }

int main() {
    try {
        Document document;
        document.parameters = {{"larghezza", "40 mm", ""}, {"foro", "larghezza / 4", ""},
                               {"imperiale", "1 in", "in"}};
        auto rows = evaluate_parameters(document);
        require(rows.size() == 3, "numero di parametri");
        require(rows[0].value && near(*rows[0].value, 40.0) && rows[0].unit == "mm", "40 mm");
        require(rows[1].value && near(*rows[1].value, 10.0) && rows[1].unit == "mm", "riferimento");
        require(rows[2].value && near(*rows[2].value, 1.0) && rows[2].unit == "in", "1 in");

        Quantity result;
        std::string error;
        require(evaluate_expression(document, "(larghezza + 1 in) / 2", result, error), error.c_str());
        require(result.dimension == Dimension::Length && near(result.value, 32.7), "conversione unità");
        require(!evaluate_expression(document, "larghezza + 2", result, error) &&
                error.find("Unità incompatibili") != std::string::npos, "unità incompatibili");
        require(!evaluate_expression(document, "larghezza / 0", result, error) &&
                error.find("Divisione per zero") != std::string::npos, "divisione per zero");
        require(!evaluate_expression(document, "ignoto + 2 mm", result, error) &&
                error.find("Parametro sconosciuto") != std::string::npos, "riferimento sconosciuto");
        require(!evaluate_expression(document, "1e309 mm", result, error), "numero enorme accettato");
        require(!evaluate_expression(document, std::string(256, '-') + "1", result, error),
                "catena unaria senza limite");

        document.parameters = {{"a", "b + 1 mm", ""}, {"b", "a + 1 mm", ""}};
        rows = evaluate_parameters(document);
        require(!rows[0].value && !rows[1].value &&
                rows[0].error.find("Riferimento circolare") != std::string::npos &&
                rows[0].error.find("a -> b -> a") != std::string::npos, "ciclo non riconosciuto");
        std::cout << "Espressioni: unità, riferimenti, cicli e errori verificati\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
