# Verifica visiva M3

I quattro screenshot principali sono in `docs/screenshots/`:

- `m3-plane-selection.png`: piani semitrasparenti, piano XY evidenziato e suggerimento al cursore;
- `m3-sketch-mode.png`: allineamento TOP, scheda contestuale e tavolozza;
- `m3-live-dimensions.png`: rettangolo in anteprima dopo `40 Tab 20`, prima di Invio;
- `m3-closed-profile.png`: rettangolo nero 40×20, profilo azzurro e zero gradi di libertà.

Questi sono in tema chiaro a 1440×900. Le sottocartelle `light-1280x720`, `dark-1440x900`, `dark-1280x720` contengono le stesse fasi negli altri formati e temi. Nessuna golden image M0 è stata sostituita.

Confronto con `docs/reference/fusion/selezione-piano.png`, `modalita-schizzo.png`, `quote-durante-disegno.png`, `profilo-chiuso.png`: piani selezionabili nell'origine, camera frontale al piano, scheda Schizzo, palette a destra, misure vicine alla geometria e riempimento dei profili. Corrette densità della griglia, icona Fine schizzo, suggerimenti che entravano sotto la tavolozza e cattura incompleta del front buffer delle finestre nascoste. A 720 pixel la tavolozza scorre e il pulsante Fine schizzo resta accessibile nel piè di pagina; nel tema scuro le geometrie vincolate usano il colore chiaro del testo per rimanere visibili.

Rigenerazione tramite eventi reali della UI:

```powershell
& .\build\agent-m3\Release\sketch_interaction_test.exe docs/screenshots
& .\build\agent-m3\Release\sketch_interaction_test.exe docs/screenshots/m3/light-1280x720 1280x720 light
& .\build\agent-m3\Release\sketch_interaction_test.exe docs/screenshots/m3/dark-1440x900 1440x900 dark
& .\build\agent-m3\Release\sketch_interaction_test.exe docs/screenshots/m3/dark-1280x720 1280x720 dark
```
