# Phase 12: Export e stampa

> Phase 12 of 15 - Milestone M7 "Export e stampa" of PLAN.md

## Overview
Rende SagomaCad comodo per la stampa 3D: export in 3MF e STEP oltre a STL, tolleranza di tassellazione impostabile, export di un corpo o di tutti, apertura diretta in Bambu Studio e controllo di stampabilità. L'export continua a chiedere al motore lo stadio del corpo: STEP prende la geometria esatta, STL e 3MF il risultato finale (oggi coincidono, con le SDF no).

## Plan References
- PLAN.md: "11. Accesso per agenti AI" (Tool `export`, Misure `check_printability`), "12. Traguardi" (M2 e M7), "13. Feature SDF" (Regole di modello, punto 3: due uscite per corpo)
- RULES.md: regole 1, 5, 8, 10

## Riferimenti SolidCraft
Clone in `../solidcraft-ref`. Da studiare:
- `crates/io/` (export 3MF con nomi e colori dei corpi, STEP AP242 con nomi e colori, STL binario)
- `crates/ui-egui/src/dialogs_save_mesh.rs`: dialog "Salva come mesh" con l'elenco degli slicer trovati sul computer
- `docs/formats/3mf-face-tags.md` (idea per il futuro, non da implementare ora)
- Nel README: lo strumento esterno Send2Orca, che manda il progetto aperto a OrcaSlicer come STEP, 3MF o STL

## Phase Exit Criteria
(Copiato da M7) STL, 3MF e STEP dello stesso pezzo si aprono correttamente in Bambu Studio e in un altro CAD.

## Features Covered
- `export.3mf`, `export.step`
- Tolleranza di tassellazione impostabile
- Export di un corpo o di tutti
- Apertura diretta in Bambu Studio se installato
- Stadio del corpo: STEP esatto, STL e 3MF finale
- MCP: `export` anche per 3MF e STEP, `check_printability`
- 3MF con nomi e colori dei corpi e dialog "Salva come mesh" con scelta dello slicer (SolidCraft)

## Dependencies
- Fase 11 completata

## Tasks

### Task 12.0: Screenshot di riferimento (bloccante)
**Action:** USER
**Files:**
- `docs/reference/fusion/29-export.png` ... `31-stampa-3d.png`, ognuno con `.md`
- `docs/reference/bambu/32-import-stl.png`, `33-oggetti-3mf.png`, ognuno con `.md`

**Details:**
Se i file mancano, **fermati** e scrivi all'utente:

> Per l'export mi servono questi screenshot, solo per il comportamento.
> **In Fusion** (cartella `docs/reference/fusion/`), con un pezzo aperto:
> 1. **29-export.png**: **File > Export**, con il menu Type aperto (si vedono i formati).
> 2. **30-salva-mesh.png**: nel browser a sinistra, tasto destro su un corpo > **Save As Mesh**. Screenshot del dialog con Format e Refinement aperti.
> 3. **31-stampa-3d.png**: **UTILITIES > MAKE > 3D Print**. Screenshot del dialog con le opzioni (Refinement, Send to 3D Print Utility).
>
> **In Bambu Studio** (cartella `docs/reference/bambu/`):
> 4. **32-import-stl.png**: importa un STL esportato da SagomaCad (per esempio il box di M2) e fai lo screenshot del piatto.
> 5. **33-oggetti-3mf.png**: importa un 3MF con due o più oggetti, anche fatto con Fusion, e fai lo screenshot della lista Objects a sinistra, dove si vedono i nomi.
>
> Accanto a ogni immagine un `.md` con una riga su cosa guardare.

**Validation:**
```bash
ls docs/reference/fusion/29-*.png docs/reference/fusion/30-*.png docs/reference/fusion/31-*.png docs/reference/bambu/3[23]-*.png
```

### Task 12.1: Stadio del corpo nell'export
**Action:** MODIFY
**Files:**
- `src/engine/cmds/export_cmds.cpp`, `src/regen/regenerator.cpp`
- `tests/export_stl.cpp`

**Details:**
Funzione unica `body_output(body, stage: exact|final)`: STEP chiede `exact`, STL e 3MF `final`. Oggi i due coincidono; un test fittizio con uno stadio finale diverso verifica che STEP non lo usi. Corpi di tipo `mesh` rifiutati da STEP con errore leggibile.

**Validation:**
```bash
ctest --test-dir build -R export --output-on-failure
```

### Task 12.2: Tolleranza di tassellazione
**Action:** MODIFY
**Files:**
- `src/doc/document.hpp` (impostazione del documento), `src/mesh/surface.cpp`, `src/engine/cmds/export_cmds.cpp`

**Details:**
Tolleranza di corda e angolo come impostazione del documento (con valori predefiniti fine, medio, grezzo) e sovrascrivibile in ogni export. Test: più fine = più triangoli e volume più vicino a quello esatto.

**Validation:**
```bash
ctest --test-dir build -R export --output-on-failure
```

### Task 12.3: Export 3MF
**Action:** CREATE
**Files:**
- `src/mesh/three_mf.cpp`, `src/mesh/three_mf.hpp`, `vcpkg.json` (lib3mf o zip minimale), `ATTRIBUTION.md` se serve
- `tests/export_3mf.cpp`

**Details:**
`export.3mf {bodies?, path, tolerance?, separate: bool}`: un oggetto per corpo con il suo nome e il colore del corpo, unità in millimetri. Test: rileggi il file, controlla numero di oggetti, nomi e volume di ciascuno entro la tolleranza.

