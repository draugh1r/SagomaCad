# Phase 11: Immediatezza Tinkercad

> Phase 11 of 15 - Milestone M6 "Immediatezza Tinkercad" of PLAN.md

## Overview
Porta in SagomaCad la velocità di Tinkercad senza perdere il parametrico: forme trascinate da un pannello su un piano o su una faccia, fori trascinati, premi/tira sulle facce, appoggio di una faccia su un'altra, spostamento con una triade, allineamenti rapidi. Ogni gesto produce feature normali nella timeline, modificabili con le quote, e ogni trascinamento è un solo passo di annulla.

## Plan References
- PLAN.md: "0. Obiettivo" (immediatezza Tinkercad), "4. Modello del documento" (`Primitive`, `Place`), "5. Comandi" (`primitive.add`, `body.place`, `face.pushPull`, `selection.cycle`), "6. Strumenti e overlay", "12. Traguardi" (M6)
- RULES.md: regole 1, 3, 5, 8, 10 e sezione 7 (riferimenti stabili)
- `docs/modeling-guide.md`, `docs/mcp.md`

## Riferimenti SolidCraft
Clone in `../solidcraft-ref`. Da studiare e tradurre:
- `crates/ui-egui/src/smart_extrude.rs`: direzione dal lato in cui si prende il profilo, operazione automatica (Taglia se il trascinamento entra in un corpo, Unisci se esiste un solido, altrimenti Nuovo corpo) **bloccata** finché il valore non ripassa per lo zero; una scelta manuale spegne l'automatismo
- `crates/engine/src/cmd/mod.rs`: campo `live` di `CommandSpec`, cioè una versione veloce del comando durante il gesto, senza passo di annulla, e il comando vero al rilascio
- `crates/ui-egui/src/gizmo.rs`: triade con frecce X/Y/Z, quadrati dei piani, anelli di rotazione e sfera centrale; dimensione costante sullo schermo; scatti a valori tondi in base allo zoom e a 5 gradi, Alt o Ctrl per il movimento libero
- `crates/ui-egui/src/drag_snap.rs`: aggancio a punti, centri, metà dei lati e curve, allineamento orizzontale e verticale con guida tratteggiata
- `crates/ui-egui/src/canvas.rs`: casella del valore e freccia nel viewport (già presenti in M4-M5: riusale)
- Navigazione preset Tinkercad in `crates/ui-egui/src/navigation.rs` (facoltativa, solo come idea)

## Phase Exit Criteria
(Copiato da M6) Una scatola con foro centrale si crea in meno di 10 secondi senza aprire uno schizzo, e nella timeline compaiono feature normali e modificabili.

## Features Covered
- Primitive (box, cilindro, sfera, cono, toro) trascinate dal pannello su un piano o su una faccia, come feature parametriche
- Fori trascinati sulle facce
- `face.pushPull` che scrive nella feature esistente quando può, altrimenti crea un'estrusione
- `body.place` faccia su faccia con snap al centro e ai bordi e offset
- Allineamenti rapidi
- `selection.cycle` registrato come comando
- Operazione automatica bloccata durante il trascinamento (SolidCraft)
- Comandi `live` per i gesti (SolidCraft)
- Triade di spostamento e aggancio durante il trascinamento (SolidCraft)

## Dependencies
- Fase 10 completata (viewport GPU, handle, scenari)

## Tasks

### Task 11.0: Screenshot di riferimento (bloccante)
**Action:** USER
**Files:**
- `docs/reference/fusion/20-premi-tira.png` ... `23-primitiva-box.png`, ognuno con `.md`
- `docs/reference/tinkercad/24-forme.png` ... `28-maniglie.png`, ognuno con `.md`

**Details:**
Se i file mancano, **fermati** e scrivi all'utente queste istruzioni, poi aspetta. Servono solo per il comportamento, non per l'aspetto.

