# Phase 9: Identità visiva - riferimenti, icone e token

> Phase 9 of 15 - Milestone M5.5 "Identità visiva e infrastruttura UI" of PLAN.md (prima di due fasi)

## Overview
Dà a SagomaCad un aspetto suo, separando i riferimenti di comportamento (screenshot privati di Fusion) da quelli di aspetto (`docs/design/`, nostri). Sostituisce le icone Lucide del ribbon, del browser e della timeline con i master SVG di SolidCraft ricolorati per ruoli semantici, amplia i token del tema a tutti gli stati del viewport e centralizza la creazione delle finestre. La fase si chiude con una tavola di design approvata dall'utente.

## Plan References
- PLAN.md: "8. UI (`ui/`)", "12. Traguardi" (M5.5), "15. Decisioni"
- RULES.md: regole 4, 6, 7
- `docs/ui-design.md`, `docs/parity.md`, `src/ui/theme.hpp`, `src/ui/toolbar_catalog.cpp`, `apps/common/shell.cpp`

## Riferimenti SolidCraft
Clona il riferimento **fuori dal repository**: `git clone --depth 1 https://github.com/tangency-labs/solidcraft ../solidcraft-ref`. Licenza MIT OR Apache-2.0: gli SVG e la palette si riusano citandoli in `ATTRIBUTION.md`; il codice Rust si studia e si traduce in C++, citando i file da cui viene una logica non banale.
- `assets/icons/ui/STYLE_GUIDE.md`, `assets/icons/ui/AGENTS.md`: regole delle icone (griglia 64, spessori 2,55 / 4,25 / 5,6, ruoli colore, una sola idea per icona)
- `assets/icons/ui/design/svg/` (268 master in `flat/`, `model/`, `small/`, `workspace/`), `design/palette.toml`, `design/aliases.toml`
- `crates/ui-egui/src/icon_svg.rs`: parsing dei master, sostituzione dei ruoli, cache per (icona, accento, tema, pixel)
- `crates/ui-egui/src/theme.rs` e `assets/styles/ui/palettes.rs`: elenco dei token (circa 90) e stati del viewport
- `crates/ui-egui/src/frame.rs`: unica fabbrica di finestre e test che lo impone

## Phase Exit Criteria
Nessuno (fase intermedia di M5.5: il "Fatto quando" è nella fase 10). La fase però non si chiude senza la tavola approvata dal task 9.7.

## Features Covered
- Riferimenti separati: comportamento da `docs/reference/`, aspetto da `docs/design/`
- Icone SVG di SolidCraft con ricolorazione per ruoli, citate in ATTRIBUTION.md
- Token del tema ampliati agli stati del viewport
- Regola "una sola fabbrica di finestre", con test

## Dependencies
- Fasi 1-8 (M0-M5) completate

## Tasks

### Task 9.0: Riferimenti dall'utente (bloccante)
**Action:** USER
**Files:**
- `docs/reference/ispirazione/` (privata, fuori da git)
- `docs/design/README.md` (risposte alle domande)

**Details:**
Prima di qualsiasi altro task, controlla se esistono i file elencati sotto. Se mancano, **fermati** e scrivi all'utente queste istruzioni, poi aspetta la conferma:

> Per dare a SagomaCad un aspetto suo mi servono alcune immagini e tre risposte.
> 1. Crea la cartella `docs/reference/ispirazione/` (resta fuori da git).
> 2. Salva lì 2-4 screenshot di programmi di cui ti piace l'aspetto (non per forza CAD: per esempio Bambu Studio, Figma, Linear, Blender). Chiamali `ispirazione-01.png`, `ispirazione-02.png` e così via.
> 3. Per ognuno scrivi un file con lo stesso nome e `.md` con una o due righe: cosa ti piace (colori, densità, forma dei pannelli, icone).
> 4. Rispondimi a tre domande: tema predefinito chiaro o scuro? Colore d'accento preferito (blu, verde, arancio, altro)? Interfaccia più compatta come Fusion o più ariosa?

Le risposte vanno in `docs/design/README.md`, sezione "Scelte dell'utente", con la data.

**Validation:**
```bash
ls docs/reference/ispirazione/*.png && grep -q "Scelte dell'utente" docs/design/README.md
```

