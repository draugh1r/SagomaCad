# UI M0

## Modellazione e timeline (M4)

`E` apre Estrudi; Rivoluzione è nel gruppo Crea. Prima o dopo l'apertura si
selezionano le regioni nel viewport: corona e disco dello stesso schizzo sono
selezioni distinte. Il pannello destro è il nodo flottante `contextual` dell'albero
Clay, ancorato al viewport sotto il cubo. Le righe seguono i riferimenti Fusion
07–09; OK e Annulla rimangono nel piè di pagina mentre i campi possono scorrere.
Tipo solido, piano iniziale e sformo nullo sono costanti dell'estrusione disponibile.

Le regioni e le mesh di anteprima sono view-model prodotti dal motore. La UI
proietta le maniglie, inoltra selezioni e aggiorna l'espressione durante il drag;
non modifica il documento fino a OK. La distanza è modificabile anche nella
casella accanto alla freccia. L'anteprima è blu per Nuovo corpo e Unisci, rossa per
Taglia. Operazione elenca Unisci, Taglia, Interseca, Nuovo corpo con icone Lucide.
Il campo Corpi permette di scegliere più destinazioni. Rivoluzione attiva la
selezione dell'asse facendo clic sul campo Asse, poi su un asse dell'origine o
una linea dello schizzo; asse e maniglia angolare sono evidenziati nel viewport.

La timeline usa icone Lucide da 20 px per i diversi tipi e un marker verticale
trascinabile fra le feature. I passi dopo il marker sono attenuati e non applicati;
i controlli a sinistra spostano o riproducono il marker. Un drag è un passo di undo.
Doppio clic riapre schizzo, primitiva, estrusione o rivoluzione. La modifica mostra
il modello al passo interessato; la conferma ricalcola i successivi. Gli errori
sono rossi con messaggio nel tooltip. Gli schizzi consumati si nascondono e tornano
visibili durante la modifica o prima della feature che li usa.

`ui.inspect` espone `model` (parametri, selettore asse, errore, maniglia, casella e
rettangoli dei campi), `timeline_items` e `timeline_marker`, oltre ai rettangoli
Clay. Evidenze e confronti: `docs/screenshots/m4-confronto/`.

I token di colore, spaziatura, dimensione e delle etichette di sezione sono in `src/ui/theme.hpp`, per i temi `light` (predefinito) e `dark`. La shell SagomaCad mostra un ribbon con schede e gruppi, browser, viewport e timeline; il pannello Proprietà compare solo quando serve. Il catalogo in `src/ui/toolbar_catalog.cpp` associa ogni voce a un ID di comando del piano e a un'icona Lucide. Il menu File accanto al documento contiene Nuovo, Apri, Salva e Salva con nome; annulla e ripeti restano accanto. Ciascun gruppo del ribbon mostra due o tre comandi e la tendina elenca tutti quelli del gruppo. In M0 nessun comando è registrato: tutte le voci sono disabilitate con tooltip «Non ancora disponibile». La scheda Schizzo compare solo in modalità schizzo; Superficie, Lamiera e Plastica non compaiono. La parità è documentata in `docs/parity.md`.

Nel viewport la griglia isometrica, gli assi, l'indicatore di vista e la barra di navigazione sono disegnati con Dear ImGui e OpenGL. Le icone della barra di navigazione e i controlli della timeline sono statici in M0. I pannelli occupano rettangoli Clay in finestre ImGui fisse senza decorazioni; i titoli delle sezioni sono etichette interne. Il viewport non ha titolo. Inter usa l'intervallo U+0020–U+024F, che comprende Latin-1 e Latin Extended A/B.

## Cambia parametri (M1)

Il pannello mostra Nome, Unità, Espressione, Valore e Commento. `+` crea una riga; un clic in una cella apre la modifica diretta. Invio conferma tramite `param.set`, Esc annulla. Il valore viene ricalcolato nell'anteprima mentre si modifica l'espressione. Il campo Espressione accetta anche `nome = espressione` se Nome è vuoto o coincide. Errori del comando e dell'anteprima compaiono in rosso sotto la cella e nel tooltip. Testo e colori delle intestazioni e degli errori provengono da `src/ui/theme.hpp` per i temi chiaro e scuro.