> **In Fusion** (cartella `docs/reference/fusion/`), parti da una scatola estrusa:
> 1. **20-premi-tira.png**: premi **Q** (Press Pull), clicca la faccia superiore e trascina un po' la freccia verso l'alto. Screenshot con freccia, casella del valore e dialog visibili.
> 2. **21-sposta-triade.png**: seleziona il corpo e premi **M** (Move/Copy). Screenshot con la triade (frecce, quadrati, anelli) e il dialog.
> 3. **22-allinea.png**: crea un secondo corpo, poi **MODIFY > Align**. Clicca una faccia del primo corpo e una del secondo, screenshot con l'anteprima.
> 4. **23-primitiva-box.png**: **CREATE > Box**, clicca un piano e trascina la base. Screenshot mentre disegni, con le caselle delle misure.
>
> **In Tinkercad** (gratis su tinkercad.com, cartella `docs/reference/tinkercad/`):
> 5. **24-forme.png**: pannello delle forme a destra mentre trascini un **Box** sul piano di lavoro.
> 6. **25-foro.png**: un **Box** e un **Cylinder** impostato come **Hole**, sovrapposti, prima di raggrupparli.
> 7. **26-piano-lavoro.png**: con lo strumento **Workplane** (tasto W) clicca una faccia del box: si vede il piano di lavoro arancione sulla faccia; poi trascina una forma sopra.
> 8. **27-allinea.png**: seleziona due forme e premi **L** (Align): si vedono i pallini di allineamento sui tre assi.
> 9. **28-maniglie.png**: seleziona una forma e clicca una maniglia d'angolo: si vedono le caselle con le misure.
>
> Accanto a ogni immagine crea un file `.md` con lo stesso nome e una o due righe su cosa guardare, per esempio "il valore si scrive direttamente nella casella vicino alla maniglia".

**Validation:**
```bash
ls docs/reference/fusion/2[0-3]-*.png docs/reference/tinkercad/2[4-8]-*.png
```

### Task 11.1: Modello delle primitive complete
**Action:** MODIFY
**Files:**
- `src/doc/document.hpp`, `src/doc/document.cpp`
- `src/regen/model_features.cpp`, `src/kernel/shape.cpp`
- `tests/primitive.cpp`

**Details:**
`Primitive` con tipo box, cilindro, sfera, cono, toro (aggiungi quelli che mancano), posizionamento su piano dell'origine o su faccia (riferimento stabile M5 + punto sulla faccia + rotazione nel piano), dimensioni come espressioni, operazione (nuovo corpo, unisci, taglia, interseca). Formato `.sagomacad` con default per i file vecchi. Test di volume per ogni tipo con GProp.

**Validation:**
```bash
ctest --test-dir build -R primitive --output-on-failure
```

### Task 11.2: Comandi live per i gesti
**Action:** MODIFY
**Files:**
- `src/engine/command.hpp`, `src/engine/registry.cpp`
- `tests/command.cpp`

**Details:**
Campo `live` opzionale in `CommandSpec`: durante il trascinamento la UI chiama la versione live (anteprima veloce, nessun passo di annulla, nessuna scrittura nel documento); al rilascio chiama il comando vero, che vale un solo passo di annulla. Sostituisce la coalescenza dove serve. Test: dieci frame live più il rilascio = un solo passo nella storia.

**Validation:**
```bash
ctest --test-dir build -R command --output-on-failure
```

### Task 11.3: Operazione automatica bloccata
**Action:** CREATE
**Files:**
- `src/engine/tools/smart_operation.hpp`, `src/engine/tools/smart_operation.cpp`
- `tests/smart_operation.cpp`

**Details:**
Traduci la logica di `smart_extrude.rs`: dentro un corpo = Taglia, fuori con un solido presente = Unisci, altrimenti Nuovo corpo; la scelta resta bloccata finché il valore non passa per lo zero, poi si decide di nuovo per l'altro lato; una scelta a mano spegne l'automatismo. Usata da premi/tira, forme e fori trascinati, ed estrusione da faccia. Test sui casi del documento SolidCraft.

**Validation:**
```bash
ctest --test-dir build -R smart_operation --output-on-failure
```

### Task 11.4: Premi/tira
**Action:** CREATE
**Files:**
- `src/engine/cmds/face_cmds.cpp`, `src/regen/extrusion.cpp`
- `tests/push_pull.cpp`, `tests/scenarios/premi-tira.json`

