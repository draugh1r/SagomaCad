# Piano di sviluppo - SagomaCad

Documento da dare a Codex come guida del progetto. Va letto insieme a `RULES.md` (stessa cartella), che contiene le regole del codice.
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
4. **Livelli imposti in build.** Un modulo può dipendere solo da moduli di livello inferiore. Sotto la UI nessuno include ImGui, SDL, OpenGL o Clay. Un controllo automatico lo verifica.
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
| Interfaccia | Dear ImGui per i widget, Clay 0.14 per il layout | Shell dichiarativa con rettangoli stabili, widget e comandi esistenti |
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
  L6  ui/          layout Clay, widget Dear ImGui (shell sottile): pannelli, toolbar, viewport, overlay
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
- `tools/check_layers.py` analizza gli `#include` e fallisce se un livello include un livello superiore, se un modulo fuori da `kernel/` include header OCCT, o se un modulo sotto `ui/` include ImGui, SDL, OpenGL o Clay.
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
param.set {name, expression}  param.list  param.delete {name}
sketch.create {plane: "XY"|"XZ"|"YZ"}  sketch.finish {sketch?}  sketch.edit {sketch}
sketch.line {sketch?,x1,y1,x2,y2,length?,angle?,anchor?}
sketch.rect {sketch?,x?,y?,width,height,centered?,anchor?}  sketch.circle {sketch?,x?,y?,diameter,anchor?}
sketch.arc {sketch?,x?,y?,radius,start_angle,end_angle,anchor?}
sketch.constraint.add {sketch?,kind,refs}   // coincident, horizontal, vertical, parallel, perpendicular, tangent, equal, midpoint, fix
sketch.dimension.add {sketch?,kind,refs,expression}   // distance, length, diameter, radius, angle
sketch.dimension.set {sketch?,dimension,expression}
sketch.inspect {sketch?}  sketch.point.move {sketch?,point,x,y}
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

ID riservati per le voci della barra Fusion, senza implementazione nel traguardo M0: `document.saveAs`, `construction.plane`, `construction.axis`, `construction.point`, `inspect.measure`, `inspect.section`, `import.step`. Il menu «Salva con nome» richiede sempre un percorso nuovo; `document.save` continua a salvare il documento corrente.

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
- Navigazione: tasto centrale premuto pan, Shift+centrale orbita vincolata all'asse Z del mondo senza roll, rotella zoom verso il cursore (come Fusion). L'inclinazione laterale cambia solo con le rotazioni esplicite del cubo.
- View cube: Home ripristina l'isometrica e inquadra il modello; clic sulle 6 facce, 12 spigoli o 8 vertici allinea la camera. Il cubo si ruota trascinandolo, evidenzia la zona sotto il mouse e mostra le etichette delle facce e gli assi X/Y/Z. Nelle viste di faccia compaiono frecce per le viste adiacenti e per ruotare di 90°.

---

## 8. UI (`ui/`)

Il flusso di lavoro e il comportamento seguono le convenzioni dei CAD parametrici, con Fusion 360 come riferimento principale per cosa succede e in che ordine. L'aspetto visivo è nostro: colori, icone, proporzioni, forme dei pannelli e testi seguono `docs/design/` e i token del tema, non Fusion. Le aggiunte in stile Tinkercad della sezione 6 e del traguardo M6 restano.

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
- Due tipi di riferimento: `docs/reference/` (privato, fuori da git) contiene screenshot di Fusion e Tinkercad con note e serve solo per il **comportamento** (campi, ordine dei passi, cosa succede a passaggio del mouse, clic, digitazione). `docs/design/` (nel repository) contiene token, icone e schermate di SagomaCad approvate dall'utente e serve per l'**aspetto**. Le schermate approvate stanno in `docs/design/approvate/`.
- Icone: master SVG di SolidCraft (MIT OR Apache-2.0) con colori per ruoli semantici, citati in `ATTRIBUTION.md`; Lucide resta per le icone di navigazione che SolidCraft non copre.

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

## 11. Accesso per agenti AI

Qualsiasi agente AI compatibile MCP può collegarsi a SagomaCad, leggere il modello, modellare, misurare e vedere il risultato mentre l’utente segue il lavoro nell’app.

### Principio

