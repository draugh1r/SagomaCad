# Piano di sviluppo - SagomaCad

Documento da dare a Codex come guida del progetto. Va letto insieme ad `AGENTS.md`.
Il nome del progetto è SagomaCad; identificatori tecnici, binari e formato file usano `sagomacad`.

---

## 0. Obiettivo

Un CAD parametrico desktop, locale e gratuito, con:

- **il modo di lavorare di Fusion 360**: schizzi 2D con vincoli e quote, feature (estrusione, taglio, rivoluzione, raccordo, smusso, foro), timeline parametrica modificabile, browser degli oggetti, pannello proprietà;
- **l'immediatezza di Tinkercad**: primitive da trascinare, fori drag and drop, appoggio di una faccia su un'altra con snap, push and pull diretto sulle facce, selezione a clic ripetuto sugli oggetti sovrapposti. Ogni gesto rapido genera feature parametriche vere nella timeline, mai geometria "morta";
- **file in locale** e export rapido (STL, 3MF, STEP) pensato per la stampa 3D.

### Non obiettivi (per ora)

- Assiemi con giunti, disegni tecnici 2D, CAM, simulazioni, collaborazione cloud.
- Scrivere un kernel geometrico da zero: si usa OpenCascade.
- Copiare grafica, icone o asset di Autodesk: si replicano layout e comportamento, non gli asset.

---

## 1. Cosa prendiamo da PhotoCraft (storytold/photocraft)

PhotoCraft ha ricreato l'interfaccia di Photoshop in un solo linguaggio con agenti AI. Il metodo, non il codice, è quello che copiamo:

1. **Motore prima, UI dopo.** Il documento è fatto di soli dati. Tutto quello che fa la UI deve essere possibile anche da CLI e da canale di controllo, senza finestra.
2. **Tutto è un comando.** Ogni azione dell'utente ha un id stabile (`feature.extrude`), parametri JSON documentati, una funzione `enabled` e una `run`. Menu, toolbar, scorciatoie, palette dei comandi, CLI e canale di controllo chiamano tutti lo stesso registro.
3. **Gli strumenti producono dati.** Uno strumento riceve eventi del puntatore e restituisce operazioni da eseguire e "overlay" da disegnare (linee, maniglie, quote, testo) come semplici strutture. La UI li disegna e basta.
4. **Livelli imposti in build.** Un modulo può dipendere solo da moduli di livello inferiore. Sotto la UI nessuno include ImGui, SDL o OpenGL. Un controllo automatico lo verifica.
5. **UI sottile e guidata dallo stato.** Lo stato della UI (strumento attivo, pannelli, vista, dialog aperti) è una struttura serializzabile, così il canale di controllo può leggerlo e modificarlo.
6. **L'agente vede quello che costruisce.** Renderer offscreen che produce PNG da uno script di comandi, più canale di controllo sull'app in esecuzione con `ui.screenshot`. Nessuna modifica visiva è finita senza screenshot controllato.
7. **Mai crashare.** Input malformato, file corrotti e operazioni geometriche fallite producono errori leggibili, non crash. Un test "fuzz" lancia ogni comando con parametri ostili.
8. **Un registro di sviluppo** (`log/devlog.md`) aggiornato a ogni task, così il prossimo agente riparte da dove si è fermato il precedente.

---

## 2. Stack tecnico

