# Architettura — M3

| Livello | Target | Contenuto |
|---|---|---|
| L0 | `sagomacad_core` (INTERFACE) | Infrastruttura comune |
| L1 | `sagomacad_doc` | Documento JSON, espressioni con unità, dati degli schizzi |
| L2 | `sagomacad_solver` | Equazioni 2D, Levenberg–Marquardt con Eigen, rango SVD, diagnostica e profili chiusi |
| L2 | `sagomacad_kernel` | Adattatore OpenCascade con risultati ed errori |
| L3 | `sagomacad_regen` | Timeline delle primitive B-rep e stato degli schizzi |
| L3 | `sagomacad_mesh` | Tassellazione e STL |
| L4 | `sagomacad_engine` | Registro, storia, comandi, strumenti e overlay indipendenti da ImGui |
| L5 | `sagomacad_automation` | Canale di controllo e server MCP |
| L6 | `sagomacad_ui` | Stato, token, camera, proiezioni e catalogo della barra |
| App | `sagomacad_shell` | SDL3, Dear ImGui/OpenGL, disegno degli overlay e inoltro degli eventi |

Tutti i target, tranne `core`, sono librerie statiche. `sagomacad` e `sagomacad-cli` condividono la shell; i test del solver non la collegano. `solver` dipende solo da `core`, `doc` ed Eigen. Solo `kernel` include OpenCascade. `tools/check_layers.py` verifica gli include tra livelli.

Una feature `sketch` conserva piano, punti, entità, vincoli e quote in `Feature.data`. Le espressioni originali restano nel documento; gradi di libertà, conflitti e profili sono risultati derivati. UI e `sketch_inspect` usano lo stesso solver. `SketchTool` traduce eventi in parametri dei comandi; anche l'anteprima li esegue su una copia del documento. Le feature da profili restano in M4.
