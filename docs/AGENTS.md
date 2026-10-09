# AGENTS.md - guida per agenti AI e contributori

`SagomaCad` è un CAD parametrico desktop in **C++20**: il flusso di Fusion 360 (schizzi con vincoli, feature, timeline) con l'immediatezza di Tinkercad, file in locale, pensato per la stampa 3D. Kernel geometrico OpenCascade, interfaccia Dear ImGui su SDL3 e OpenGL. Leggi questo file per primo, poi `PLAN.md` nella stessa cartella e il resto di `docs/`.

Non aggiungere Tauri, Electron, webview o framework JavaScript. Non aggiungere Qt. Una sola interfaccia: Dear ImGui.

## 1. Orientamento

| Leggi | Perché |
|---|---|
| `PLAN.md` | Obiettivi, architettura, comandi, traguardi e traguardo corrente |
| `docs/architecture.md` | Mappa dei moduli, livelli, confine motore/UI |
| `docs/control-protocol.md` | Canale di controllo JSON: come pilotare e fotografare l'app |
| `docs/ui-design.md` | Token del tema, widget condivisi, layout |
| `log/devlog.md` | Cosa è stato fatto e cosa resta aperto |

## 2. Livelli (imposti)

```text
L0 core/   L1 doc/   L2 solver/ kernel/   L3 regen/ mesh/   L4 engine/   L5 automation/   L6 ui/
apps/sagomacad   apps/sagomacad-cli
```

- Un modulo dipende solo da livelli inferiori. Lo verifica `python tools/check_layers.py`.
- Solo `kernel/` include header OpenCascade.
- Sotto `ui/` nessuno include ImGui, SDL o OpenGL.
- `doc/` contiene solo dati serializzabili, mai puntatori a oggetti OCCT.

## 3. Regole d'oro

1. **Tutto è un comando.** Un nuovo comportamento visibile all'utente è un comando in `engine/cmds/<area>_cmds.cpp` (id, etichetta, menu, scorciatoia, documentazione dei parametri, `enabled`, `run`) registrato nel registro, con test. UI, CLI e canale di controllo chiamano i comandi per id. Solo lo stato di vista (zoom, orbita, pannelli, tema) appartiene alla shell UI.
2. **La UI è sottile.** La UI legge view-model e overlay prodotti dal motore e inoltra input. Non calcola geometria. Lo stato della UI vive in `ui/state.hpp` ed è serializzabile.
3. **Gli strumenti producono dati.** Gli strumenti (`engine/tools/`) ricevono eventi del puntatore e tastiera e restituiscono comandi e `OverlayPrim`. Non conoscono ImGui.
4. **Colori e misure dai token.** Mai colori, raggi o spaziature scritti a mano nei widget: tutto da `ui/theme.hpp`. Widget condivisi in `ui/widgets.cpp`.
5. **Mai crashare.**
   - Nessuna eccezione OCCT esce da `kernel/`: cattura `Standard_Failure` e controlla `IsDone()`, restituisci `Result<T>`.
   - `run` di un comando valida tutti i parametri e restituisce errore per qualsiasi input. Nessun `assert` o `abort` su input esterni.
   - Indici, dimensioni e numeri da file o parametri sono ostili: controlla i limiti, evita divisioni per zero e NaN, limita le allocazioni.
   - Una feature che fallisce va in errore nella timeline, la regen continua.
   - Ogni correzione di un crash arriva con un test che crashava prima della correzione.
6. **Asset puliti.** Si replicano 1:1 layout, flusso di lavoro, scorciatoie e comportamento di Fusion 360, con le aggiunte in stile Tinkercad indicate nel piano; mai icone, grafica, shader o codice proprietari. Icone Lucide (ISC), font Inter e JetBrains Mono (OFL). Ogni asset esterno va in `ATTRIBUTION.md` con licenza.
7. **Verifica visiva obbligatoria.** Prima di ogni lavoro sulla UI, guarda le immagini e le note in `docs/reference/fusion/`. Ogni modifica alla UI va controllata con uno screenshot prima di considerarla finita:
   ```sh
   sagomacad-cli snapshot --size 1440x900 --scale 2 --out /tmp/check.png --script '[[method, params], ...]'
   ```
   oppure avviando l'app con `--control` e chiamando `ui.screenshot`. Guarda il PNG e confronta lo screenshot del risultato con il riferimento in `docs/reference/fusion/`. Controlla almeno due dimensioni di finestra e il tema chiaro e scuro.
8. **Il kernel si verifica coi numeri.** Ogni feature geometrica ha test su volume, area, bounding box o numero di facce, calcolati con `GProp`.
9. **I test sono il cancello.** Ogni modifica arriva con test. Le golden image si aggiornano solo con approvazione dell'utente.
10. **Accesso per agenti AI.** Da M1.5 in poi ogni nuovo comando o informazione sul modello deve essere raggiungibile sia dal client CLI sia via MCP, con un test.

## 4. Prima di chiudere un task

```sh
cmake --build build --config Release
ctest --test-dir build --output-on-failure
python tools/check_layers.py
./build/apps/sagomacad-cli/sagomacad-cli fuzz-commands     # se hai aggiunto o cambiato comandi
./build/apps/sagomacad-cli/sagomacad-cli snapshot ...      # se hai toccato la UI: guarda il PNG
```

Poi aggiungi una voce breve in `log/devlog.md`: cosa è stato fatto, numeri misurati, cosa resta aperto. Le sessioni possono interrompersi in ogni momento: il devlog e un albero che compila sono il modo in cui il prossimo agente riparte. Lascia la build verde a ogni passo.

## 5. Scelta del lavoro

1. Il traguardo corrente in `PLAN.md` (sezione 12). Non iniziare il traguardo successivo senza richiesta.
2. Le voci "resta aperto" più recenti in `log/devlog.md`.
3. Prima l'infrastruttura (comandi, test, canale di controllo), poi le funzioni, poi la rifinitura.

## 6. Agenti in parallelo

- Usa una cartella di build tua (`build/agent-<nome>`).
- Nuovi comandi in un file nuovo `engine/cmds/<area>_cmds.cpp` con una funzione `specs()`, invece di far crescere file condivisi.
- File condivisi (registro comandi, `ui/state.hpp`, `CMakeLists.txt` principale): modifiche piccole e mirate, rileggi prima di modificare.
- Se la build è rotta da una modifica altrui in corso, aspetta e riprova: non correggere file di altri.