| Area | Scelta | Motivo |
|---|---|---|
| Linguaggio | C++20 | Accesso diretto e nativo a OpenCascade, un solo linguaggio in tutto il progetto |
| Build | CMake + vcpkg (manifest `vcpkg.json`) | OpenCascade, SDL3, ImGui, Eigen disponibili in vcpkg su Windows, macOS e Linux |
| Kernel geometrico | OpenCascade (OCCT) 7.8 o successivo | B-rep esatto, booleane, raccordi, tassellazione, STEP |
| Finestra e input | SDL3 | Multipiattaforma, gestisce finestra, tastiera, mouse, DPI |
| Grafica | OpenGL 3.3 core (4.1 su macOS) | Sufficiente per un viewport CAD, gira su Mac nonostante la deprecazione |
| Interfaccia | Dear ImGui, ramo `docking` | Immediate mode, l'equivalente di egui usato da PhotoCraft: layout densi senza lottare con un sistema a componenti |
| Icone | Lucide (SVG, licenza ISC) renderizzate con lunasvg | Niente asset proprietari |
| Font | Inter (UI) e JetBrains Mono (numeri), licenza OFL | Come PhotoCraft |
| Solver vincoli | Scritto da noi, Newton-Raphson / Levenberg-Marquardt con Eigen | Problema matematico chiuso, ottimo da scrivere in casa |
| JSON | nlohmann/json | Comandi, file di progetto, canale di controllo |
| Test | doctest | Leggero, veloce da compilare |
| PNG | stb_image_write | Screenshot e golden image |

**Alternativa valutata:** interfaccia web con Tauri e motore C++ in un processo separato. Scartata per ora perché obbliga a tre linguaggi e a un ponte tra processi, e perché il ciclo screenshot è più semplice e deterministico con un renderer nativo offscreen. Grazie alla separazione dei livelli, se un giorno si cambia idea si riscrive solo il livello UI.

---

## 3. Architettura a livelli

```text
src/
  L0  core/        matematica (vettori, matrici, trasformazioni), Result<T>, errori, id, log
  L1  doc/         modello del documento: SOLO DATI (parametri, schizzi, vincoli, feature, timeline, corpi)
  L2  solver/      solver dei vincoli 2D (dipende solo da core e doc)
  L2  kernel/      adattatore OpenCascade: L'UNICO modulo che include header OCCT
  L3  regen/       valutazione della timeline: doc -> forme B-rep, riferimenti topologici
  L3  mesh/        tassellazione per la vista, export STL / 3MF / STEP
  L4  engine/      Session, CommandRegistry, undo/redo, eventi, view-model, interfaccia Tool, overlay
  L5  automation/  canale di controllo JSON (TCP loopback con token), runner headless
  L6  ui/          shell Dear ImGui (sottile): pannelli, toolbar, viewport, disegno overlay
apps/
  sagomacad/           app desktop (SDL3 + OpenGL + ImGui), opzione --control
  sagomacad-cli/       CLI headless: run, export, snapshot, commands, info
tools/
  check_layers.py  verifica che nessun modulo includa livelli superiori
tests/             test per modulo + golden image + fuzz comandi
docs/              architettura, protocollo di controllo, design UI, roadmap
docs/reference/    screenshot di riferimento di Fusion e Tinkercad con note (non vanno nel binario)
log/devlog.md      registro di sviluppo
```

Regole:

- Ogni modulo è un target CMake separato. Un modulo dichiara come dipendenze solo moduli di livello inferiore.
- `tools/check_layers.py` analizza gli `#include` e fallisce se un livello include un livello superiore, se un modulo fuori da `kernel/` include header OCCT, o se un modulo sotto `ui/` include ImGui, SDL o OpenGL.
- `doc/` non contiene puntatori a oggetti OCCT: solo dati serializzabili. Le forme calcolate vivono in una cache di `regen/`, indicizzata per revisione del documento.

---

## 4. Modello del documento (`doc/`)