L'MCP non è un modulo a parte con una sua logica: è un altro client del registro comandi, come la UI, la CLI e il canale di controllo. Se un comando esiste, l'agente può usarlo. Nessuna funzione deve esistere solo nella UI.

Il valore rispetto agli MCP esistenti per altri CAD (che di solito passano da add-in o API parziali) viene da tre cose:

1. **Accesso completo**: tutto ciò che fa l'utente passa dai comandi, quindi l'agente può fare tutto.
2. **L'agente vede**: rendering del modello da qualsiasi vista, con facce e spigoli etichettati.
3. **Riferimenti stabili**: facce e spigoli hanno nomi stabili (punto 4 di PLAN.md), quindi l'agente può dire "raccorda `ext1.side(e3)` con raggio 2" e il riferimento sopravvive alle modifiche.


### Architettura

Tre livelli, uno sopra l'altro. Il motore è uno solo, le porte d'ingresso sono due.

```text
app con terminale (Codex, Claude Code, script)  --> sagomacad-cli call ...   --+
                                                                               +--> canale di controllo (socket TCP) --> app SagomaCad
app senza terminale (Claude Desktop, Cursor...)  --> MCP --> sagomacad-cli mcp --+
                                                             (oppure headless: Session in memoria, senza finestra)
```

#### Il canale di controllo (socket) è la base

Esiste già da M0 (`auth`, `ui.inspect`, `ui.screenshot`, in seguito `engine.execute` e il resto). Tutto passa da lì. Né il client CLI né l'MCP contengono logica propria: traducono e inoltrano.

#### Client CLI per agenti da terminale

Gli agenti che sanno usare la shell non hanno bisogno di MCP: basta un comando ben documentato. Costa poco e non occupa contesto con le descrizioni degli strumenti.

```sh
sagomacad-cli call engine.commands
sagomacad-cli call engine.execute '{"command":"feature.extrude","params":{"sketch":"sk1","distance":"20 mm"}}'
sagomacad-cli call ui.screenshot '{"path":"shot.png"}'
sagomacad-cli call batch '{"steps":[...]}'
```

