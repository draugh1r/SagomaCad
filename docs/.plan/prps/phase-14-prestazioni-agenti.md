# Phase 14: Rifinitura, parte 2 - prestazioni e agenti

> Phase 14 of 15 - Milestone M8 "Rifinitura" of PLAN.md (seconda di due fasi)

## Overview
Mantiene SagomaCad fluido con modelli da centinaia di feature (anteprime e operazioni lunghe fuori dal thread della finestra, cache delle mesh) e rende visibile il lavoro degli agenti: pannello Agente con il registro delle azioni, interruttore "Consenti agenti", evidenziazione delle modifiche nel viewport e un cursore animato che mostra cosa fa l'agente.

## Plan References
- PLAN.md: "11. Accesso per agenti AI" (Esperienza utente nell'app, Sicurezza), "12. Traguardi" (M8)
- RULES.md: regole 2, 5, 10

## Riferimenti SolidCraft
Clone in `../solidcraft-ref`. Da studiare e tradurre:
- `crates/ui-egui/src/preview.rs`: anteprima calcolata su una copia della sessione in un altro thread; l'ultima anteprima buona resta mentre arriva la prossima; per unione e taglio si mostra solo l'utensile, la booleana gira all'OK
- `crates/ui-egui/src/deferred.rs`: OK, rollback, soppressione, annulla e ripeti calcolati fuori dal thread della finestra quando superano un'attesa breve; i comandi chiesti nel frattempo aspettano il loro turno; un marker spostato di nuovo sostituisce lo spostamento in attesa
- `crates/ui-egui/src/display_lod.rs` (solo come idea: dettaglio delle mesh che cresce quando ci si avvicina)
- `crates/ui-egui/src/agent_cursor.rs`: puntatore dell'agente che vola sul pulsante e sulle selezioni prima che il comando parta; la risposta aspetta l'animazione; spento di default
- `docs/agent-chat.md` e `crates/ui-egui/src/agent_chat/`: pannello laterale per parlare con un agente (idea per dopo, non in questa fase)

## Phase Exit Criteria
PLAN.md non definisce un "Fatto quando" per M8. Criteri proposti con il piano delle fasi del 2026-10-17:
- un modello generato con 300 feature si apre, e modificando il primo schizzo la finestra resta reattiva (nessun frame oltre 100 ms) mentre il modello si ricalcola;
- l'orbita su quel modello resta sopra 30 fps;
- un agente collegato via MCP compare nel pannello Agente con ogni comando, parametri ed esito; con "Consenti agenti" spento il collegamento in modalità bridge viene rifiutato con un errore chiaro;
- i passi dell'agente sono marcati nella timeline e le entità che ha modificato si illuminano per qualche secondo.

## Features Covered
- Prestazioni su modelli con centinaia di feature
- Anteprima e OK calcolati fuori dal thread della finestra (SolidCraft)
- Pannello Agente con registro delle azioni
- Interruttore "Consenti agenti"
- Evidenziazione delle modifiche dell'agente nel viewport
- Cursore animato dell'agente (SolidCraft)

## Dependencies
- Fase 13 completata

## Tasks

### Task 14.0: Mockup del pannello Agente (bloccante)
**Action:** USER
**Files:**
- `docs/design/mockup-pannello-agente.png`
- `docs/design/approvate/pannello-agente.png`

**Details:**
In Fusion non esiste un pannello equivalente, quindi il riferimento è un mockup nostro. Prima di implementare, disegna con la UI vera e dati finti (snapshot con uno stato di prova) il pannello Agente: intestazione con nome dell'agente e stato (collegato, in attesa, spento), interruttore "Consenti agenti", elenco delle azioni con ora, comando, parametri riassunti ed esito (icona e colore dai token), clic su una riga che seleziona la feature nella timeline. Salvalo in `docs/design/mockup-pannello-agente.png`, poi **fermati** e chiedi all'utente:

> Ho preparato il mockup del pannello Agente in `docs/design/mockup-pannello-agente.png`. Lo approvi? Se vuoi cambiare posizione (destra, sinistra, in basso), informazioni mostrate o stile, dimmelo.

Dopo l'approvazione copialo in `docs/design/approvate/pannello-agente.png`.

**Validation:**
```bash
test -f docs/design/approvate/pannello-agente.png
```

### Task 14.1: Modello di prova e misure
**Action:** CREATE
**Files:**
- `perf/generate_model.py`, `perf/run.py`, `perf/README.md`, `perf/baseline.json`

**Details:**
Script che genera via CLI un modello con 300 feature (schizzi, estrusioni, fori, raccordi, schemi ripetuti) e misura: apertura, ricalcolo dopo modifica del primo schizzo, tempo massimo di frame durante il ricalcolo, fps in orbita, memoria. Salva una baseline; ogni task successivo confronta i numeri.

**Validation:**
```bash
python perf/run.py --quick
```

### Task 14.2: Cache delle mesh per revisione
**Action:** MODIFY
**Files:**
- `src/regen/regenerator.cpp`, `src/mesh/surface.cpp`, `src/ui/render/gl_viewport.cpp`

**Details:**
Ricalcola dalla feature modificata in poi (già presente) e ritassella solo i corpi cambiati; il renderer ricarica solo i VBO dei corpi con revisione nuova. Misura prima e dopo con `perf/run.py`.

**Validation:**
```bash
python perf/run.py --quick --compare perf/baseline.json
```

### Task 14.3: Anteprima fuori dal thread della finestra
**Action:** CREATE
**Files:**
- `src/engine/preview_worker.cpp`, `src/engine/tools/model_preview.cpp`, `src/engine/tools/surface_preview.cpp`

**Details:**
Con un dialog aperto, il comando di anteprima gira su una copia della sessione in un thread di lavoro; l'ultima anteprima buona resta visibile; un errore dell'input si mostra nel dialog senza togliere l'anteprima. Per unione e taglio si mostra solo l'utensile e la booleana si calcola all'OK. Il documento e l'annulla non vengono mai toccati dall'anteprima. Test con un comando artificialmente lento.

**Validation:**
```bash
ctest --test-dir build -R preview --output-on-failure
```

### Task 14.4: OK e storia fuori dal thread della finestra
**Action:** CREATE
**Files:**
- `src/engine/deferred.cpp`, `apps/common/shell.cpp`

**Details:**
OK di una feature lenta, spostamento del marker, soppressione, annulla e ripeti: se superano un'attesa breve (per esempio 150 ms) continuano su un altro thread e la barra di stato dice "Calcolo in corso..."; i comandi chiesti nel frattempo aspettano in coda; un nuovo spostamento del marker sostituisce quello in attesa. Il canale di controllo risponde quando il calcolo è finito.

**Validation:**
```bash
ctest --test-dir build -R deferred --output-on-failure && python perf/run.py --quick
```

### Task 14.5: Registro delle azioni degli agenti
**Action:** CREATE
**Files:**
- `src/automation/agent_log.cpp`, `src/automation/dispatch.cpp`

**Details:**
Ogni richiesta che arriva dal canale di controllo o dall'MCP viene registrata con ora, origine (nome del client MCP se noto), comando, parametri riassunti, esito ed eventuali id delle feature create o modificate. Limite di righe in memoria. Esposto da `ui.inspect` e da un metodo `agent.log`.

**Validation:**
```bash
pytest tests/test_control.py -k agent_log
```

### Task 14.6: Pannello Agente
**Action:** CREATE
**Files:**
- `src/ui/layout.cpp` (nodo Clay), `apps/common/agent_panel.cpp`

**Details:**
Implementa il mockup approvato al task 14.0. Pannello comprimibile, misure persistenti come gli altri pannelli, clic su una riga che seleziona la feature e la mostra nella timeline.

**Validation:**
```bash
sagomacad-cli snapshot --size 1440x900 --scale 2 --out build/check-agent.png --script tests/scenarios/agente-registro.json
```

### Task 14.7: Interruttore "Consenti agenti"
**Action:** MODIFY
**Files:**
- `src/automation/dispatch.cpp`, `apps/common/control.cpp`, `apps/common/shell.cpp`, `src/ui/state.hpp`

**Details:**
Interruttore nella barra in alto e nel pannello, salvato nelle preferenze. Spento: la modalità bridge rifiuta nuove connessioni e richieste con un errore chiaro ("Gli agenti sono disattivati in SagomaCad"); la modalità headless non è toccata. Test di bridge rifiutato.

**Validation:**
```bash
pytest tests/test_mcp_integration.py -k allow_agents
```

### Task 14.8: Passi dell'agente nella timeline
**Action:** MODIFY
**Files:**
- `src/doc/document.hpp` (origine della feature), `apps/common/timeline_widget.cpp`

**Details:**
Le feature create da un agente portano l'origine "agente" e nella timeline hanno un piccolo segno; un `batch_run` resta un solo passo di annulla. Il formato del file accetta il campo con default per i file vecchi.

**Validation:**
```bash
ctest --test-dir build -R "document|timeline" --output-on-failure
```

### Task 14.9: Evidenziazione delle modifiche
**Action:** MODIFY
**Files:**
- `src/ui/render/gl_viewport.cpp`, `src/ui/theme.hpp`

**Details:**
Dopo un comando dell'agente, facce e corpi creati o modificati si illuminano con un token dedicato che sfuma in pochi secondi. Spegnibile da Preferenze, pagina Agenti.

**Validation:**
```bash
ctest --test-dir build -R scenario --output-on-failure
```

### Task 14.10: Cursore animato dell'agente
**Action:** CREATE
**Files:**
- `apps/common/agent_cursor.cpp`, `apps/common/control.cpp`

**Details:**
Traduzione di `agent_cursor.rs`: prima di eseguire un comando dell'agente, un puntatore riconoscibile vola sul pulsante del comando (usando gli handle `toolbar:<id>`) e poi sulle selezioni proiettate sullo schermo; solo dopo il comando parte e la risposta aspetta l'animazione. Spento di default; `ui.agent_cursor {show, speed}` e Preferenze. Con `animate: false` si salta.

**Validation:**
```bash
pytest tests/test_control.py -k agent_cursor
```

### Task 14.11: CLI, MCP, fuzz e documentazione
**Action:** MODIFY
**Files:**
- `docs/mcp.md`, `docs/agent-cli.md`, `apps/sagomacad-cli/fuzz.cpp`, `tests/test_mcp.py`

**Details:**
Documenta registro, interruttore e cursore; test MCP per `agent.log` e per il rifiuto con agenti disattivati; fuzz per i nuovi metodi.

**Validation:**
```bash
pytest tests/test_mcp.py && ./build/apps/sagomacad-cli/sagomacad-cli fuzz-commands
```

### Task 14.12: Verifica criterio di fine fase
**Action:** VERIFY
**Details:** Controlla per davvero i criteri proposti sopra con `perf/run.py` (300 feature, frame massimo, fps) e con una sessione MCP reale da Claude o Codex mentre l'app è aperta: righe nel pannello, segno nella timeline, evidenziazione, rifiuto con l'interruttore spento, cursore con l'opzione accesa. Confronto del pannello con `docs/design/approvate/pannello-agente.png`. Build, ctest, livelli, fuzz. Se qualcosa fallisce, aggiungi task di correzione.

**Validation:**
```bash
cmake --build build --config Release && ctest --test-dir build --output-on-failure && python tools/check_layers.py && python perf/run.py
```
