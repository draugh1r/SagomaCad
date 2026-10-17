# Phase 15: Test end-to-end

> Phase 15 of 15 - Milestone M8 "Rifinitura" of PLAN.md (fase di test finale)

## Overview
Verifica SagomaCad dall'inizio alla fine come lo userebbe una persona e come lo userebbe un agente: flussi completi pilotati dal canale di controllo con screenshot confrontati con le schermate approvate, un benchmark per agenti con compiti di modellazione misurati sui numeri, e il controllo finale di tutti i criteri di M5.5, M6, M7 e M8.

## Plan References
- PLAN.md: "12. Traguardi" (M5.5, M6, M7, M8), "11. Accesso per agenti AI" (Test)
- RULES.md: regole 7, 8, 9

## Riferimenti SolidCraft
Clone in `../solidcraft-ref`. Da studiare e tradurre:
- `tests/scenarios/` e `crates/ui-egui/src/scenario.rs`: scenari JSON con passi, attese e confronto immagini
- `bench/` (se presente) e `docs/agents.md`: compiti per agenti con verifica automatica

## Phase Exit Criteria
Tutti i criteri "Fatto quando" di M5.5, M6 e M7 in PLAN.md, più i criteri proposti per M8 nella fase 14, controllati di nuovo sul build finale. In più:
- gli scenari end-to-end di questa fase passano in ctest;
- il confronto con le schermate in `docs/design/approvate/` resta sotto la soglia scelta;
- il benchmark per agenti completa almeno 6 compiti su 8 con le misure corrette.

## Features Covered
- Flussi completi utente (schizzo, estrusione, raccordo, foro, export 3MF)
- Flusso rapido stile Tinkercad
- Modifica a monte che regge
- Ripristino dopo crash
- Agente via MCP che crea ed esporta una staffa
- Confronto con le schermate approvate (golden image)
- Benchmark per agenti

## Dependencies
- Fase 14 completata

## Tasks

### Task 15.0: MCP di Fusion per il confronto (bloccante)
**Action:** USER
**Files:**
- `docs/reference/fusion/39-mcp-fusion.md`

**Details:**
Il benchmark può confrontare SagomaCad con Fusion sugli stessi compiti, ma solo se l'utente ha un MCP per Fusion. **Fermati** e chiedi all'utente:

> Per il benchmark posso confrontare SagomaCad con Fusion. Hai un MCP per Fusion installato? Per controllare:
> 1. Apri Fusion.
> 2. Vai su **Utilities > Add-Ins > Scripts and Add-Ins** e guarda se c'è un add-in MCP attivo (per esempio uno che si chiama "MCP" o "Fusion MCP").
> 3. Se c'è, scrivi in `docs/reference/fusion/39-mcp-fusion.md` il nome dell'add-in, la porta o il comando con cui si collega, e incolla la riga di configurazione che usi in Claude o Codex.
> 4. Se non c'è, scrivi nello stesso file solo "nessuno": il benchmark girerà solo su SagomaCad.

Prosegui solo quando il file esiste.

**Validation:**
```bash
test -f docs/reference/fusion/39-mcp-fusion.md
```

### Task 15.1: Scenario schizzo, estrusione, raccordo, foro, 3MF
**Action:** CREATE
**Files:**
- `tests/e2e/flusso-classico.json`

**Details:**
Dal documento vuoto: schizzo rettangolare 60x40 con quote, estrusione 15, raccordo 3 su spigoli verticali, foro passante 8 al centro, export 3MF. Controlli: volume con tolleranza, numero di facce, file 3MF valido, screenshot a ogni passo. Solo clic su handle e scorciatoie, nessuna chiamata diretta a `engine.execute`, così si prova l'interfaccia vera.

**Validation:**
```bash
sagomacad-cli scenario run tests/e2e/flusso-classico.json
```

### Task 15.2: Scenario rapido stile Tinkercad
**Action:** CREATE
**Files:**
- `tests/e2e/flusso-rapido.json`

**Details:**
Box dal pannello Forme, cilindro trascinato come foro, maniglie per le misure, allinea al centro, sposta con il gizmo, export STL. Controlla che tutto sia una sola timeline modificabile dopo.

**Validation:**
```bash
sagomacad-cli scenario run tests/e2e/flusso-rapido.json
```