- Legge porta e token da `--port` / `--token-file` o da un file di sessione scritto dall'app all'avvio (`.sagomacad/session.json`, solo per l'utente corrente), così l'agente non deve conoscerli.
- Stampa la risposta JSON su stdout, gli errori su stderr, codice di uscita diverso da zero in caso di errore.
- Se non c'è un'app aperta e si passa `--headless <file.sagomacad>`, esegue la chiamata su una `Session` in memoria.
- `docs/agent-cli.md`: guida breve per agenti da terminale, con esempi di chiamate per i flussi tipici (schizzo, estrusione, misura, screenshot). È il file da indicare a Codex e Claude Code.

#### Server MCP per tutte le altre app

- `sagomacad-cli mcp` è un server MCP su **stdio** (il trasporto standard che tutti i client supportano).
- **Modalità bridge** (`--bridge 7878 --token-file ...`): si collega all'app in esecuzione. L'utente vede in tempo reale ogni operazione dell'agente nel viewport e nella timeline. È la modalità principale.
- **Modalità headless** (default se l'app non è aperta): crea una `Session` propria. Serve per automazioni, test e per Codex durante lo sviluppo.
- Il protocollo MCP è JSON-RPC 2.0: si implementa direttamente con nlohmann/json, senza dipendenze nuove. Modulo `automation/mcp/` (livello L5).
- Più avanti, opzionale: trasporto HTTP streamable su loopback, per agenti che non lanciano processi.


### Tool esposti

Pochi tool generici e ben documentati funzionano meglio di centinaia di tool, uno per comando. Il catalogo completo dei comandi si scopre con `commands_list`.

#### Scoperta ed esecuzione

| Tool | Cosa fa |
|---|---|
| `commands_list {filter?}` | Elenco comandi: id, etichetta, documentazione parametri, abilitato o no e perché |
| `command_run {command, params}` | Esegue un comando, restituisce risultato o errore leggibile |
| `batch_run {steps, stop_on_error}` | Esegue più comandi come **un solo passo di undo** |
| `undo` / `redo` | Annulla o ripete |

#### Lettura del modello

| Tool | Cosa fa |
|---|---|
| `document_inspect` | Parametri, timeline con stato ed errori di ogni feature, corpi, unità |
| `feature_get {id}` | Parametri completi di una feature |
| `sketch_inspect {sketch}` | Entità, vincoli, quote, gradi di libertà residui, stato del solver |
| `geometry_query {body?, kind, filter?}` | Facce, spigoli o vertici con riferimento stabile e descrizione geometrica (tipo, normale, raggio, baricentro, area, lunghezza). Filtri: piane, cilindriche, parallele a un asse, sopra una quota, eccetera |
| `selection_get` | Cosa ha selezionato l'utente nell'app (solo bridge): "raccorda gli spigoli che ho selezionato" |

#### Misure

| Tool | Cosa fa |
|---|---|
| `mass_properties {body}` | Volume, area, bounding box, baricentro |
| `measure {a, b}` | Distanza minima e angolo tra due entità |
| `check_printability {body}` | Solido chiuso o no, pareti sotto uno spessore minimo, sbalzi oltre un angolo (dopo M7) |

#### Vista

| Tool | Cosa fa |
|---|---|
| `render_view {view, size?, highlight?, labels?}` | PNG del modello da `iso`, `top`, `front`, `right` o da una camera libera. `labels: true` disegna i nomi stabili sulle facce e sugli spigoli, così l'agente può riferirsi a quello che vede |
| `ui_screenshot` | Screenshot dell'intera app (solo bridge) |

#### File

| Tool | Cosa fa |
|---|---|
| `document_open {path}` / `document_save {path?}` | Solo dentro le cartelle radice concesse |
| `export {format: stl|3mf|step, body?, path}` | Export, stesse regole sui percorsi |

#### Risorse MCP

- `sagomacad://document` - il documento corrente in JSON
- `sagomacad://commands` - il catalogo comandi
- `sagomacad://docs/modeling-guide` - una guida breve per agenti: come si crea uno schizzo, come si estrude, come si usano i riferimenti stabili, errori comuni


### Esperienza utente nell'app

- **Pannello Agente**: mostra se un agente è collegato e il registro delle sue azioni (comando, parametri, esito).
- Ogni batch di un agente è un passo della timeline e dell'undo, marcato con un'icona "agente". L'utente annulla con Ctrl+Z come per qualsiasi altra operazione.
- Interruttore "Consenti agenti" nella barra in alto. Se è spento, la modalità bridge rifiuta le connessioni.
- Mentre l'agente lavora, il viewport mostra in evidenza le entità che ha appena creato o modificato.


### Sicurezza

- Solo loopback, token obbligatorio in modalità bridge.
- Lettura e scrittura file solo nelle cartelle radice concesse all'avvio. Percorsi assoluti e `..` rifiutati.
- Sovrascrivere un file esistente richiede `overwrite: true` esplicito.
- Nessun tool esegue codice arbitrario o comandi di sistema.
- Limiti su dimensione delle richieste, numero di passi di un batch e dimensione delle immagini.
- Il fuzz dei comandi copre anche il percorso MCP: input malformato produce errore JSON-RPC, mai crash.


### Errori pensati per agenti

Ogni errore restituisce:

- un messaggio chiaro (`"Lo schizzo sk2 non ha profili chiusi"`);
- un codice stabile (`sketch.no_closed_profile`);
- quando possibile, un suggerimento (`"Lo spigolo e4 non tocca e1: aggiungi un vincolo coincidente tra i punti p3 e p5"`).

Gli agenti si correggono molto meglio con errori precisi che con un generico "operazione fallita".


### Test

- Test di integrazione MCP: uno script avvia `sagomacad-cli mcp` in headless, crea una scatola 40x40x20 con un foro passante da 10 mm usando solo tool MCP, e verifica con `mass_properties` che il volume sia corretto entro la tolleranza.
- Test di conformità: handshake `initialize`, `tools/list`, `tools/call`, `resources/list`, `resources/read`, gestione degli errori JSON-RPC.
- Test bridge locale su Windows: app avviata con `--control`, MCP collegato, `render_view` e `ui_screenshot` restituiscono PNG validi.
- Codex usa l'MCP in headless durante lo sviluppo per verificare le feature che implementa.

---

## 12. Traguardi

Ogni traguardo si chiude solo con build Release locale su Windows verde, ctest e controllo livelli verdi, fuzz dei comandi se toccati e screenshot se si tocca la UI, oltre ai test numerici indicati. La CI multipiattaforma è sospesa per scelta dell'utente: nessun avvio, attesa o lettura dei risultati remoti e nessuna verifica su macOS o Linux finché non richiesta. Il codice resta portabile. Il workflow è conservato con il solo trigger `workflow_dispatch` per l'avvio manuale da GitHub da parte dell'utente.

**M0 - Scheletro**
CMake + vcpkg, app che apre una finestra SDL3 con ImGui docking, viewport vuoto con griglia e assi, tema con token, `sagomacad-cli snapshot` funzionante, canale di controllo con `auth`, `ui.inspect`, `ui.screenshot`. `tools/check_layers.py`. Verifica locale su Windows.
*Fatto quando:* lo snapshot dell'app vuota esiste ed è approvato come prima golden image.

**M1 - Documento e comandi**
`doc/` con serializzazione `.sagomacad`, registro comandi, undo/redo, parametri con espressioni, `sagomacad-cli run`, palette dei comandi nella UI, `document.*`, `param.*`, `edit.*`. Client `sagomacad-cli call <metodo> [params]` per il canale di controllo, scoperta di porta e token dal file di sessione, opzioni `--port`, `--token-file` e `--headless <file.sagomacad>`, guida `docs/agent-cli.md`. `Body` prevede `kind: brep | mesh` e la timeline distingue feature `exact | sdf`, anche se in M1 esistono solo quelle esatte; il formato `.sagomacad` accoglie i nuovi tipi senza rompere i file precedenti. Gli id `sdf.lattice`, `sdf.shell`, `sdf.smoothUnion`, `sdf.texture`, `sdf.offset` e `mesh.import` sono riservati, senza implementazione. Test di fuzz dei comandi attivo.
*Fatto quando:* salva, riapri, undo e redo funzionano da CLI e da UI con gli stessi comandi; nei test locali su Windows il client chiama `engine.execute` e `ui.screenshot` sull'app e verifica risposta JSON e PNG.

**M1.5 - MCP base**
`sagomacad-cli mcp` su stdio con JSON-RPC 2.0 e modalità headless; `commands_list`, `command_run`, `batch_run` come unico passo di undo, `undo`, `redo`, `document_inspect`, `document_open`, `document_save` e risorse MCP. Documentazione in `docs/mcp.md` con la configurazione per Claude Desktop, Claude Code e Codex.
*Fatto quando:* passano i test di conformità `initialize`, `tools/list`, `tools/call`, `resources/list`, `resources/read` e di errore JSON-RPC; CLI e MCP leggono ed eseguono gli stessi comandi.

**M2 - Kernel e vista 3D**
`kernel/` con OCCT, `primitive.add` (box, cilindro), tassellazione, vista shaded con spigoli, orbita/pan/zoom, view cube, `export.stl`. L'export chiede al motore quale stadio del corpo esportare anziché prendere sempre l'ultimo risultato; in M2 gli stadi esatto e finale coincidono. MCP: `render_view`, `mass_properties`, `export` (inizialmente STL) e modalità bridge verso l'app in esecuzione, inclusi `ui_screenshot` e test del collegamento.
*Fatto quando:* un box 40x40x20 esportato in STL ha volume corretto entro la tolleranza e si apre in Bambu Studio.

**LAYOUT - Shell dichiarativa con Clay - completato (16 ottobre 2026)**
Clay 0.14 da vcpkg; albero shell e ribbon in `ui/layout.cpp`, arena limitata, misura testo ImGui, rettangoli `ShellLayout`, renderer decorativo ImDrawList. Dockspace rimosso. Separatori trascinabili con reset al doppio clic, pannelli comprimibili e misure persistenti nello stato UI. `ui.inspect` espone rettangoli, scala, stato e diagnostica. Test di layout, input, riavvio e confronto prima/dopo nei due temi e dimensioni, senza aggiornare golden image. La migrazione conserva M3 già presente nel branch, senza aggiungere funzioni CAD.

**M3 - Sketcher 2D**
Piani di schizzo, linea, rettangolo, cerchio, arco, vincoli automatici e manuali, quote, solver, colori per gradi di libertà (blu libero, nero vincolato), **quote digitate durante il disegno** come descritto al punto 6. MCP: `sketch_inspect`.
*Fatto quando:* un rettangolo 40x20 si disegna digitando "40 Tab 20 Invio" e risulta completamente vincolato.

Protocollo ed esempi: `docs/sketches.md`. Include solver Eigen con diagnostica di conflitti/ridondanze, piani dell'origine, quote parametriche, profili chiusi, trascinamento con coalescenza, modifica dalla timeline e parità CLI/MCP (`sketch_inspect`). Schizzi su faccia in M5, feature da profili in M4.

**M4 - Feature e timeline - completato il 17 ottobre 2026**
Estrusione e taglio da profili, rivoluzione, timeline con modifica parametri e rollback, regen incrementale, feature in errore senza crash. MCP: `feature_get`.
*Fatto quando:* cambiare la quota dello schizzo a monte aggiorna il solido e i test di volume restano corretti.

Completate le sei tappe: regioni annidate con riferimenti alle entità, estrusione
e rivoluzione con operazioni e destinazioni, cache incrementale del prefisso,
rollback/soppressione/errori isolati, dialoghi e maniglie, CLI/MCP e `feature_get`.
Accettazione numerica e istruzioni: `docs/modeling-guide.md`; staffa a L con due
fori creata via MCP e misurata in `docs/examples/m4/`. Confronti 07–10 nei due
temi e due dimensioni in `docs/screenshots/m4-confronto/`. Build Windows Release,
41 test e controlli di livelli/UTF-8 verdi. **M5 completato: vedere il traguardo seguente.**

Audit successivo di M4: `docs/sketch-extrude-audit.md`. Corretti recupero e riuso
degli schizzi nascosti, selezioni obsolete, disambiguazione fra schizzi e
inserimento al marker. Rimane aperta la suddivisione di contorni intersecanti
all'interno dello stesso schizzo (riproduzione con due cerchi sovrapposti).
È un limite del riconoscitore di profili, distinto dal picking facce di M5.

**M5 - Selezione e feature su facce - completato il 17 ottobre 2026**

Picking con ID buffer, hover, clic ripetuto, schizzi su faccia, raccordi e smussi su spigoli selezionati, riferimenti topologici del punto 4. MCP: `geometry_query`, `measure`, `selection_get` e `render_view` con etichette dei riferimenti stabili. Test di integrazione headless: scatola 40×40×20 con foro passante da 10 mm creata usando solo tool MCP, volume verificato tramite `mass_properties`.
*Fatto quando:* un raccordo su uno spigolo sopravvive alla modifica della quota a monte (test dedicato).

Completate le tappe con commit e verifica locale Windows. Picking ID/profondità,
nomi stabili e storia OCCT, schizzi su faccia con proiezioni, estrusione da faccia,
raccordo/smusso multipli, foro quotato, tool agenti e scorciatoie E/F/H.
I test topologici delle catene reali e della faccia eliminata precedono la UI.
Release verde, CTest 51/51, livelli e fuzz verdi. Confronti 11–16 in 32 coppie
nei due temi e due dimensioni. Rapporto: `docs/m5-validation.md`; guida
eseguibile: `docs/modeling-guide.md`; esempio MCP: `docs/examples/m5/`.
M5.5 e M6 restano da iniziare.

**M5.5 - Identità visiva e infrastruttura UI**
Due tipi di riferimento (comportamento da `docs/reference/`, aspetto da `docs/design/`), icone SVG di SolidCraft con ricolorazione per ruoli, token del tema ampliati agli stati del viewport, regola "una sola fabbrica di finestre". Viewport su GPU (OpenGL con depth buffer, linee spesse in screen space, spigoli sempre sopra le facce, sagome dei corpi curvi, sfondo a gradiente, MSAA) usato anche da snapshot e `render_view`. Handle con un nome per ogni elemento disegnato e scenari JSON che guidano l'app senza finestra. Correzione dei profili con contorni intersecanti rimasta aperta da M4.
*Fatto quando:* la tavola di palette e icone nei due temi è approvata dall'utente e salvata in `docs/design/approvate/`; il viewport GPU disegna un pezzo M5 con spigoli netti e sagome a qualsiasi angolo; almeno 10 scenari JSON passano in ctest; due cerchi sovrapposti producono tre regioni selezionabili.

**M6 - Immediatezza Tinkercad**
Primitive trascinate dal pannello sul piano o su una faccia, fori drag and drop, push and pull sulle facce che scrive nei parametri, `body.place` faccia su faccia con snap al centro e ai bordi, allineamenti rapidi.
*Fatto quando:* una scatola con foro centrale si crea in meno di 10 secondi senza aprire uno schizzo, e nella timeline compaiono feature normali e modificabili.

**M7 - Export e stampa**
3MF e STEP, tolleranza di tassellazione impostabile, export di un corpo o di tutto, apertura diretta in Bambu Studio se installato. L'export continua a chiedere al motore lo stadio del corpo: STEP usa lo stadio esatto, STL e 3MF il risultato finale; prima delle feature SDF i due risultati coincidono. MCP: `export` anche per 3MF e STEP, `check_printability`.
*Fatto quando:* STL, 3MF e STEP dello stesso pezzo si aprono correttamente in Bambu Studio e in un altro CAD.

**M8 - Rifinitura**
Menu a marcatura (tasto destro), scorciatoie personalizzabili, tema chiaro e scuro, salvataggio di recupero, prestazioni su modelli con centinaia di feature. Pannello Agente con registro delle azioni, interruttore "Consenti agenti" ed evidenziazione delle modifiche nel viewport.

---

## 13. Feature SDF

Le feature SDF per la stampa 3D arrivano dopo M8. In M1, M2 e M7 si preparano solo i contratti del modello e dell'export descritti sopra; non si implementano ora operazioni SDF.

### A cosa servono

La geometria esatta (B-rep, OpenCascade) serve a progettare. Le feature SDF servono a preparare il pezzo per la stampa con operazioni che il kernel esatto fa male o non fa:

- reticoli e gyroid dentro il pezzo (pezzi leggeri, imbottiture, impugnature);
- svuotamento con pareti uniformi su forme complesse;
- lavoro su mesh importate (STL di scansioni e miniature): taglio, base, svuotamento, fori per perni, senza booleane che falliscono;
- unioni morbide tra forme;
- texture sulle superfici (zigrinature, pattern);
- offset per tolleranze di incastro e stampe "print in place".

Il risultato è una mesh, non geometria esatta. Per la stampa FDM va bene: lo slicer vuole comunque triangoli, e la risoluzione di campionamento (per esempio 0,05 mm) è sotto la precisione della stampante.

### Regole di modello

1. **Due stadi per corpo.** La timeline di ogni corpo ha uno stadio esatto e, opzionalmente, uno stadio SDF che viene dopo. Una feature SDF non può essere seguita da feature esatte sullo stesso corpo: il comando rifiuta l'operazione con un errore chiaro.
2. **Niente si perde.** La feature SDF legge il solido esatto prodotto dalle feature precedenti. Sopprimerla, o spostare il marker della timeline prima di lei, riporta al solido esatto. Cambiare una quota a monte ricalcola anche la feature SDF.
3. **Due uscite per corpo.**
   - Export STEP: usa sempre il risultato dello stadio esatto.
   - Export STL e 3MF: usa il risultato finale, SDF compreso.
   Se un corpo ha feature SDF, il dialog di export STEP lo segnala ("esporto la geometria esatta, senza le feature di stampa").
4. **Corpi da mesh.** Un STL importato è un corpo di tipo mesh. Su di esso si possono usare solo feature SDF (e trasformazioni). Non entra mai nello stadio esatto.
5. **Parametri come tutte le altre feature.** Spessore delle pareti, dimensione delle celle, risoluzione di campionamento sono parametri ed espressioni, modificabili dalla timeline e dai comandi.

### Cosa deve prevedere già da ora

- **M1 (documento):** `Body` ha un campo `kind: brep | mesh` e la timeline distingue il tipo di ogni feature (`exact | sdf`), anche se oggi esistono solo quelle esatte. Il formato `.sagomacad` deve poter aggiungere questi tipi senza rompere i file vecchi.
- **M2 / M7 (export):** l'export chiede al motore "quale stadio" del corpo esportare, invece di prendere sempre l'ultimo risultato. Oggi le due risposte coincidono.
- **Comandi e MCP:** gli id previsti sono `sdf.lattice`, `sdf.shell`, `sdf.smoothUnion`, `sdf.texture`, `sdf.offset`, `mesh.import`. Non vanno implementati ora, ma i nomi sono riservati.

### Implementazione (quando arriverà)

- Modulo `sdf/` al livello L3, accanto a `regen/` e `mesh/`. Non dipende da OCCT: riceve una mesh densa del solido esatto da `mesh/`.
- Conversione mesh -> campo di distanza su griglia sparsa (solo vicino alla superficie), operazioni sul campo, estrazione della superficie con dual contouring o marching cubes, semplificazione adattiva della mesh.
- Anteprima nel viewport a bassa risoluzione mentre si modificano i parametri, risoluzione piena all'export.
- Test: volume e spessore minimo delle pareti misurati sulla mesh risultante, mesh chiusa e senza autointersezioni, tempi su un pezzo di riferimento.

---

## 14. Primo prompt per Codex

> Leggi `AGENTS.md` e `PLAN.md`. Implementa il traguardo M0 del piano: struttura delle cartelle e dei target CMake per tutti i livelli (anche vuoti), vcpkg manifest, app `sagomacad` con finestra SDL3, Dear ImGui docking e viewport OpenGL con griglia e assi, tema con token in `ui/theme.hpp`, `sagomacad-cli snapshot` che renderizza offscreen e salva un PNG, canale di controllo con `auth`, `ui.inspect` e `ui.screenshot`, script `tools/check_layers.py`, workflow GitHub Actions opzionale con il solo trigger `workflow_dispatch` per l'avvio manuale da parte dell'utente. La verifica è solo locale su Windows secondo `AGENTS.md`, mantenendo il codice portabile. Alla fine genera lo snapshot dell'app vuota in `docs/screenshots/m0.png`, guardalo, correggi quello che non va, e aggiorna `log/devlog.md`. Non iniziare M1.

---

## 15. Decisioni

- 2026-10-17: lo stato del lavoro passa al framework code-runner. Piano in `docs/.plan/PLAN.md`, regole in `docs/.plan/RULES.md`, stato solo in `docs/.plan/tasks.json`, fasi in `docs/.plan/prps/`. I traguardi M0-M5 (con M1.5 e LAYOUT) sono registrati come fasi 1-8 concluse; il devlog resta come storia.
- 2026-10-17: l'interfaccia non replica più Fusion 1:1. Comportamento e flusso seguono le convenzioni dei CAD parametrici (riferimento privato `docs/reference/`), l'aspetto è nostro (`docs/design/`). Motivo: evitare di copiare l'aspetto di un prodotto commerciale.
- 2026-10-17: nuovo traguardo M5.5 dall'analisi di SolidCraft (tangency-labs/solidcraft, MIT OR Apache-2.0): icone e palette per ruoli, token, viewport GPU, handle con nome, scenari UI. Il codice Rust si studia e si traduce, mai si copia senza attribuzione; gli SVG si riusano citandoli.
- 2026-10-17: ogni fase inizia con un task 0 bloccante che chiede all'utente gli screenshot di riferimento con istruzioni passo passo; senza i file la fase non procede.

- 2026-10-17: su richiesta dell'utente, per la fase 9 SolidCraft (`../solidcraft-ref`) sostituisce gli screenshot di ispirazione. Tema chiaro predefinito, accento verde pistacchio, densita compatta come Fusion.

- 2026-10-17: precisazione dell’utente sul tema chiaro: background bianco in tutta l’app; verde pistacchio esclusivamente come accento, senza dominante verde negli sfondi.

- 2026-10-18: su richiesta esplicita, fase correttiva 9.5 (M5) prima della 10: schizzo visibile per default con preferenza di occultamento; operazione estrusione geometrica automatica fino a scelta manuale; quote separate ancorate ai tratti e modifica in loco. Criteri e regressioni nel PRP phase-9.5-correzioni-m5.md.

- 2026-10-18: correzione delle giunzioni dopo booleana: ShapeUpgrade_UnifySameDomain su facce/spigoli in kernel, History nel naming e fallback al risultato grezzo se la pulizia fallisce. La L deve restare un corpo con otto facce e i riferimenti di raccordi/schizzi devono seguire le modifiche a monte.

- 2026-10-19: autorizzata la correzione della validazione 10.2: dimensions esplicite per il pezzo di prova; verifica di visibilità aggiornata alla nuova luce GPU, senza modificare golden approvate.
