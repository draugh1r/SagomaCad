# Verifica LAYOUT — 16 ottobre 2026

Queste immagini sono evidenze di verifica, **non golden approvate**. Le immagini
precedenti del repository non sono state sostituite. I file `before-*` sono stati
catturati prima della migrazione dal binario della versione iniziale del branch;
`after-*` dalla build Release `build/agent-layout` con Clay.

## Confronti affiancati

Sinistra: dockspace ImGui precedente. Destra: layout Clay.

| Tema e dimensione | Confronto | Pixel differenti |
|---|---|---|
| Chiaro 1280×720 | [Prima/dopo](compare-light-1280x720.png) | 7 960 (0,8637%) |
| Chiaro 1440×900 | [Prima/dopo](compare-light-1440x900.png) | 8 474 (0,6539%) |
| Scuro 1280×720 | [Prima/dopo](compare-dark-1280x720.png) | 3 374 (0,3661%) |
| Scuro 1440×900 | [Prima/dopo](compare-dark-1440x900.png) | 2 888 (0,2228%) |

Il confronto conta un pixel come diverso se cambia almeno un canale RGB,
senza tolleranza. Dati in [comparison.json](comparison.json). Rimangono equivalenti
posizioni dei pannelli, griglia, assi, camera, cubo, navigazione e timeline.
Le differenze sono la freccia di compressione aggiunta al browser, i bordi delle
finestre senza docking e separatori/etichette dei gruppi del ribbon spostati al
massimo di un pixel dal calcolo Clay e dagli arrotondamenti del testo. Nel contenuto
del viewport, ritaglio `(255,175,width-10,height-74)`, cambia un solo pixel di
antialiasing del cubo, di 1–6 livelli RGB: `(1182,245)` a 1280×720 e `(1342,245)`
a 1440×900. Nessuna golden aggiornata per assorbire queste differenze.

Guardati tutti e quattro i confronti, lo snapshot a scala 2 in
`build/agent-layout/scale2-dark.png`, il ridimensionamento e il browser compresso.
Confrontati anche i riferimenti locali Fusion in `docs/reference/fusion/`, incluse
le note del cubo e la modalità schizzo. Gli strumenti CAD esistenti sono invariati;
i test di sketch, cubo, timeline e navigazione restano verdi.

## Separatori, compressione e riavvio

- [Trascinamento prima/dopo](compare-resize.png): input mouse reali portano il
  browser da 243 a 298, proprietà da 253 a 288 e timeline da 62 a 106.
- [Browser compresso](browser-collapsed.png): larghezza 24, espansione ripristina 298.
  Il test comprime ed espande anche le proprietà conservandone la larghezza.
- [Riavvio affiancato](compare-restart.png): le tre misure sopravvivono alla
  distruzione e ricreazione della shell, caricando lo stesso file JSON separato.
  `before-restart.png` e `after-restart.png` sono identici pixel per pixel.
- Doppio clic su ciascuno dei tre separatori ripristina la misura predefinita.
- [Finestra 800×600](small-800x600.png): entrambi i pannelli visibili, viewport
  esterno largo 320; nessuna sovrapposizione. Rettangoli e diagnostica salvati
  in [inspect-800x600.json](inspect-800x600.json).

## Verifica riproducibile

```powershell
cmake --build build/agent-layout --config Release
ctest --test-dir build/agent-layout -C Release --output-on-failure
python tools/check_layers.py
build/agent-layout/apps/sagomacad-cli/sagomacad-cli.exe fuzz-commands
build/agent-layout/Release/layout_interaction_test.exe build/agent-layout/layout-evidence
build/agent-layout/apps/sagomacad-cli/sagomacad-cli.exe snapshot --size 1280x720 --theme dark --out build/agent-layout/check-dark.png
```

Il test di interazione rigenera le immagini in una cartella di build e usa
preferenze proprie. Non tocca preferenze personali, documenti o golden.
`layout` verifica 384 combinazioni, inclusi 1280×720, 1920×1080, 800×600,
320×240, 100×60 e 0×0, entrambe le scale e tutte le combinazioni di pannelli.
Controlla anche recupero dopo input non finito, errore reale di capacità Clay,
contesti multipli e serializzazione con dati ostili. Tutte le aree e i separatori
restano non negativi e disgiunti; da 800×600 il viewport conserva almeno 320×200.

Risultati: build Release MSVC 19.44, CTest **34/34**, livelli e UTF-8 verdi;
fuzz **188 casi diretti + 162 MCP**. Implementazione Clay e dichiarazioni del layout
compilate C++20 anche con GCC 13.3 e Clang 18.1 in WSL; la suite completa è stata
eseguita su Windows, senza avviare CI remota.