## Cubo di navigazione

`src/ui/view_cube.cpp` mantiene un cubo normale a spigoli vivi e 26 zone di selezione sulle sue facce. Le fasce degli spigoli e le zone dei vertici compaiono solo come evidenziazioni piatte al passaggio del mouse; non modificano la geometria del cubo. `apps/common/view_cube_widget.cpp` disegna cubo, etichette proiettate sulle facce, assi colorati ed evidenziazione. Geometria, colori, misure ed etichette sono guidati dai token `ViewCubeTokens` in `src/ui/theme.hpp`.

Un clic su faccia, spigolo o vertice allinea la visuale; il trascinamento sinistro sul cubo ruota la camera mantenendo l'asse verticale del mondo e senza accumulare roll. Home torna alla vista isometrica e adatta la scena alla finestra. Le viste allineate mostrano quattro frecce per passare alla faccia adiacente e due frecce circolari per ruotare esplicitamente di 90°. La camera conserva una base coerente nelle viste Alto e Basso; l'orbita si ferma ai poli senza ribaltare l'orizzonte. Il cubo intercetta il mouse senza attivare contemporaneamente Pan o Orbita della barra.

Le evidenziazioni di spigoli e vertici usano `highlight_band`, mentre la selezione usa una fascia più ampia `hit_band` e un margine esterno `hit_padding`. Le dimensioni del testo sono indipendenti dalle zone di selezione. Ogni clic sul cubo, Home compresa, avvia una transizione di 0,85 secondi con accelerazione e decelerazione graduali. Orientamento, centro della vista e zoom vengono interpolati; gli angoli seguono il percorso più breve. Un nuovo clic riparte dalla posizione corrente, mentre trascinamento e zoom interrompono l'animazione e restituiscono immediatamente il controllo all'utente.
Un'espressione numerica senza suffisso, per esempio `40`, assume l'unità della colonna Unità (`mm` → 40 mm, `in` → 1 in). Le espressioni già dimensionali conservano la propria unità e vengono convertite per mostrare il valore nella colonna.

## Layout della shell (LAYOUT)

`src/ui/layout.cpp` dichiara questo albero, con ID stabili:

```text
shell (colonna)
  topbar                  nome documento, menu File, azioni, schede
  ribbon (riga)           gruppi dal catalogo della scheda attiva
    group[i] (colonna)    riga dei pulsanti, spazio, menu/etichetta
  header_separator
  content (riga)
    browser, browser_separator
    viewport (grow)
    properties_separator, properties
    contextual           larghezza zero, riservata per aree future
  timeline_separator, timeline
```

A ogni frame la shell aggiorna lo stato contestuale e chiama `LayoutEngine::compute`.
Clay risolve l’albero; `Clay_GetElementData` trasferisce i rettangoli in `ShellLayout`.
I pannelli aprono finestre ImGui senza decorazioni, senza spostamento o dimensionamento
nativo, sui rispettivi rettangoli. Il viewport mantiene il rendering e i widget
esistenti nel contenuto del suo rettangolo, sottratto il padding del tema.
`ui/widgets.cpp` disegna i pulsanti del ribbon e richiama gli stessi ID di comando.
Il renderer Clay traduce rettangoli, bordi, testo e clip su `ImDrawList`; non gestisce
input. Il testo viene misurato con `ImFont::CalcTextSizeA` del font ImGui caricato,
alla dimensione scalata usata dal renderer. Token e DPI vengono applicati una sola volta.

L’arena viene allocata all’avvio: massimo 512 elementi e 1024 parole nella cache di
misura. Un errore Clay viene registrato su stderr e in `layout_error`; rettangoli e
comandi decorativi dell’ultimo frame valido restano disponibili. I comandi e le loro
stringhe vengono copiati: non dipendono dalla memoria temporanea del frame fallito.

