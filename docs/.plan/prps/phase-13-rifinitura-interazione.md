# Phase 13: Rifinitura, parte 1 - interazione e sicurezza del lavoro

> Phase 13 of 15 - Milestone M8 "Rifinitura" of PLAN.md (prima di due fasi)

## Overview
Rende SagomaCad piacevole da usare tutti i giorni e sicuro per il lavoro dell'utente: menu radiale col tasto destro con Ripeti, scorciatoie personalizzabili con rilevamento dei conflitti, casella S di ricerca dei comandi al cursore, tema chiaro e scuro completi con finestra Preferenze, salvataggio di recupero e report dei crash riproducibili.

## Plan References
- PLAN.md: "8. UI (`ui/`)" (palette dei comandi), "10. Robustezza" (salvataggio automatico di recupero), "12. Traguardi" (M8)
- RULES.md: regole 1, 4, 5, 7

## Riferimenti SolidCraft
Clone in `../solidcraft-ref`. Da studiare e tradurre:
- `crates/ui-egui/src/context_menu.rs`: menu del tasto destro con lo stesso stile ovunque; menu radiale a otto voci con Ripeti in alto; tenendo premuto e muovendo si sceglie con un gesto; voci come comandi con parametri presi dal bersaglio; `ui.menu` e `ui.menuPick` sul canale di controllo
- `crates/ui-egui/src/keymap.rs`: scorciatoie predefinite dai comandi, modifiche dell'utente, conflitti segnalati e sostituiti solo su richiesta, tasti riservati (Esc, S, Canc, Ctrl+Z)
- `crates/ui-egui/src/shortcut_box.rs`: casella S al cursore con ricerca approssimata, recenti e preferiti
- `crates/ui-egui/src/prefs.rs`: preferenze salvate, valori sconosciuti ignorati senza perdere gli altri
- `crates/engine/src/crash*` e `docs/crash-reports.md`: ogni crash salva progetto e comandi; `crashes list/show/replay`
- `crates/ui-egui/src/recovery_ui.rs`: offerta di recupero all'avvio

## Phase Exit Criteria
Nessuno (fase intermedia di M8: i criteri sono nella fase 14).

## Features Covered
- Menu radiale col tasto destro, con Ripeti
- Scorciatoie personalizzabili con rilevamento dei conflitti
- Casella S di ricerca dei comandi al cursore (SolidCraft)
- Tema chiaro e scuro completi, finestra Preferenze
- Salvataggio di recupero
- Report dei crash riproducibili (SolidCraft)

## Dependencies
- Fase 12 completata

## Tasks

### Task 13.0: Screenshot di riferimento (bloccante)
**Action:** USER
**Files:**
- `docs/reference/fusion/34-menu-radiale-corpo.png` ... `38-casella-s.png`, ognuno con `.md`

**Details:**
Se i file mancano, **fermati** e scrivi all'utente:

> Per menu e scorciatoie mi servono questi screenshot di Fusion, solo per il comportamento. Salvali in `docs/reference/fusion/`.
> 1. **34-menu-radiale-corpo.png**: seleziona un corpo e fai **clic destro** nel viewport. Si apre il menu radiale con le voci intorno al cursore e l'elenco sotto: screenshot.
> 2. **35-menu-radiale-vuoto.png**: clic destro su uno spazio vuoto del viewport, senza niente selezionato.
> 3. **36-menu-comando.png**: passa il mouse su un comando della barra (per esempio Extrude), clicca i **tre puntini** che compaiono accanto e fai lo screenshot del menu con "Change Keyboard Shortcut".
> 4. **37-scorciatoia-dialog.png**: clicca **Change Keyboard Shortcut** e fai lo screenshot del dialog.
> 5. **38-casella-s.png**: premi **S** nel viewport: si apre la casella di ricerca dei comandi al cursore. Scrivi "fil" e fai lo screenshot.
> Accanto a ogni immagine un `.md` con una riga su cosa guardare.

**Validation:**
```bash
ls docs/reference/fusion/3[4-8]-*.png
```

### Task 13.1: Menu del tasto destro e menu radiale
**Action:** CREATE
**Files:**
- `src/engine/context_menu.cpp` (voci per bersaglio: faccia, spigolo, corpo, entità di schizzo, vuoto)
- `apps/common/context_menu_widget.cpp`, `apps/common/control.cpp`
- `tests/context_menu.cpp`, `tests/scenarios/menu-radiale.json`

**Details:**
Clic destro: otto comandi intorno al cursore (Ripeti in alto) più l'elenco per l'oggetto sotto il cursore. Tenendo premuto il destro e rilasciando su una direzione si sceglie con un gesto. Vicino ai bordi della finestra il menu si apre verso l'interno. Le voci sono id di comando con i parametri presi dal bersaglio; il canale di controllo ha `ui.menu` e `ui.menuPick`. Il pannello di contesto esistente del browser usa lo stesso stile.

**Validation:**
```bash
ctest --test-dir build -R "context_menu|menu-radiale" --output-on-failure
```

### Task 13.2: Ripeti l'ultimo comando
**Action:** MODIFY
**Files:**
- `src/ui/state.hpp`, `apps/common/shell.cpp`

**Details:**
L'ultimo comando avviato in modo interattivo resta come "Ripeti <nome>" in cima al menu radiale e sul tasto Invio a vuoto nel viewport (come in Fusion). Non si ripetono comandi di vista né Fine schizzo.

