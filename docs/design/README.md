# Design di SagomaCad

## Scelte dell'utente

2026-10-17, indicazioni esplicite dell’utente:
- Riferimento visivo: SolidCraft in `../solidcraft-ref`; nessuno screenshot di ispirazione richiesto.
- Tema predefinito: chiaro, con sfondi bianchi (#ffffff) in tutta l’app, inclusi ribbon, pannelli, intestazioni, dialog e viewport (precisazione dell’utente del 2026-10-17).
- Accento principale: verde pistacchio, riservato agli accenti e agli stati di interazione; nessuna tinta verde negli sfondi ordinari. Testi e bordi neutri.
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

## Approvazione

2026-10-17: l’utente conferma «ok ci siamo» dopo aver verificato l’app con sfondi bianchi e accento pistacchio. Tavole chiara e scura archiviate in `approvate/` come riferimento della fase 9.

## Aggiungere un’icona o cambiare un colore

1. Cercare prima un master o un alias in `assets/icons/solidcraft/`. Per aggiornare il riferimento, usare `tools/import_solidcraft_icons.py --source ../solidcraft-ref` e registrare il commit e le licenze in `ATTRIBUTION.md`; controllare con `--check`. I master importati restano identici alla sorgente.
2. Per un master originale, creare `assets/icons/sagomacad/svg/model/sagoma_nome.svg` (eventuale variante ottica in `small/`). Griglia 64×64, stessa proiezione e spessori SolidCraft; usare soltanto gli identificatori dei ruoli di `palette.toml`. Il compilatore include questa cartella separata e rifiuta nomi che sovrascrivono un master o alias importato. Nessun asset commerciale.
3. Associare ID e accento in `src/ui/toolbar_catalog.cpp`. `icon` identifica il fallback Lucide, `design_icon` il master per ruoli. Usare un `design_icon` vuoto soltanto per un fallback esplicito. Aggiornare `docs/parity.md` con `python tools/update_parity.py`, poi verificarlo con `--check`.
4. Cambiare i colori dell’interfaccia solo in `src/ui/theme.hpp`; quelli delle icone soltanto nella palette. Una modifica locale alla palette importata va dichiarata e distinta dall’import originale nel controllo di provenienza. Non ricolorare i master SVG. Nel tema chiaro gli sfondi ordinari rimangono bianchi: verde solo per accenti e stati.
5. Ricompilare Release e verificare `icons`, `icon_palette`, `theme_tokens` e `frame_rule`; poi generare una tavola e screenshot nei due temi e alle due dimensioni. Il renderer risolve i ruoli e rasterizza ai pixel effettivi, DPI incluso; cache per ID/accento/tema/pixel/stato disabilitato. `gap` è un ritaglio trasparente, non una tinta di sfondo.

Solo l’utente approva un’immagine: presentare la proposta, attendere la conferma, copiare esattamente il PNG in `docs/design/approvate/` e annotare data e ambito. Non rigenerare o sovrascrivere schermate approvate dopo una modifica dei token senza nuova approvazione.