```text
Document
  parameters: [Parameter { name, expression, unit }]        // "larghezza = 40 mm", "foro = larghezza / 4"
  timeline:   [FeatureId]                                   // ordine di valutazione
  rollback:   index                                         // marker della timeline
  features:   map<FeatureId, Feature>
  bodies:     [Body { id, name, visible, color }]

Feature (variant)
  Sketch     { plane: PlaneRef, entities, constraints, dimensions }
  Extrude    { sketch, profiles, extent, direction, operation: new|join|cut|intersect, target_bodies }
  Revolve    { sketch, profiles, axis, angle, operation }
  Fillet     { edges: [EdgeRef], radius }
  Chamfer    { edges: [EdgeRef], distance }
  Hole       { face: FaceRef, position, diameter, depth | through_all }
  Primitive  { kind: box|cylinder|sphere|cone|torus, placement, dimensions, operation }
  Place      { body, from: FaceRef, to: FaceRef, offset, snap }   // "appoggia faccia su faccia"
  Each feature: { id, name, suppressed, error: optional<string> }
```

- Valori numerici sempre come espressioni (`"40 mm"`, `"larghezza / 2"`), valutate da un piccolo parser in `doc/` o `core/`.
- File di progetto: `.sagomacad` = JSON con versione di formato. Ogni campo nuovo ha un default, così i file vecchi si aprono sempre.
- Salvataggio atomico: scrivere su file temporaneo e poi rinominare.

### Riferimenti topologici (il problema difficile)

Le feature successive devono ritrovare facce e spigoli anche quando una feature precedente cambia. Strategia per la prima versione:

1. Ogni feature assegna **nomi stabili** alle facce che genera, partendo dalla sua origine. Esempio per un'estrusione: `cap_start`, `cap_end`, `side(<id entità dello schizzo>)`.
2. Le booleane propagano i nomi usando la storia di OCCT (`Generated`, `Modified`, `IsDeleted` di `BRepAlgoAPI_*` e `BRepFilletAPI_*`).
3. Un `FaceRef` / `EdgeRef` salva il nome stabile più una "firma geometrica" di riserva (tipo di superficie, normale, baricentro, area).
4. Se il nome non si risolve, si prova la firma geometrica. Se fallisce anche quella, la feature va in errore (rossa nella timeline, con messaggio) e la regen continua con le altre. Mai crash.

Test dedicati: cambiare una quota a monte e verificare che raccordi e fori a valle restino sulle facce giuste.

---

## 5. Comandi (`engine/`)

```cpp
struct CommandSpec {
    std::string_view id;          // "feature.extrude"
    std::string_view label;       // "Estrudi"
    std::vector<std::string_view> menu;   // {"Solido", "Crea"}
    std::optional<std::string_view> shortcut;   // "E"
    std::string_view params_doc;  // descrizione JSON dei parametri, leggibile da umani e agenti
    Enabled enabled;              // (const Session&) -> Result<void>
    Run run;                      // (Session&, const json& params) -> Result<json>
    bool journal;                 // false per le query
};
```

- Ogni gruppo di comandi vive nel suo file (`engine/cmds/sketch_cmds.cpp`, `feature_cmds.cpp`, ...) con una funzione `specs()`. Il registro le unisce.
- Ogni comando che modifica il documento passa da `History`: un comando = un passo di undo. I trascinamenti continui usano la "coalescenza" (un drag = un solo passo).
- `run` valida tutti i parametri e restituisce errore, mai eccezioni non gestite né crash.

### Comandi del primo traguardo (MVP)

```text
document.new  document.open {path}  document.save {path?}  document.inspect
edit.undo  edit.redo  edit.delete {ids}
param.set {name, expression}  param.list
sketch.create {plane: "XY"|"XZ"|"YZ" | face: FaceRef}  sketch.finish  sketch.edit {feature}
sketch.line {points}  sketch.rect {corner, corner2 | center, size}  sketch.circle {center, diameter}  sketch.arc {...}
sketch.constraint.add {kind, entities}   // coincident, horizontal, vertical, parallel, perpendicular, tangent, equal, midpoint, fix
sketch.dimension.add {kind, entities, value}   // distance, length, diameter, radius, angle
sketch.dimension.set {id, value}
feature.extrude {sketch, profiles, distance, direction, operation}
feature.revolve {sketch, profiles, axis, angle, operation}
feature.fillet {edges, radius}  feature.chamfer {edges, distance}
feature.hole {face, position, diameter, depth}
feature.edit {id, params}  feature.suppress {id}  timeline.rollback {index}
primitive.add {kind, position, dimensions, operation}   // genera feature parametriche
body.place {body, from_face, to_face, offset}           // appoggia faccia su faccia
face.pushPull {face, distance}                          // scrive nella feature esistente se possibile, altrimenti crea un'estrusione
selection.set {refs}  selection.cycle {screen_point}
export.stl {body?, path, tolerance}  export.3mf {...}  export.step {...}
```