**Details:**
`face.pushPull {face, distance}` con scorciatoia **Q**. Se la faccia è il tappo di un'estrusione o un lato di una primitiva, modifica l'espressione di quella feature (un passo di annulla). Altrimenti crea un'estrusione dalla faccia con l'operazione automatica del task 11.3. Freccia e casella del valore come nell'estrusione. Test: volume dopo spinta e tirata, modifica della feature esistente quando possibile, errore leggibile su faccia non piana.

**Validation:**
```bash
ctest --test-dir build -R "push_pull|premi-tira" --output-on-failure
```

### Task 11.5: Pannello Forme
**Action:** CREATE
**Files:**
- `src/ui/layout.cpp` (nuovo nodo Clay `shapes`), `src/ui/state.hpp`
- `apps/common/shapes_panel.cpp`
- `src/ui/toolbar_catalog.cpp`

**Details:**
Pannello comprimibile con tessere per box, cilindro, sfera, cono, toro e le versioni foro (box foro, cilindro foro). Icone dal renderer della fase 9. Trascinando una tessera nel viewport compare un fantasma che si appoggia al piano sotto il cursore o alla faccia sotto il cursore (come il piano di lavoro di Tinkercad, `26-piano-lavoro`), con dimensioni predefinite dai token. Al rilascio: `primitive.add` con il posizionamento. Aspetto dai token, comportamento da 24 e 26.

**Validation:**
```bash
ctest --test-dir build -R "layout|scenario" --output-on-failure
```

### Task 11.6: Fori trascinati
**Action:** MODIFY
**Files:**
- `apps/common/shapes_panel.cpp`, `src/engine/cmds/primitive_cmds.cpp`
- `tests/scenarios/foro-trascinato.json`

**Details:**
Una tessera foro rilasciata su una faccia crea una primitiva con operazione Taglia nel corpo sotto il cursore, con le quote di posizione rispetto a due spigoli come per il foro M5. Un cilindro foro su una faccia piana può diventare `feature.hole` se l'utente sceglie "Foro" nel dialog. Comportamento da `25-foro`.

**Validation:**
```bash
ctest --test-dir build -R foro-trascinato --output-on-failure
```

### Task 11.7: Maniglie delle dimensioni
**Action:** CREATE
**Files:**
- `apps/common/primitive_handles.cpp`, `src/engine/tools/primitive_tool.cpp`

**Details:**
Con una primitiva selezionata: maniglie agli angoli e sui lati che cambiano le dimensioni, Shift per proporzioni uniformi, casella del valore accanto alla maniglia attiva (scrivi il numero, Invio). Ogni trascinamento usa il comando live e scrive nelle espressioni della feature. Comportamento da `28-maniglie`. Handle pubblicati con nome (`handle:size-x` e simili).

**Validation:**
```bash
ctest --test-dir build -R scenario --output-on-failure
```

### Task 11.8: Triade di spostamento
**Action:** CREATE
**Files:**
- `apps/common/gizmo.cpp`, `src/engine/cmds/body_cmds.cpp`, `src/doc/document.hpp`
- `tests/body_move.cpp`

**Details:**
Nuova feature di trasformazione `body.move {bodies, translation, rotation}` con scorciatoia **M**, nella timeline. Triade tradotta da `gizmo.rs`: frecce, quadrati dei piani, anelli, sfera centrale, dimensione costante sullo schermo, scatti in base allo zoom e a 5 gradi, Alt o Ctrl per il movimento libero, valori digitabili. Serve per riposizionare le forme trascinate. Comportamento da `21-sposta-triade`. Test di bounding box dopo spostamento e rotazione.

**Validation:**
```bash
ctest --test-dir build -R body_move --output-on-failure
```

### Task 11.9: Appoggia faccia su faccia
**Action:** CREATE
**Files:**
- `src/regen/place_feature.cpp`, `src/engine/cmds/body_cmds.cpp`
- `tests/body_place.cpp`

