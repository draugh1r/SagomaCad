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

L’obiettivo è replicare 1:1 layout, flusso di lavoro, scorciatoie e comportamento di Fusion 360, come fa PhotoCraft con Photoshop, con asset e icone nostri. Le eccezioni sono le aggiunte in stile Tinkercad della sezione 6 e del traguardo M6.

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
- `docs/agents.md`: guida breve per agenti da terminale, con esempi di chiamate per i flussi tipici (schizzo, estrusione, misura, screenshot). È il file da indicare a Codex e Claude Code.

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
- Test bridge: app avviata in CI con `--control`, MCP collegato, `render_view` e `ui_screenshot` restituiscono PNG validi.
- Codex usa l'MCP in headless durante lo sviluppo per verificare le feature che implementa.

---

## 12. Traguardi

Ogni traguardo si chiude solo con build verde, test verdi, controllo livelli verde e gli screenshot o i test numerici indicati.

**M0 - Scheletro**
CMake + vcpkg, app che apre una finestra SDL3 con ImGui docking, viewport vuoto con griglia e assi, tema con token, `sagomacad-cli snapshot` funzionante, canale di controllo con `auth`, `ui.inspect`, `ui.screenshot`. `tools/check_layers.py`. CI su Windows, macOS e Linux.
*Fatto quando:* lo snapshot dell'app vuota esiste ed è approvato come prima golden image.

**M1 - Documento e comandi**
`doc/` con serializzazione `.sagomacad`, registro comandi, undo/redo, parametri con espressioni, `sagomacad-cli run`, palette dei comandi nella UI, `document.*`, `param.*`, `edit.*`. Client `sagomacad-cli call <metodo> [params]` per il canale di controllo, scoperta di porta e token dal file di sessione, opzioni `--port`, `--token-file` e `--headless <file.sagomacad>`, guida `docs/agents.md`. `Body` prevede `kind: brep | mesh` e la timeline distingue feature `exact | sdf`, anche se in M1 esistono solo quelle esatte; il formato `.sagomacad` accoglie i nuovi tipi senza rompere i file precedenti. Gli id `sdf.lattice`, `sdf.shell`, `sdf.smoothUnion`, `sdf.texture`, `sdf.offset` e `mesh.import` sono riservati, senza implementazione. Test di fuzz dei comandi attivo.
*Fatto quando:* salva, riapri, undo e redo funzionano da CLI e da UI con gli stessi comandi; in CI il client chiama `engine.execute` e `ui.screenshot` sull'app e verifica risposta JSON e PNG.

**M1.5 - MCP base**
`sagomacad-cli mcp` su stdio con JSON-RPC 2.0 e modalità headless; `commands_list`, `command_run`, `batch_run` come unico passo di undo, `undo`, `redo`, `document_inspect`, `document_open`, `document_save` e risorse MCP. Documentazione in `docs/mcp.md` con la configurazione per Claude Desktop, Claude Code e Codex.
*Fatto quando:* passano i test di conformità `initialize`, `tools/list`, `tools/call`, `resources/list`, `resources/read` e di errore JSON-RPC; CLI e MCP leggono ed eseguono gli stessi comandi.

**M2 - Kernel e vista 3D**
`kernel/` con OCCT, `primitive.add` (box, cilindro), tassellazione, vista shaded con spigoli, orbita/pan/zoom, view cube, `export.stl`. L'export chiede al motore quale stadio del corpo esportare anziché prendere sempre l'ultimo risultato; in M2 gli stadi esatto e finale coincidono. MCP: `render_view`, `mass_properties`, `export` (inizialmente STL) e modalità bridge verso l'app in esecuzione, inclusi `ui_screenshot` e test del collegamento.
*Fatto quando:* un box 40x40x20 esportato in STL ha volume corretto entro la tolleranza e si apre in Bambu Studio.

**M3 - Sketcher 2D**
Piani di schizzo, linea, rettangolo, cerchio, arco, vincoli automatici e manuali, quote, solver, colori per gradi di libertà (blu libero, nero vincolato), **quote digitate durante il disegno** come descritto al punto 6. MCP: `sketch_inspect`.
*Fatto quando:* un rettangolo 40x20 si disegna digitando "40 Tab 20 Invio" e risulta completamente vincolato.

**M4 - Feature e timeline**
Estrusione e taglio da profili, rivoluzione, timeline con modifica parametri e rollback, regen incrementale, feature in errore senza crash. MCP: `feature_get`.
*Fatto quando:* cambiare la quota dello schizzo a monte aggiorna il solido e i test di volume restano corretti.

**M5 - Selezione e feature su facce**
Picking con ID buffer, hover, clic ripetuto, schizzi su faccia, raccordi e smussi su spigoli selezionati, riferimenti topologici del punto 4. MCP: `geometry_query`, `measure`, `selection_get` e `render_view` con etichette dei riferimenti stabili. Test di integrazione headless: scatola 40×40×20 con foro passante da 10 mm creata usando solo tool MCP, volume verificato tramite `mass_properties`.
*Fatto quando:* un raccordo su uno spigolo sopravvive alla modifica della quota a monte (test dedicato).

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

> Leggi `AGENTS.md` e `PLAN.md`. Implementa il traguardo M0 del piano: struttura delle cartelle e dei target CMake per tutti i livelli (anche vuoti), vcpkg manifest, app `sagomacad` con finestra SDL3, Dear ImGui docking e viewport OpenGL con griglia e assi, tema con token in `ui/theme.hpp`, `sagomacad-cli snapshot` che renderizza offscreen e salva un PNG, canale di controllo con `auth`, `ui.inspect` e `ui.screenshot`, script `tools/check_layers.py`, workflow CI per Windows, macOS e Linux. Alla fine genera lo snapshot dell'app vuota in `docs/screenshots/m0.png`, guardalo, correggi quello che non va, e aggiorna `log/devlog.md`. Non iniziare M1.
