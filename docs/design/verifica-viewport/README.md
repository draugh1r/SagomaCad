# Verifica del viewport GPU - 2026-10-19

Build Release Windows in `build/agent-phase10`. Tavole approvate light/dark presenti e immutate. Comportamento confrontato con i riferimenti Fusion 17/18/19: spigoli visibili e continui in vista isometrica e radente; aspetto confrontato con le tavole approvate, usando i token della shell.

Le immagini qui archiviate mostrano tre angoli nei due temi: 1440x900 per il chiaro, 1280x720 per lo scuro. Tutte le dodici combinazioni di angolo/tema/dimensione sono state generate in `build/phase10-final`. I JSON accanto alle immagini ricreano il pezzo: scatola 40x30x10, quattro raccordi verticali R2, cilindro R8 alto 15.

```powershell
build/agent-phase10/apps/sagomacad-cli/sagomacad-cli.exe snapshot --size 1440x900 --theme light --out build/verifica.png --script docs/design/verifica-viewport/iso.json
```

| Angolo | Chiaro | Scuro |
|---|---|---|
| Isometrico | [immagine](iso-light.png) | [immagine](iso-dark.png) |
| Radente | [immagine](radente-light.png) | [immagine](radente-dark.png) |
| Dal basso | [immagine](basso-light.png) | [immagine](basso-dark.png) |

Spigoli netti, contorno del cilindro continuo e facce nascoste occluse. La verifica dal basso ha scoperto e corretto un difetto: la griglia copriva le pareti sopra il proprio piano. La nuova regressione controlla i pixel di entrambe le pareti, sopra e sotto il piano, nei due temi; il backdrop ora colora solo i campioni di profondita ancora vuoti.

[Selezione della lente](profili-lente-selezionata.png) e [lente estrusa dopo modifica del raggio](profili-lente-estrusa.png). Lo scenario `profili-intersecanti` seleziona ed estrude separatamente tutte e tre le regioni, cambia R10 in R11 e controlla volume e identita. Per due cerchi R10 a distanza 10 mm: area lente 122.8369699 mm2, ciascuna lunetta 191.3222955 mm2. Test anche sui piani XY/XZ/YZ, supporto ruotato, linea/cerchio, arco/linea e tre cerchi (sette regioni); riferimenti scomparsi producono errori leggibili nella timeline.

Validazione finale: **87/87 ctest in 75.76 s**, inclusi **13 file JSON** e i test di input del runner, frame, UTF-8 e parita. Livelli validi; fuzz 278 casi/45 comandi e 237 casi MCP. Pytest render_view e handles: 1/1 ciascuno. Benchmark renderer su RTX 3070 Ti: 200000 triangoli, 1440x900, MSAA4, 120 frame di orbita con glFinish, un upload; 3263.51 fps medi, 2113.72 minimi, p95 2609.60. Sono tempi della pipeline del renderer, senza il limite di frame della finestra.

Log della suite: `build/phase10-tests.log`; output dettagliato in `build/agent-phase10/Testing/Temporary/LastTest.log`. Nessuna golden image aggiornata.
