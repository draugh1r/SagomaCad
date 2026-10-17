# Phase 10: Viewport GPU, handle con nome e scenari UI

> Phase 10 of 15 - Milestone M5.5 "Identità visiva e infrastruttura UI" of PLAN.md (seconda di due fasi)

## Overview
Oggi il viewport proietta i triangoli sulla CPU e li disegna con `ImDrawList` senza depth buffer (`apps/common/model_widget.cpp`, `geometry_widget.cpp`, `shell.cpp`). Questa fase porta il disegno su OpenGL con depth buffer, linee spesse vere, spigoli sempre visibili sopra le facce, sagome dei corpi curvi e sfondo a gradiente, con la stessa pipeline per finestra, snapshot e `render_view`. Poi aggiunge l'infrastruttura che permette agli agenti di lavorare sulla UI senza romperla: handle con un nome per ogni elemento disegnato e scenari JSON che guidano l'app senza finestra. Chiude anche il bug dei profili intersecanti rimasto aperto da M4.

## Plan References
- PLAN.md: "7. Viewport e selezione", "9. Canale di controllo e ciclo screenshot", "12. Traguardi" (M5.5)
- RULES.md: regole 2, 7, 9
- `docs/control-protocol.md`, `docs/sketch-extrude-audit.md` (bug dei due cerchi)