**Validation:**
```bash
ctest --test-dir build -R export_3mf --output-on-failure
```

### Task 12.4: Export STEP
**Action:** CREATE
**Files:**
- `src/kernel/step_export.cpp`, `src/engine/cmds/export_cmds.cpp`
- `tests/export_step.cpp`

**Details:**
`export.step {bodies?, path}` con OCCT (`STEPCAFControl_Writer`) dallo stadio esatto, con nomi e colori dei corpi. Eccezioni OCCT catturate nel kernel. Test: rileggi con OCCT, confronta volume e numero di facce con il corpo originale.

**Validation:**
```bash
ctest --test-dir build -R export_step --output-on-failure
```

### Task 12.5: Un corpo o tutti
**Action:** MODIFY
**Files:**
- `src/engine/cmds/export_cmds.cpp`, `apps/common/shell.cpp`

**Details:**
Tutti gli export accettano un elenco di corpi; senza elenco esportano i corpi visibili. Tasto destro su un corpo nel browser > "Esporta corpo...". Corpi nascosti esclusi a meno di richiesta esplicita.

**Validation:**
```bash
pytest tests/test_cli.py -k export
```

### Task 12.6: Dialog "Salva come mesh"
**Action:** CREATE
**Files:**
- `apps/common/export_dialog.cpp`, `src/ui/frame.cpp` (finestra dalla fabbrica unica)

**Details:**
Formato (STL, 3MF), finezza (fine, media, grezza, personalizzata con tolleranze), corpi, unione o oggetti separati, e "Apri con" con l'elenco degli slicer trovati (task 12.7). Comportamento da 29-31, aspetto dai token. Ricorda le ultime scelte.

**Validation:**
```bash
sagomacad-cli snapshot --size 1440x900 --scale 2 --out build/check-export.png --script '[["ui.start",{"command":"export.mesh"}]]'
```

### Task 12.7: Apri in Bambu Studio
**Action:** CREATE
**Files:**
- `src/automation/slicers.cpp` (ricerca degli slicer installati), `src/engine/cmds/export_cmds.cpp`
- `tests/slicers.cpp`

**Details:**
Ricerca di Bambu Studio, OrcaSlicer e PrusaSlicer nei percorsi tipici di Windows (Program Files, registro di disinstallazione) e macOS (Applications). Comando `export.openInSlicer {slicer?, bodies?}`: esporta un 3MF temporaneo e avvia lo slicer con il file. Senza slicer trovato, errore leggibile con il suggerimento di scegliere il percorso a mano (salvato nelle preferenze). Pulsante nella scheda Esporta. Test con percorsi finti.

**Validation:**
```bash
ctest --test-dir build -R slicers --output-on-failure
```

### Task 12.8: Controllo di stampabilità
**Action:** CREATE
**Files:**
- `src/kernel/printability.cpp`, `src/engine/cmds/measure_cmds.cpp`, `apps/common/geometry_widget.cpp`
- `tests/printability.cpp`

**Details:**
`inspect.printability {body, min_wall: "0.8 mm", max_overhang: "45 deg"}` che restituisce: solido chiuso o no (BRepCheck più mesh senza bordi liberi), zone con pareti sotto lo spessore minimo (campionamento con raggi lungo la normale interna), facce con sbalzo oltre l'angolo rispetto a Z. Nel viewport le zone sono evidenziate con i token di avviso. Test su pezzi costruiti apposta (parete da 0,5 mm, sbalzo a 60 gradi, solido aperto).

**Validation:**
```bash
ctest --test-dir build -R printability --output-on-failure
```

### Task 12.9: MCP e CLI
**Action:** MODIFY
**Files:**
- `apps/sagomacad-cli/mcp.cpp`, `docs/mcp.md`, `docs/agent-cli.md`, `tests/test_mcp.py`

**Details:**
Lo strumento MCP `export` accetta `format: stl|3mf|step`, `bodies`, `tolerance`, sempre dentro le cartelle radice concesse e con `overwrite` esplicito. Nuovo strumento `check_printability`. Test di conformità e di percorso rifiutato.

**Validation:**
```bash
pytest tests/test_mcp.py
```

### Task 12.10: Fuzz e guida
**Action:** MODIFY
**Files:**
- `apps/sagomacad-cli/fuzz.cpp`, `docs/modeling-guide.md`

**Details:**
Parametri ostili per export e stampabilità (percorsi vuoti, tolleranze zero o negative, corpi inesistenti). Nella guida per agenti: "prepara per la stampa" (controllo, correzione, export 3MF).

**Validation:**
```bash
./build/apps/sagomacad-cli/sagomacad-cli fuzz-commands
```

### Task 12.11: Verifica criterio di fine fase
**Action:** VERIFY
**Details:** Esporta lo stesso pezzo (staffa a L con fori di M4) in STL, 3MF e STEP; verifica i volumi nei test. Poi chiedi all'utente di aprire i tre file in Bambu Studio e il file STEP in un altro CAD (FreeCAD gratuito o Fusion), con queste istruzioni: "Apri `export/staffa.stl`, `export/staffa.3mf` e `export/staffa.step` in Bambu Studio e dimmi se il pezzo e le misure sono giusti; poi apri `export/staffa.step` in FreeCAD o Fusion e controlla che sia un solido unico". Prova anche "Apri in Bambu Studio" dall'app. Build, ctest, livelli, fuzz. Se qualcosa fallisce, aggiungi task di correzione.

**Validation:**
```bash
cmake --build build --config Release && ctest --test-dir build --output-on-failure && python tools/check_layers.py
```