**Validation:**
```bash
ctest --test-dir build -R scenario --output-on-failure
```

### Task 13.3: Scorciatoie personalizzabili
**Action:** CREATE
**Files:**
- `src/ui/keymap.hpp`, `src/ui/keymap.cpp`
- `tests/keymap.cpp`

**Details:**
Scorciatoie predefinite dalle `CommandSpec`, modifiche dell'utente salvate nelle preferenze, tasti riservati non assegnabili, conflitto segnalato con il nome del comando che usa già il tasto e sostituito solo se l'utente conferma. Test: conflitti, tasti riservati, nessuna attivazione mentre si scrive in un campo, file corrotto ignorato senza perdere le altre preferenze.

**Validation:**
```bash
ctest --test-dir build -R keymap --output-on-failure
```

### Task 13.4: Finestra Preferenze
**Action:** CREATE
**Files:**
- `apps/common/preferences_window.cpp`, `src/ui/state.hpp`

**Details:**
Pagine: Generale (tema, lingua futura, unità predefinite), Navigazione (preset, velocità), Scorciatoie (ricerca, riassegnazione, ripristino, comportamento da 36-37), Agenti (riservata alla fase 14). Salvataggio atomico come `layout.json`. Finestra creata dalla fabbrica unica.

**Validation:**
```bash
sagomacad-cli snapshot --size 1440x900 --scale 2 --out build/check-prefs.png --script '[["ui.set",{"preferences":true}]]'
```

### Task 13.5: Casella S
**Action:** CREATE
**Files:**
- `apps/common/shortcut_box.cpp`, `src/ui/state.hpp`

**Details:**
Tasto **S** nel viewport: ricerca approssimata dei comandi per nome, id, scheda e gruppo, aperta dove si trova il cursore; senza testo mostra i recenti; una riga di preferiti fissati in alto. Su e Giù scelgono, Invio esegue, Esc chiude. Unifica con la palette Ctrl+K esistente (stesso motore di ricerca). Comportamento da `38-casella-s`.

**Validation:**
```bash
ctest --test-dir build -R scenario --output-on-failure
```

### Task 13.6: Tema chiaro e scuro completi
**Action:** MODIFY
**Files:**
- `src/ui/theme.hpp`, `tests/theme_tokens.cpp`, `tests/scenarios/tema-*.json`

**Details:**
Ogni token ha un valore in entrambi i temi; il cambio è immediato da Preferenze e salvato. Scenari che fotografano ribbon, browser, timeline, dialog, menu radiale e viewport nei due temi; confronto con `docs/design/approvate/`.

**Validation:**
```bash
ctest --test-dir build -R "theme_tokens|tema-" --output-on-failure
```

### Task 13.7: Salvataggio di recupero
**Action:** CREATE
**Files:**
- `src/automation/recovery.cpp`, `apps/common/recovery_dialog.cpp`
- `tests/recovery.cpp`

**Details:**
Ogni pochi minuti (impostabile) e dopo un'operazione pesante, salvataggio atomico nella cartella di recupero (`SDL_GetPrefPath`). All'avvio, se ci sono file più recenti dell'ultimo salvataggio dell'utente, finestra che offre di recuperarli o scartarli. Il recupero non sovrascrive mai il file dell'utente senza conferma.

**Validation:**
```bash
ctest --test-dir build -R recovery --output-on-failure
```

### Task 13.8: Report dei crash riproducibili
**Action:** CREATE
**Files:**
- `src/automation/crash_report.cpp`, `apps/sagomacad/main.cpp`, `apps/sagomacad-cli/main.cpp`
- `docs/crash-reports.md`, `tests/test_cli.py`

**Details:**
Su eccezione non gestita, `std::terminate` o eccezione strutturata di Windows: salva in una cartella dei crash il progetto e il registro dei comandi della sessione (`replay.json`). `sagomacad-cli crashes list`, `crashes show <id>`, `crashes replay <id>`. RULES.md: un crash si corregge partendo dal suo `replay.json` come test di regressione.

**Validation:**
```bash
pytest tests/test_cli.py -k crashes
```

### Task 13.9: Documentazione e fuzz
**Action:** MODIFY
**Files:**
- `docs/ui-design.md`, `docs/.plan/RULES.md` (regola sui crash), `apps/sagomacad-cli/fuzz.cpp`

**Details:**
Descrivi menu radiale, scorciatoie, casella S, recupero e crash. Fuzz per `ui.menuPick` e per le preferenze.

**Validation:**
```bash
./build/apps/sagomacad-cli/sagomacad-cli fuzz-commands && python tests/test_utf8.py
```

### Task 13.10: Verifica della fase
**Action:** VERIFY
**Details:** Scenari di menu radiale, Ripeti, casella S e temi verdi; prova a mano: riassegna una scorciatoia in conflitto, chiudi l'app a forza con un documento modificato e riaprila (deve offrire il recupero), provoca un crash di prova in una build di debug e riproducilo con `crashes replay`. Confronto con 34-38 per il comportamento e con `docs/design/approvate/` per l'aspetto. Build, ctest, livelli, fuzz. Se qualcosa fallisce, aggiungi task di correzione.

**Validation:**
```bash
cmake --build build --config Release && ctest --test-dir build --output-on-failure && python tools/check_layers.py
```
