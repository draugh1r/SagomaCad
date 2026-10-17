# Design di SagomaCad

## Scelte dell'utente

2026-10-17, indicazioni esplicite dell’utente:
- Riferimento visivo: SolidCraft in `../solidcraft-ref`; nessuno screenshot di ispirazione richiesto.
- Tema predefinito: chiaro.
- Accento principale: verde pistacchio.
- Densità: compatta come Fusion, con aspetto e asset propri.

La scelta sostituisce il requisito degli screenshot del task 9.0.

## Token e icone

I token C++ sono in `src/ui/theme.hpp`: colori, misure e stati dei widget si cambiano qui. Le icone provengono dai master SVG SolidCraft (MIT OR Apache-2.0), conservati senza modifiche in `assets/icons/solidcraft/svg/`. La palette e gli accenti per ruoli sono in `assets/icons/solidcraft/palette.toml`, gli alias in `aliases.toml`. Lucide resta disponibile per la navigazione e per le operazioni senza un master equivalente.

## Riferimenti e approvazione

`docs/reference/` è privata e ignorata da Git; contiene soltanto riferimenti di comportamento. Per questa fase il riferimento visivo è SolidCraft, come richiesto dall’utente. Le schermate proprie di SagomaCad e le tavole sono in `docs/design/`. Solo l’utente può approvarle: dopo conferma esplicita si copiano in `docs/design/approvate/` annotando la data. Aree: ribbon, browser, timeline, dialog e viewport. Le tavole generate non valgono automaticamente come golden image.

## Tavole di design

Generazione locale con la stessa shell SDL/OpenGL e lo stesso renderer SVG:

```powershell
build/agent-phase9/apps/sagomacad-cli/sagomacad-cli.exe design-board --theme light --out docs/design/tavola-light.png
build/agent-phase9/apps/sagomacad-cli/sagomacad-cli.exe design-board --theme dark --out docs/design/tavola-dark.png
```

Le tavole contengono tutti i 77 colori registrati in `theme_colors`, i 37 comandi unici del catalogo alle dimensioni 16/24/32 px, ruoli disabilitati e campioni di ribbon, browser, timeline e dialog. Layout delle aree dichiarato in Clay (`LayoutEngine::design_board`). Dimensione predefinita 1600×1800; scala DPI 1 o 2.

Accento UI: `#93c572` nel tema chiaro, `#b1ce8a` nello scuro, con testo scuro sui pulsanti primari. Gli accenti delle icone rimangono semantici: Solido blu, Schizzo verde, Vincoli rosso, Costruzione arancio e Mesh giallo. I due fallback Lucide sono Salva con nome e Scorri selezione. I master SVG e le palette importate rimangono identici al commit indicato in `assets/icons/solidcraft/SOURCE`.

L’approvazione riguarda colori, densità, dimensioni e campioni. La copia in `approvate/` avviene soltanto dopo la conferma esplicita dell’utente; il passaggio successivo applica le icone a ribbon, browser, timeline e menu.

Via MCP: `design_board({"theme":"light"})` o `design_board({"theme":"dark"})` restituisce direttamente un’immagine PNG e i metadati, senza modificare il modello.