Comandi puramente di vista (zoom, orbita, vista predefinita, tema, pannelli) appartengono alla shell UI, non al motore.

---

## 6. Strumenti e overlay

```cpp
struct PointerEvent { Vec2 screen; Ray world_ray; Buttons buttons; Modifiers mods; Kind kind; };

class Tool {
public:
    virtual std::string_view id() const = 0;
    virtual ToolResponse on_pointer(const PointerEvent&, ToolContext&) = 0;
    virtual ToolResponse on_key(const KeyEvent&, ToolContext&) = 0;   // include input numerico
    virtual std::vector<OverlayPrim> overlay(const ViewTransform&) const = 0;
};

// OverlayPrim: Line, Polyline, Arc, Handle, DimensionLabel (modificabile), Text, Glyph (vincolo), Highlight (faccia o spigolo)
```

Gli strumenti vivono in `engine/` e non sanno nulla di ImGui. La UI disegna gli overlay e inoltra input.

### Requisito chiave: quote mentre si disegna

Questo è il motivo per cui Dune 3D non va bene. Lo strumento rettangolo deve funzionare così:

1. Clic sul primo angolo.
2. Mentre si muove il mouse compaiono due campi quota (larghezza, altezza) vicino al rettangolo.
3. Si digita un numero: va nel campo attivo e blocca quella misura. Tab passa all'altro campo.
4. Invio conferma: il rettangolo viene creato con vincoli orizzontale/verticale automatici e le due quote come vincoli veri, già modificabili.
5. Esc annulla.

Stesso schema per linee (lunghezza, angolo), cerchi (diametro), archi. I vincoli automatici (coincidente, orizzontale, verticale, tangente) si applicano durante il disegno, con un simbolo visibile.

---

## 7. Viewport e selezione

- Rendering della mesh tassellata (shaded + spigoli visibili), griglia, assi, piani di riferimento, view cube.
- **Picking con ID buffer:** si renderizza in un framebuffer nascosto con ogni faccia e spigolo colorati con il proprio id. Leggere il pixel sotto il mouse dà l'entità esatta. Ogni triangolo della mesh conserva l'id della faccia OCCT da cui viene.
- Clic ripetuto nello stesso punto: scorre tra le entità sovrapposte (`selection.cycle`).
- Evidenziazione al passaggio del mouse, prima del clic.
- Navigazione: tasto centrale orbita, Shift+centrale pan, rotella zoom verso il cursore (come Fusion).

---

## 8. UI (`ui/`)

Layout ispirato a Fusion 360, ricreato con asset nostri:

- **Barra superiore:** schede contestuali (Solido, Schizzo) con gruppi di comandi a icone ed etichette, menu a tendina per gruppo.
- **Sinistra:** browser del documento (parametri, corpi, schizzi, piani) con visibilità a occhio.
- **Centro:** viewport, view cube in alto a destra, barra di navigazione in basso.
- **Destra (o flottante):** pannello del comando attivo con i parametri, generato dai `params_doc` dove possibile, con widget custom per i comandi complessi.
- **Basso:** timeline delle feature, con icone, marker di rollback trascinabile, doppio clic per modificare, menu contestuale (sopprimi, elimina, modifica), feature in errore in rosso.
- **Palette dei comandi** (S o Ctrl+K): ricerca su tutto il registro.

Regole:

- Colori, raggi, spaziature e font vengono solo dai token del tema (`ui/theme.hpp`). Nessun colore scritto a mano nei widget.
- Widget condivisi in `ui/widgets.cpp` (campo valore con unità, slider, dropdown, toggle, bottone primario e secondario, sezione a scheda).
- Tutto lo stato della UI in una struct serializzabile (`ui/state.hpp`).
- `docs/reference/` contiene screenshot di Fusion e Tinkercad con note su cosa replicare. Si replicano posizioni e comportamento, non icone o grafica.

---

## 9. Canale di controllo e ciclo screenshot (`automation/`)

### App in esecuzione

`sagomacad --control 7878 --control-token-file .private/control.token` apre un server TCP solo su loopback. Ogni connessione deve autenticarsi col token prima di qualsiasi altro metodo. Messaggi JSON a riga singola: `{"id", "method", "params"}`.

Metodi:

```text
auth {token}
engine.execute {command, params}      engine.commands
ui.inspect                            ui.set {tool?, panels?, view?, theme?, ...}
ui.pointer {events: [{kind: down|move|up, x, y}], modifiers?}    // coordinate schermo del viewport
ui.click {x, y, button?, count?}      ui.key {key, modifiers?}      ui.type {text}
ui.resize {width, height}             ui.screenshot {path?}
app.open {path}                       app.save {path?}              app.quit
```

I percorsi file passano da cartelle radice concesse all'avvio (`--automation-root`). Percorsi assoluti o con `..` vengono rifiutati.

### Headless

```sh
sagomacad-cli snapshot --size 1440x900 --scale 2 --out ui.png \
  --script '[["engine.execute",{"command":"sketch.create","params":{"plane":"XY"}}],
             ["ui.set",{"tool":"sketch.rect"}]]'
sagomacad-cli run progetto.sagomacad --cmd feature.extrude --params '{...}' --out risultato.sagomacad
sagomacad-cli export progetto.sagomacad --stl out.stl
```

Lo snapshot crea un contesto OpenGL nascosto, renderizza alcuni frame della UI completa in un framebuffer e salva il PNG. Nessuna finestra visibile, nessun furto di focus.

### Test

- **Test geometrici numerici** (prioritari per il kernel): volume, area, bounding box, numero di facce, controllati con `GProp` di OCCT. Più robusti degli screenshot.
- **Golden image** per la UI: script di comandi + PNG di riferimento approvato dall'utente, confronto con tolleranza. Uno per funzione importante.
- **Fuzz dei comandi**: ogni comando del registro viene lanciato con parametri vuoti, tipi sbagliati, valori fuori scala, NaN, id inesistenti. Deve restituire errore, mai crash.

---

## 10. Robustezza

- OCCT segnala i fallimenti con eccezioni (`Standard_Failure`) e con `IsDone() == false`. Il modulo `kernel/` cattura tutto al confine e restituisce `Result<T>`. Nessuna eccezione OCCT esce da `kernel/`.
- Una feature che fallisce durante la regen viene marcata in errore e la valutazione prosegue. Il documento resta sempre apribile e salvabile.
- L'app ha una rete di sicurezza attorno all'esecuzione dei comandi e all'import/export: un errore imprevisto diventa un messaggio, non una chiusura.
- Salvataggio automatico di recupero ogni pochi minuti in una cartella dedicata.
- I numeri che arrivano da file o parametri sono ostili: controllo dei limiti, niente divisioni per zero, niente allocazioni enormi.

---

## 11. Traguardi

Ogni traguardo si chiude solo con build verde, test verdi, controllo livelli verde e gli screenshot o i test numerici indicati.

**M0 - Scheletro**
CMake + vcpkg, app che apre una finestra SDL3 con ImGui docking, viewport vuoto con griglia e assi, tema con token, `sagomacad-cli snapshot` funzionante, canale di controllo con `auth`, `ui.inspect`, `ui.screenshot`. `tools/check_layers.py`. CI su Windows, macOS e Linux.
*Fatto quando:* lo snapshot dell'app vuota esiste ed è approvato come prima golden image.

