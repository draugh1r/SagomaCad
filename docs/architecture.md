# Architettura — M4

| Livello | Target | Contenuto |
|---|---|---|
| L0 | `sagomacad_core` (INTERFACE) | Infrastruttura comune |
| L1 | `sagomacad_doc` | Documento JSON, espressioni con unità, dati degli schizzi |
| L2 | `sagomacad_solver` | Equazioni 2D, Levenberg–Marquardt con Eigen, rango SVD, diagnostica e profili chiusi |
| L2 | `sagomacad_kernel` | Primitive, facce esatte con fori, prismi, rivoluzioni e booleane OpenCascade con risultati ed errori |
| L3 | `sagomacad_regen` | Timeline incrementale di schizzi, primitive, estrusioni e rivoluzioni; rollback ed errori isolati |
| L3 | `sagomacad_mesh` | Tassellazione e STL |
| L4 | `sagomacad_engine` | Registro, storia, comandi, strumenti e overlay indipendenti da ImGui |
| L5 | `sagomacad_automation` | Canale di controllo e server MCP |
| L6 | `sagomacad_ui` | Stato, token, layout Clay, widget ImGui, camera, proiezioni e catalogo della barra |
| App | `sagomacad_shell` | SDL3, Dear ImGui/OpenGL, disegno degli overlay e inoltro degli eventi |

Tutti i target, tranne `core`, sono librerie statiche. `sagomacad` e `sagomacad-cli` condividono la shell; i test del solver non la collegano. `solver` dipende solo da `core`, `doc` ed Eigen. Solo `kernel` include OpenCascade. `tools/check_layers.py` verifica gli include tra livelli.

Una feature `sketch` conserva piano, punti, entità, vincoli e quote in `Feature.data`. Le espressioni originali restano nel documento; gradi di libertà, conflitti e profili sono risultati derivati. UI e `sketch_inspect` usano lo stesso solver. Le regioni hanno contorno e fori immediatamente annidati, con ID canonico basato sugli ID delle entità. Estrusioni e rivoluzioni conservano questi riferimenti, espressioni, operazione e destinazioni; il kernel riceve lo schizzo risolto e costruisce curve esatte.

La cache della regen conserva per ogni passo chiave di input, stato ed esiti dei soli corpi modificati. La chiave comprende i dati della feature e i valori delle espressioni: un parametro estraneo non invalida il prefisso. La prima differenza invalida il suffisso; rollback conserva risultati successivi riutilizzabili. Soppressione, riferimenti mancanti e errori delle booleane non impediscono di valutare i passi successivi. Gli schizzi consumati sono una proprietà derivata dello snapshot.

`SketchTool` e `engine/tools/model_preview` producono dati per la vista. Le anteprime di modellazione usano il contesto prima della feature modificata, generano utensile e mesh e verificano le destinazioni senza scrivere nel documento. La UI conserva solo selezioni, espressioni temporanee e stato dei gesti; il commit passa da `feature.extrude`, `feature.revolve` o `feature.edit`. La guida MCP è incorporata a build dalla stessa `docs/modeling-guide.md` eseguita dai test.

## Motore di layout della shell

Clay 0.14 è una dipendenza header-only vcpkg fissata dal baseline del manifest.
`src/ui/clay.cpp` ne compila l’implementazione C++20; nessun header Clay esce da
`ui/`. `tools/check_layers.py` vieta Clay ai livelli inferiori, come ImGui, SDL e OpenGL.

La shell non usa dockspace. L’albero in `ui/layout.cpp` è una colonna topbar/ribbon,
una riga browser/viewport/proprietà/area contestuale vuota e la timeline in basso;
i separatori sono nodi dello stesso albero. I gruppi del ribbon vengono dichiarati
dal catalogo dei comandi, con pulsanti sopra e menu/etichetta sotto. Tutte le misure
provengono da `ui/theme.hpp`. Ogni frame: stato → dichiarazione Clay → rettangoli
`ShellLayout` → finestre e widget ImGui → disegno OpenGL esistente. Il renderer
ImDrawList gestisce solo decorazioni e clip; l’input rimane in ImGui.

Un’arena per istanza viene allocata una volta, con 512 elementi e 1024 parole.
Il callback misura il font ImGui già caricato alla scala DPI del frame. Il gestore
di errore registra la diagnostica e mantiene l’ultimo layout valido, comprese copie
indipendenti dei comandi decorativi. Ogni istanza ripristina il contesto Clay precedente.

Dimensioni dei pannelli, visibilità e compressione appartengono a `ui::State`,
non al documento CAD né alla storia undo. `layout_state_json` e `restore_layout_state`
forniscono la serializzazione con validazione; l’app salva le modifiche nella
cartella preferenze SDL. I separatori aggiornano lo stato, Clay ne limita la
rappresentazione per proteggere il viewport quando la finestra si restringe.

Per aggiungere un pannello: dichiarare un token, aggiungere il suo rettangolo a
`ShellLayout`, dichiarare il nodo con ID stabile in `ui/layout.cpp` e leggerne il
risultato dopo `Clay_EndLayout`. Per esempio, un figlio di `content` con
`.id=id("new_panel")` e `.layout={.sizing={CLAY_SIZING_FIXED(t.new_panel_width*z),
CLAY_SIZING_GROW(0)}}` viene esposto con `candidate.new_panel=rect("new_panel")`.
Aggiornare budget del viewport, ispezione e test di sovrapposizione. Esempio esteso,
albero e formato delle preferenze sono in `docs/ui-design.md`.

## Booleane e domini coincidenti

I risultati persistenti di Fuse, Cut e Common passano da `kernel::unify_same_domain`: ShapeUpgrade_UnifySameDomain unisce facce e spigoli, in SafeInputMode. La storia della booleana viene prima trasferita al risultato grezzo; poi History() della pulizia rimappa gli stessi alias sulle facce e sugli spigoli fusi. Le eliminazioni restano esplicite e i riferimenti divisi restano ambigui: nessuna selezione per vicinanza.

La pulizia è opzionale: se Build() genera Standard_Failure, produce una forma nulla/non valida o non fornisce storia, si conserva l’intero risultato grezzo con i suoi nomi. OCCT 8 non espone IsDone() per questo algoritmo; si controllano forma, storia e BRepCheck_Analyzer, mentre IsDone() è verificato sulle booleane. Il passaggio copre anche i fori nei profili, i fori solidi tramite subtract e le intersezioni di misura. Esempi numerici e screenshot in docs/design/verifica-booleane/.