**Details:**
`body.place {body, from_face, to_face, offset, snap: center|edge|none, flip}` come da PLAN.md sezione 4: la faccia di partenza combacia con quella di arrivo (normali opposte), poi il corpo scorre sulla faccia con scatti al centro e ai bordi. Riferimenti stabili M5; se una faccia sparisce la feature va in errore, mai sulla faccia più vicina. Test con bounding box attesi.

**Validation:**
```bash
ctest --test-dir build -R body_place --output-on-failure
```

### Task 11.10: Aggancio durante il trascinamento
**Action:** CREATE
**Files:**
- `src/engine/tools/drag_snap.cpp`, `apps/common/geometry_widget.cpp`

**Details:**
Traduzione di `drag_snap.rs` per corpi e forme: aggancio a centri delle facce, metà degli spigoli, vertici e facce di altri corpi, allineamento con guida tratteggiata. Ctrl o Alt lo spengono. Usato da forme trascinate, triade e appoggio.

**Validation:**
```bash
ctest --test-dir build -R scenario --output-on-failure
```

### Task 11.11: Allineamenti rapidi
**Action:** CREATE
**Files:**
- `src/engine/cmds/body_cmds.cpp`, `apps/common/align_widget.cpp`
- `tests/body_align.cpp`

**Details:**
`body.align {bodies, axis, mode: min|center|max, reference}` con scorciatoia **L**, come feature nella timeline. Nel viewport: pallini sui tre assi del riquadro della selezione, passandoci sopra si vede l'anteprima, clic per applicare. Comportamento da `27-allinea` e `22-allinea`.

**Validation:**
```bash
ctest --test-dir build -R body_align --output-on-failure
```

### Task 11.12: Seleziona il successivo come comando
**Action:** MODIFY
**Files:**
- `src/engine/cmds/` (registrazione `selection.cycle`), `tools/update_parity.py`

**Details:**
Il clic ripetuto di M5 diventa il comando `selection.cycle {screen_point}`, raggiungibile da palette, CLI e MCP. `docs/parity.md` aggiornato.

**Validation:**
```bash
python tools/update_parity.py --check && pytest tests/test_mcp.py -k cycle
```

### Task 11.13: Timeline, browser e modifica
**Action:** MODIFY
**Files:**
- `apps/common/timeline_widget.cpp`, `src/ui/feature_editor.cpp`

**Details:**
Icone e nomi per primitive, spostamento, appoggio, allineamento; doppio clic riapre il dialog con le espressioni originali; errori in rosso con messaggio.

**Validation:**
```bash
ctest --test-dir build -R "timeline|feature_edit" --output-on-failure
```

### Task 11.14: CLI, MCP e guida
**Action:** MODIFY
**Files:**
- `docs/modeling-guide.md`, `docs/mcp.md`, `tests/test_modeling_agents.py`

**Details:**
Tutti i nuovi comandi raggiungibili da CLI e MCP, con test. Nella guida per agenti un "flusso rapido": scatola, foro trascinato, appoggio di un secondo corpo, allineamento, export.

**Validation:**
```bash
pytest tests/test_modeling_agents.py
```

### Task 11.15: Fuzz dei comandi
**Action:** MODIFY
**Files:**
- `apps/sagomacad-cli/fuzz.cpp`

**Details:**
Parametri ostili per i nuovi comandi (facce inesistenti, offset NaN, rotazioni enormi, corpi cancellati): errori, mai crash.

**Validation:**
```bash
./build/apps/sagomacad-cli/sagomacad-cli fuzz-commands
```

### Task 11.16: Verifica criterio di fine fase
**Action:** VERIFY
**Details:** Scenario `tests/scenarios/scatola-foro-rapida.json`: trascina un box sul piano, un cilindro foro al centro della faccia superiore, conferma; misura il numero di passi e controlla il volume. Poi prova tu a mano e cronometra: deve servire meno di 10 secondi, senza aprire uno schizzo. Nella timeline devono comparire una primitiva e un taglio, entrambi modificabili con doppio clic. Confronta il comportamento con 20-28 e l'aspetto con `docs/design/approvate/`. Build, ctest, livelli, fuzz. Se fallisce, aggiungi task di correzione.

**Validation:**
```bash
cmake --build build --config Release && ctest --test-dir build --output-on-failure && python tools/check_layers.py
```