`State` conserva `browser_width`, `properties_width`, `timeline_height`, visibilità
e flag `*_collapsed`, in unità del tema prima della scala. Trascinamento e doppio clic
sui separatori modificano solo questo stato; il doppio clic ripristina i valori del
tema. Le frecce nelle intestazioni comprimono ed espandono i pannelli conservandone
la larghezza. Il viewport assorbe lo spazio rimanente: a 800×600 resta almeno 320×200
unità del tema, anche con entrambi i pannelli alle dimensioni massime. Per finestre
più piccole i pannelli si riducono, il ribbon viene omesso quando non può entrare,
e nessun rettangolo della shell si sovrappone o diventa negativo.

L’app desktop carica `layout.json` nella cartella preferenze restituita da
`SDL_GetPrefPath("SagomaCad", "SagomaCad")` (su Windows, `%APPDATA%/SagomaCad/SagomaCad/`).
Ogni variazione viene salvata prima in un file temporaneo e poi sostituita con
`SDL_RenamePath`. File corrotti, numeri non finiti e tipi errati vengono ignorati;
le misure valide vengono limitate dai token. Snapshot e test non caricano le
preferenze personali: i test optano per un file temporaneo separato.

Per una nuova area, aggiungere un token, un rettangolo a `ShellLayout` e un nodo qui,
poi leggere il risultato per aprire il pannello ImGui e aggiungerlo a `ui.inspect`:

```cpp
// Dentro content, in src/ui/layout.cpp, prima del nodo contextual:
CLAY({.id=id("new_panel"), .layout={.sizing={
    CLAY_SIZING_FIXED(t.new_panel_width*z), CLAY_SIZING_GROW(0)}}}) {}
// Dopo Clay_EndLayout: candidate.new_panel = rect("new_panel");
```

Aggiornare anche il budget delle larghezze per conservare il minimo del viewport.
Non calcolare la posizione del nuovo pannello nella shell. I dialoghi e le tavolozze
contestuali già presenti conservano il comportamento precedente; `contextual` è
riservato e vuoto. Evidenze e differenze visive: `docs/screenshots/layout/README.md`.

## Identità visiva

Riferimenti di comportamento: `docs/reference/` (privati). Aspetto: `docs/design/` e token del tema; SolidCraft è il riferimento autorizzato per la fase 9. Tema predefinito chiaro, accento verde pistacchio e densità compatta. Solo le schermate approvate esplicitamente dall’utente entrano in `docs/design/approvate/`.

## Icone e finestre condivise (M5.5)

Ribbon, menu File e dei gruppi, palette dei comandi, browser e timeline usano il renderer `ui::Icons` tramite `Shell::design_icon_texture`. Le icone del ribbon sono 26 px, quelle dei menu/palette/browser 16 px, quelle della timeline 20 px, prima della scala DPI. `toolbar_command` permette alla palette di riusare la stessa associazione del ribbon; le operazioni non catalogate mostrano il segnaposto. I due fallback espliciti Lucide sono Salva con nome e Scorri selezione; i controlli di navigazione restano Lucide. I ruoli passivi delle voci disabilitate conservano la gradazione delle facce. Gli errori nella timeline usano lo sfondo di errore, senza alterare il master.

Tutti gli `ImGui::Begin` e il posizionamento delle finestre sono in `src/ui/frame.cpp`. I pannelli leggono rettangoli Clay; i dialog applicano i limiti dei token e rimangono agganciati al viewport. Le finestre flottanti si aprono centrate. `heading` usa i token delle intestazioni e riserva spazio al pulsante di compressione del browser. `frame_rule` impedisce creazioni dirette altrove; `frame_limits` verifica limiti e posizioni nei due temi, su due dimensioni e a DPI 1/2. `ui.inspect` espone i rettangoli e i limiti in `frames`, oltre al rettangolo `palette_search` per input riproducibile.

La procedura per master originali, palette, alias e approvazione delle schermate è in `docs/design/README.md`. Gli sfondi del tema chiaro sono bianchi; testi e bordi sono neutri, il pistacchio è un accento. Le tavole approvate sono il riferimento visivo della fase.