**M1 - Documento e comandi**
`doc/` con serializzazione `.sagomacad`, registro comandi, undo/redo, parametri con espressioni, `sagomacad-cli run`, palette dei comandi nella UI, `document.*`, `param.*`, `edit.*`. Test di fuzz dei comandi attivo.
*Fatto quando:* salva, riapri, undo e redo funzionano da CLI e da UI con gli stessi comandi.

**M2 - Kernel e vista 3D**
`kernel/` con OCCT, `primitive.add` (box, cilindro), tassellazione, vista shaded con spigoli, orbita/pan/zoom, view cube, `export.stl`.
*Fatto quando:* un box 40x40x20 esportato in STL ha volume corretto entro la tolleranza e si apre in Bambu Studio.

**M3 - Sketcher 2D**
Piani di schizzo, linea, rettangolo, cerchio, arco, vincoli automatici e manuali, quote, solver, colori per gradi di libertà (blu libero, nero vincolato), **quote digitate durante il disegno** come descritto al punto 6.
*Fatto quando:* un rettangolo 40x20 si disegna digitando "40 Tab 20 Invio" e risulta completamente vincolato.

**M4 - Feature e timeline**
Estrusione e taglio da profili, rivoluzione, timeline con modifica parametri e rollback, regen incrementale, feature in errore senza crash.
*Fatto quando:* cambiare la quota dello schizzo a monte aggiorna il solido e i test di volume restano corretti.

**M5 - Selezione e feature su facce**
Picking con ID buffer, hover, clic ripetuto, schizzi su faccia, raccordi e smussi su spigoli selezionati, riferimenti topologici del punto 4.
*Fatto quando:* un raccordo su uno spigolo sopravvive alla modifica della quota a monte (test dedicato).

**M6 - Immediatezza Tinkercad**
Primitive trascinate dal pannello sul piano o su una faccia, fori drag and drop, push and pull sulle facce che scrive nei parametri, `body.place` faccia su faccia con snap al centro e ai bordi, allineamenti rapidi.
*Fatto quando:* una scatola con foro centrale si crea in meno di 10 secondi senza aprire uno schizzo, e nella timeline compaiono feature normali e modificabili.

**M7 - Export e stampa**
3MF e STEP, tolleranza di tassellazione impostabile, export di un corpo o di tutto, apertura diretta in Bambu Studio se installato.
*Fatto quando:* STL, 3MF e STEP dello stesso pezzo si aprono correttamente in Bambu Studio e in un altro CAD.

**M8 - Rifinitura**
Menu a marcatura (tasto destro), scorciatoie personalizzabili, tema chiaro e scuro, salvataggio di recupero, prestazioni su modelli con centinaia di feature.

**Dopo (fuori dal primo piano):** feature SDF in fondo alla timeline (riempimenti gyroid e lattice, svuotamento) che convertono il solido esatto in campo di distanza e producono solo mesh per la stampa.

---

## 12. Primo prompt per Codex

> Leggi `AGENTS.md` e `PLAN.md`. Implementa il traguardo M0 del piano: struttura delle cartelle e dei target CMake per tutti i livelli (anche vuoti), vcpkg manifest, app `sagomacad` con finestra SDL3, Dear ImGui docking e viewport OpenGL con griglia e assi, tema con token in `ui/theme.hpp`, `sagomacad-cli snapshot` che renderizza offscreen e salva un PNG, canale di controllo con `auth`, `ui.inspect` e `ui.screenshot`, script `tools/check_layers.py`, workflow CI per Windows, macOS e Linux. Alla fine genera lo snapshot dell'app vuota in `docs/screenshots/m0.png`, guardalo, correggi quello che non va, e aggiorna `log/devlog.md`. Non iniziare M1.