### Task 9.1: Struttura dei riferimenti
**Action:** CREATE
**Files:**
- `docs/design/README.md`
- `docs/design/approvate/.gitkeep`
- `docs/reference/README.md` (locale, ignorato)
- `docs/ui-design.md`

**Details:**
`docs/design/README.md` spiega: token (dove stanno, come si cambiano), icone (origine SolidCraft, ruoli, accenti), schermate approvate (solo l'utente le approva, una per area: ribbon, browser, timeline, dialog, viewport). `docs/reference/README.md` ricorda che gli screenshot servono solo per il comportamento e non escono mai dalla cartella. Verifica che `docs/reference/` sia nel `.gitignore`. Aggiorna `docs/ui-design.md` con la nuova regola comportamento/aspetto.

**Validation:**
```bash
git check-ignore docs/reference/README.md && test -f docs/design/README.md
```

### Task 9.2: Import dei master SVG di SolidCraft
**Action:** CREATE
**Files:**
- `assets/icons/solidcraft/svg/**`
- `assets/icons/solidcraft/palette.toml`, `assets/icons/solidcraft/aliases.toml`
- `assets/icons/solidcraft/LICENSE-MIT`, `assets/icons/solidcraft/LICENSE-APACHE`
- `ATTRIBUTION.md`, `tools/import_solidcraft_icons.py`

**Details:**
Script ripetibile che copia da `../solidcraft-ref` i master, la palette e gli alias, con le due licenze. Una riga in `ATTRIBUTION.md` per cartella (percorso, autore "SolidCraft contributors", sorgente con commit, licenza MIT OR Apache-2.0). Non modificare gli SVG: i colori si cambiano solo in palette.

**Validation:**
```bash
python tools/import_solidcraft_icons.py --check && grep -q "assets/icons/solidcraft" ATTRIBUTION.md
```

### Task 9.3: Renderer delle icone per ruoli
**Action:** CREATE
**Files:**
- `src/ui/icons.hpp`, `src/ui/icons.cpp`
- `tools/palette_to_json.py` (palette.toml e aliases.toml in JSON al momento della build)
- `tests/icons.cpp`, `CMakeLists.txt`

**Details:**
Traduzione di `icon_svg.rs`: ogni master usa esadecimali segnaposto per i ruoli (ink, line/currentColor, passive-1..3, active-1..3, guide, gap). Prima di passare l'SVG a lunasvg sostituisci ogni segnaposto con il valore del tema (chiaro o scuro) e, per i ruoli `active-*`, con la rampa dell'accento della categoria. `gap` cancella fino alla trasparenza, non dipinge lo sfondo. Rasterizza alla dimensione esatta in pixel (scala DPI inclusa) e metti in cache la texture OpenGL per (id, accento, tema, pixel). Un master che non si legge disegna un segnaposto e scrive un errore, mai crash.

**Validation:**
```bash
cmake --build build --config Release && ctest --test-dir build -R icons --output-on-failure
```
Il test deve verificare: tutti i master si leggono, la sostituzione dei ruoli cambia i pixel tra chiaro e scuro, la cache restituisce la stessa texture alla seconda richiesta.

### Task 9.4: Mappa comandi, icone e accenti
**Action:** MODIFY
**Files:**
- `src/ui/toolbar_catalog.cpp`, `src/ui/toolbar_catalog.hpp`
- `tools/update_parity.py`, `docs/parity.md`

**Details:**
Ogni voce del catalogo indica l'id dell'icona SolidCraft (per esempio extrude, revolve, hole, fillet, chamfer, box, cylinder, rect, circle, line, c_parallel) e l'accento della categoria: Solido e Costruisci blu, Schizzo verde, Vincoli rosso, Costruzione arancio, Mesh giallo. Dove SolidCraft non ha l'icona usa Lucide e segnala la voce. `docs/parity.md` guadagna la colonna Icona.

**Validation:**
```bash
python tools/update_parity.py --check
```

### Task 9.5: Token del tema ampliati
**Action:** MODIFY
**Files:**
- `src/ui/theme.hpp`
- `tests/theme_tokens.cpp`

**Details:**
Aggiungi, nei due temi, i token per tutti gli stati che oggi mancano. Prendi l'elenco da `palettes.rs` di SolidCraft, ma con **valori nostri** coerenti con le scelte del task 9.0: barra applicazione, ribbon, pannelli, testo e testo secondario, accento, hover; faccia selezionata, spigolo selezionato e bordo, spigolo sotto il mouse con alone, schiarimento della faccia sotto il mouse, profilo selezionato e sotto il mouse; piani dell'origine con bordo e hover; anteprima unione, nuovo corpo, taglio, schizzo; corpo e spigolo del corpo; gradiente del viewport (alto e basso), griglia e griglia principale; quote e quote guidate; geometria proiettata, persa, di costruzione; simboli dei vincoli; riquadro di selezione finestra e incrocio; elementi della timeline normale ed errore, marker. Test: nessun colore letterale fuori da `theme.hpp` (scansione dei sorgenti) e contrasto minimo testo/fondo nei due temi.

**Validation:**
```bash
ctest --test-dir build -R theme_tokens --output-on-failure
```

### Task 9.6: Fabbrica unica delle finestre
**Action:** CREATE
**Files:**
- `src/ui/frame.hpp`, `src/ui/frame.cpp`
- `apps/common/*.cpp` (le 13 chiamate a `ImGui::Begin`)
- `tests/frame_rule.py`

**Details:**
Come `frame.rs`: dialog agganciati al bordo del viewport (larghezza minima e massima dai token), finestre flottanti centrate, titoli di sezione su una fascia più scura. Tutte le finestre passano da qui. Il test fallisce se un altro file chiama `ImGui::Begin` o se un dialog supera i limiti.

**Validation:**
```bash
python tests/frame_rule.py && ctest --test-dir build --output-on-failure
```

### Task 9.7: Tavola di design e approvazione (bloccante)
**Action:** USER
**Files:**
- `apps/sagomacad-cli/` (comando `design-board`)
- `docs/design/tavola-light.png`, `docs/design/tavola-dark.png`
- `docs/design/approvate/tavola-*.png`

**Details:**
Nuovo comando `sagomacad-cli design-board --out docs/design/tavola-<tema>.png` che disegna: campioni di tutti i token con il nome, tutte le icone del catalogo a 16, 24 e 32 px con il loro accento, un ribbon, una riga del browser, la timeline e un dialog di esempio. Generala nei due temi, guardala, correggi. Poi **fermati** e chiedi all'utente:

> Ho preparato la tavola di design in `docs/design/tavola-light.png` e `docs/design/tavola-dark.png`. Aprile e dimmi se approvi, oppure cosa cambiare (colori, accento, dimensioni delle icone, densità).

Solo dopo l'approvazione copia le due tavole in `docs/design/approvate/` e annota la data in `docs/design/README.md`.

**Validation:**
```bash
test -f docs/design/approvate/tavola-light.png && test -f docs/design/approvate/tavola-dark.png
```

### Task 9.8: Icone applicate a ribbon, browser, timeline e menu
**Action:** MODIFY
**Files:**
- `apps/common/shell.cpp`, `apps/common/timeline_widget.cpp`, `src/ui/widgets.cpp`

**Details:**
Ribbon (icone 26 px), tendine e palette dei comandi (16 px), browser (16 px), timeline (20 px) usano il renderer del task 9.3. Voci disabilitate con i ruoli spenti, non con un filtro grigio. Screenshot a 1440x900 e 1280x720 nei due temi, confrontati con `docs/design/approvate/`.

**Validation:**
```bash
sagomacad-cli snapshot --size 1440x900 --scale 2 --out build/check-ribbon.png
```

### Task 9.9: Documentazione
**Action:** MODIFY
**Files:**
- `docs/ui-design.md`, `docs/design/README.md`

**Details:**
Come si aggiunge un'icona (master in SolidCraft o nuovo master nostro con le stesse regole), come si cambia un colore (solo palette o token), come si approva una schermata.

**Validation:**
```bash
python tests/test_utf8.py
```

### Task 9.10: Verifica della fase
**Action:** VERIFY
**Details:** Build Release, ctest completo, controllo dei livelli, screenshot nei due temi e due dimensioni confrontati con la tavola approvata. Tavola approvata presente in `docs/design/approvate/`. Se qualcosa fallisce, aggiungi task di correzione a questa fase.

**Validation:**
```bash
cmake --build build --config Release && ctest --test-dir build --output-on-failure && python tools/check_layers.py
```
