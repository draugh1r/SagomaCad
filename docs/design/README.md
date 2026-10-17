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