## Riferimenti SolidCraft
Clone fuori dal repository in `../solidcraft-ref` (vedi fase 9). Da studiare e tradurre in GLSL e C++:
- `assets/shaders/viewport.wgsl`: `vs_line` (linee come due triangoli in screen space, taglio sul piano vicino), `solid` (sagome con `fwidth` di N·V), `shade` (luce principale spostata dall'occhio, luce di riempimento frontale, riflesso leggero), tratteggio della sezione
- `crates/gpu/src/view.rs`: geometria caricata solo quando la scena cambia, ogni frame cambia solo la camera; spinta indietro delle facce in base alla pendenza (`FACE_BIAS`)
- `crates/ui-egui/src/gpu.rs`: il viewport disegnato dentro una callback del framework UI
- `crates/ui-egui/src/scenario.rs`: formato degli scenari, `publish_handle`, `publish_rect`, `publish_count`, forme dei punti (`plane`, `world`, `sketch`, `handle`, `browser`)
- `crates/ui-egui/tests/scenarios/*.json` (111 esempi) e `crates/ui-egui/build.rs` (un test per file)

## Phase Exit Criteria
(Copiato da M5.5) La tavola di palette e icone nei due temi è approvata dall'utente e salvata in `docs/design/approvate/`; il viewport GPU disegna un pezzo M5 con spigoli netti e sagome a qualsiasi angolo; almeno 10 scenari JSON passano in ctest; due cerchi sovrapposti producono tre regioni selezionabili.

## Features Covered
- Viewport su GPU con depth buffer e MSAA, condiviso da finestra, snapshot e `render_view`
- Linee spesse in screen space, spigoli sopra le facce, sagome, sfondo a gradiente
- Handle con un nome per ogni elemento disegnato, esposti da `ui.inspect` e usabili dal canale di controllo
- Scenari JSON headless, un test ctest per file
- Correzione dei profili con contorni intersecanti

## Dependencies
- Fase 9 completata (token e tavola approvata)

## Tasks

### Task 10.0: Screenshot di riferimento (bloccante)
**Action:** USER
**Files:**
- `docs/reference/fusion/17-stile-visivo.png`, `.md`
- `docs/reference/fusion/18-spigoli-e-sagome.png`, `.md`
- `docs/reference/fusion/19-spigoli-angolo-radente.png`, `.md`

**Details:**
Se i file mancano, **fermati** e scrivi all'utente:

> Per il nuovo viewport mi servono tre screenshot di Fusion, solo per capire il comportamento (cosa si vede e quando), non per copiarne l'aspetto. Salvali in `docs/reference/fusion/`.
> 1. Prepara un pezzo con un cilindro e qualche raccordo: per esempio una scatola con un cilindro sopra e i raccordi sugli spigoli verticali.
> 2. **17-stile-visivo.png**: nella barra in basso al centro clicca l'icona **Display Settings**, apri **Visual Style** e fai lo screenshot con il menu aperto (Shaded, Shaded with Hidden Edges, Shaded with Visible Edges Only, Wireframe...).
> 3. **18-spigoli-e-sagome.png**: scegli **Shaded with Visible Edges Only**, vista isometrica (Home sul cubo), zoom in modo che il pezzo riempia metà schermo.
> 4. **19-spigoli-angolo-radente.png**: ruota la vista fino a guardare la faccia superiore quasi di taglio, così si vede come restano visibili gli spigoli e il contorno del cilindro.
> Accanto a ogni immagine crea un file con lo stesso nome e `.md` con una riga: per esempio "il contorno del cilindro si vede anche dove non c'è uno spigolo".

**Validation:**
```bash
ls docs/reference/fusion/17-*.png docs/reference/fusion/18-*.png docs/reference/fusion/19-*.png
```

### Task 10.1: Modulo di rendering OpenGL
**Action:** CREATE
**Files:**
- `src/ui/render/gl_viewport.hpp`, `src/ui/render/gl_viewport.cpp`
- `src/ui/render/shaders/*.glsl` (incorporati in build)
- `CMakeLists.txt`, `tools/check_layers.py` (se serve la nuova cartella)

**Details:**
Programmi per triangoli, linee e immagini; VBO per corpo caricati solo quando cambia la revisione del corpo; ogni frame aggiorna solo l'uniform della camera. Framebuffer con depth e MSAA (4x se disponibile), risolto in una texture disegnata nel rettangolo del viewport tramite callback di `ImDrawList`. OpenGL 3.3 core (4.1 su macOS). Gli overlay ImGui (maniglie, caselle, quote) restano sopra.

**Validation:**
```bash
cmake --build build --config Release && ctest --test-dir build -R gl_viewport --output-on-failure
```

### Task 10.2: Facce e luce
**Action:** CREATE
**Files:**
- `src/ui/render/shaders/solid.frag`, `solid.vert`

**Details:**
Traduci `shade()`: luce principale `normalize(v*0.8 + (-0.3, 0.2, 0.6))`, diffusa + riempimento frontale + riflesso a esponente 40 con peso 0,25, colori dai token (`body`), uscita sRGB corretta. Normale nulla = colore piatto (riempimenti di selezione, piani trasparenti). Tratteggio a 45 gradi per le facce tagliate dal piano di sezione, pronto per una sezione futura.

**Validation:**
```bash
sagomacad-cli snapshot --size 1440x900 --scale 2 --out build/check-shading.png --script '[["engine.execute",{"command":"primitive.add","params":{"kind":"box"}}]]'
```

### Task 10.3: Linee spesse in screen space
**Action:** CREATE
**Files:**
- `src/ui/render/shaders/line.vert`, `line.frag`

**Details:**
Traduci `vs_line`: ogni segmento diventa due triangoli orientati verso lo schermo con lo spessore in pixel (token), taglio sul piano vicino per i segmenti che passano dietro l'occhio. Usato per spigoli, schizzi, assi e griglia. Spessori diversi per spigolo normale, selezionato, sotto il mouse (con alone).

**Validation:**
```bash
ctest --test-dir build -R gl_lines --output-on-failure
```

### Task 10.4: Spigoli sempre sopra le facce
**Action:** MODIFY
**Files:**
- `src/ui/render/gl_viewport.cpp`
- `docs/examples/m5/camera-radente.json` (nuovo script: pezzo M5 e camera quasi radente sulla faccia superiore)

**Details:**
Spinta indietro delle facce opache in base alla pendenza (`glPolygonOffset` con fattore di pendenza) così spigoli, schizzi su faccia e decorazioni disegnati alla loro profondità vincono a qualsiasi angolo, anche radente (confronta con `19-spigoli-angolo-radente`).

**Validation:**
```bash
sagomacad-cli snapshot --size 1440x900 --scale 2 --out build/check-grazing.png --script docs/examples/m5/camera-radente.json
```

### Task 10.5: Sagome dei corpi curvi
**Action:** MODIFY
**Files:**
- `src/ui/render/shaders/solid.frag`
- `docs/examples/m5/cilindro-raccordi.json` (nuovo script: scatola, cilindro sopra, raccordi verticali, vista isometrica)

**Details:**
Dove la superficie gira via dall'occhio (N·V vicino a zero, misurato con `fwidth`) disegna una linea scura di circa 1,5 px, solo sulla superficie visibile. Attiva negli stili con spigoli, spenta in wireframe.

**Validation:**
```bash
sagomacad-cli snapshot --size 1440x900 --scale 2 --out build/check-silhouette.png --script docs/examples/m5/cilindro-raccordi.json
```

### Task 10.6: Sfondo, griglia e piani dell'origine
**Action:** MODIFY
**Files:**
- `src/ui/render/gl_viewport.cpp`, `apps/common/shell.cpp`

**Details:**
Gradiente tra `viewport_top` e `viewport_bottom`, griglia con linee principali più marcate che sfumano in lontananza, piani dell'origine trasparenti con bordo e hover dai token. Rimuovi il vecchio disegno CPU dei triangoli; lascia un percorso CPU solo come ripiego se il contesto GL non si crea (errore leggibile, mai crash).

**Validation:**
```bash
ctest --test-dir build --output-on-failure
```

### Task 10.7: Picking e anteprime sulla nuova pipeline
**Action:** MODIFY
**Files:**
- `src/engine/picking.cpp`, `apps/common/geometry_widget.cpp`, `apps/common/model_widget.cpp`

**Details:**
Il buffer ID di M5 usa le stesse mesh caricate sulla GPU. Le anteprime (unione e nuovo corpo azzurre, taglio rosso trasparente) passano dal renderer con la profondità corretta. Tutti i test M5 di picking e interazione devono restare verdi.

**Validation:**
```bash
ctest --test-dir build -R "picking|m5_interaction|model_interaction" --output-on-failure
```

### Task 10.8: Snapshot, render_view e prestazioni
**Action:** MODIFY
**Files:**
- `apps/sagomacad-cli/main.cpp`, `apps/sagomacad-cli/mcp_visual.cpp`
- `tests/test_mcp.py`

**Details:**
Snapshot e `render_view` usano lo stesso renderer in un contesto nascosto. Misura: un modello di 200.000 triangoli deve restare sopra 60 fps durante l'orbita sulla macchina dell'utente; annota i numeri nel PRP o nel devlog.

**Validation:**
```bash
pytest tests/test_mcp.py -k render_view
```

### Task 10.9: Handle con un nome
**Action:** CREATE
**Files:**
- `src/ui/handles.hpp`, `src/ui/handles.cpp`
- `apps/common/shell.cpp`, `apps/common/*_widget.cpp`, `apps/common/control.cpp`
- `docs/control-protocol.md`

**Details:**
Registro azzerato a ogni frame: `publish_handle(nome, punto)`, `publish_rect(nome, rettangolo)`, `publish_count(nome, valore)`. Pubblica: pulsanti del ribbon `toolbar:<id comando>`, righe del browser `row:<etichetta>` e frecce `fold:<etichetta>`, voci della timeline `timeline:<id feature>`, campi dei dialog `field:<etichetta>`, maniglie `handle:<nome>` (freccia, raggio, quota), zone del cubo `cube:<zona>`. `ui.inspect` restituisce `handles`; nuovi metodi `ui.at {handle}` e `ui.click {handle}`.

**Validation:**
```bash
pytest tests/test_control.py -k handles
```

### Task 10.10: Esecutore degli scenari JSON
**Action:** CREATE
**Files:**
- `apps/sagomacad-cli/scenario.cpp`, `apps/sagomacad-cli/scenario.hpp`
- `CMakeLists.txt` (un test ctest per ogni `tests/scenarios/*.json`)
- `docs/scenarios.md`

**Details:**
`sagomacad-cli scenario run <file>` guida l'app vera senza finestra, frame per frame, come una persona. Passi: `start` (clic sul pulsante del comando), `key`, `text`, `click`, `double`, `move`, `drag`, `hold` (pressione lunga), `call` (qualsiasi metodo del canale di controllo), `expect` (corpi, volume con tolleranza, errori, dialog aperto, schizzo attivo, gradi di libertà), `until` (aspetta finché i controlli passano), `shot`, `note`, `pending`. Punti: `[x,y]` schermo, `{"world":[x,y,z]}`, `{"sketch":[x,y]}`, `{"plane":"XY"}`, `{"handle":"..."}`, `{"browser":"..."}`. Aggiungere uno scenario non richiede codice.

**Validation:**
```bash
ctest --test-dir build -R scenario --output-on-failure
```

### Task 10.11: Primi scenari
**Action:** CREATE
**Files:**
- `tests/scenarios/*.json` (almeno 10)

**Details:**
Copri i flussi già fatti: rettangolo `40` Tab `20` Invio completamente vincolato; estrusione di 20 mm con volume; schizzo su faccia e taglio; raccordo che sopravvive alla modifica a monte; foro quotato dai bordi; Seleziona altro con pressione lunga; clic su spigolo del cubo; modifica di un parametro con aggiornamento del solido; rollback della timeline; annulla e ripeti di un trascinamento. Ogni scenario ha almeno un `expect` numerico.

**Validation:**
```bash
ctest --test-dir build -R scenario --output-on-failure
```

### Task 10.12: Profili con contorni intersecanti
**Action:** MODIFY
**Files:**
- `src/solver/profile_regions.cpp`, `tests/profile_regions.cpp`, `tests/scenarios/profili-intersecanti.json`

**Details:**
Riproduzione in `docs/sketch-extrude-audit.md`: due cerchi sovrapposti devono dare tre regioni (due lunette e la lente), ciascuna selezionabile ed estrudibile, con riferimenti alle entità stabili quando si modificano i raggi. Prima il test che fallisce, poi la correzione.

**Validation:**
```bash
ctest --test-dir build -R "profile_regions|profili-intersecanti" --output-on-failure
```

### Task 10.13: Verifica criterio di fine fase
**Action:** VERIFY
**Details:** Controlla per davvero il criterio di M5.5: tavola approvata presente; screenshot di un pezzo M5 a tre angoli (isometrico, radente, dal basso) con spigoli netti e sagome, confrontati con 17-19 per il comportamento e con `docs/design/approvate/` per l'aspetto; almeno 10 scenari verdi in ctest; due cerchi sovrapposti danno tre regioni. Build Release, ctest, livelli, fuzz. Se fallisce, aggiungi task di correzione.

**Validation:**
```bash
cmake --build build --config Release && ctest --test-dir build --output-on-failure && python tools/check_layers.py && ./build/apps/sagomacad-cli/sagomacad-cli fuzz-commands
```