### Task 15.3: Modifica a monte che regge
**Action:** CREATE
**Files:**
- `tests/e2e/modifica-a-monte.json`

**Details:**
Sul modello del task 15.1 cambia la larghezza del primo schizzo e il raggio del raccordo: il foro resta al centro, il raccordo resta sugli stessi spigoli (riferimenti stabili), nessuna feature in errore. Poi una modifica che rompe di proposito: la feature va in errore nella timeline e le altre continuano.

**Validation:**
```bash
sagomacad-cli scenario run tests/e2e/modifica-a-monte.json
```

### Task 15.4: Ripristino dopo crash
**Action:** CREATE
**Files:**
- `tests/e2e/ripristino.py`

**Details:**
Avvia l'app con `--control`, fa alcune modifiche, attende il salvataggio di ripristino, termina il processo in modo brusco, riavvia e verifica che il ripristino proponga il documento con le modifiche e che il rapporto di crash sia elencato da `crashes list`.

**Validation:**
```bash
python tests/e2e/ripristino.py
```

### Task 15.5: Agente via MCP crea ed esporta una staffa
**Action:** CREATE
**Files:**
- `tests/e2e/agente-staffa.py`

**Details:**
Script che parla MCP in modalità bridge con l'app aperta (solo tool MCP): staffa a L 50x30x5 con due fori da 5 e raccordo interno, `check_printability`, export 3MF. Controlli: volume, righe nel pannello Agente, passo marcato nella timeline, file valido.

**Validation:**
```bash
python tests/e2e/agente-staffa.py
```

### Task 15.6: Confronto con le schermate approvate
**Action:** CREATE
**Files:**
- `tools/compare_golden.py`, `tests/e2e/golden.json`

**Details:**
Per ogni schermata in `docs/design/approvate/` uno scenario riproduce lo stesso stato e confronta il PNG con una differenza percettiva (per esempio media per pixel più percentuale di pixel oltre soglia), nei due temi e a due dimensioni. Le differenze producono un'immagine affiancata in `build/golden-diff/`. Le golden si aggiornano solo con approvazione dell'utente (regola 9).

**Validation:**
```bash
python tools/compare_golden.py --all
```

### Task 15.7: Benchmark per agenti
**Action:** CREATE
**Files:**
- `bench/tasks/*.md`, `bench/run.py`, `bench/README.md`

**Details:**
Otto compiti scritti in linguaggio naturale (cubo con foro, staffa, scatola con coperchio a incastro, distanziale, supporto per telefono, ingranaggio semplice, maniglia con raccordi, pezzo da modificare a monte), ognuno con misure attese (volume, bounding box, numero di fori). `run.py` lancia un agente (`claude -p` con `--output-format stream-json`, oppure Codex) collegato all'MCP di SagomaCad in headless, misura tempo, numero di chiamate, esito, e verifica l'STL esportato. Se il task 15.0 ha trovato un MCP di Fusion, stessi compiti anche su Fusion.

**Validation:**
```bash
python bench/run.py --dry-run
```

### Task 15.8: Rapporto
**Action:** CREATE
**Files:**
- `bench/report.md`, `log/devlog.md`

**Details:**
Tabella con esito, tempo e chiamate per compito; errori più frequenti degli agenti e idee per migliorare messaggi d'errore e guida per agenti. Voce nel devlog.

**Validation:**
```bash
test -f bench/report.md
```

### Task 15.9: Verifica finale
**Action:** VERIFY
**Details:** Ricontrolla sul build finale tutti i criteri "Fatto quando" di M5.5, M6, M7 e quelli proposti per M8 (fase 14). Esegui tutti gli scenari e2e, il confronto golden, `perf/run.py`, il benchmark completo, fuzz e livelli. Guarda di persona gli screenshot finali nei due temi. Chiedi all'utente di provare a mano un flusso completo e di aprire un 3MF in Bambu Studio. Se qualcosa fallisce, aggiungi task di correzione.

**Validation:**
```bash
cmake --build build --config Release && ctest --test-dir build --output-on-failure && python tools/check_layers.py && python tools/compare_golden.py --all && ./build/apps/sagomacad-cli/sagomacad-cli fuzz-commands
```
