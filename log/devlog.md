# Devlog

## 2026-10-13 — Unità implicita nel dialog parametri

Corretto il caso segnalato con `Nome = larghezza`, `Unità = mm`, `Espressione = larghezza = 40`: dopo aver separato la scorciatoia, l'espressione `40` è scalare; ora il valutatore applica l'unità dichiarata al risultato scalare. Il valore è 40 mm e i parametri dipendenti ereditano la dimensione. Aggiunti test per mm, pollici e per la scorciatoia UI. Build separata `build/agent-unit-fix` verde, CTest 12/12 e controllo dei livelli verde. Lo screenshot offscreen mostra la riga senza errore. L'eseguibile della build precedente resta aperto e Windows ne impedisce la sostituzione; non è stato chiuso per preservare l'eventuale lavoro non salvato.

## 2026-10-13 — Correzione dialog Cambia parametri

La tabella ora ha le colonne Nome, Unità, Espressione, Valore e Commento, con etichette e colori delle intestazioni nei token del tema. Il commento è persistito nel documento con default per i file esistenti. `+` crea una riga; le celle si modificano direttamente, Invio conferma con `param.set`, Esc annulla, e il valore mostra un'anteprima durante la digitazione. La scorciatoia `nome = espressione` viene separata automaticamente se Nome è vuoto o coincide; i nomi incoerenti producono un errore leggibile. Gli errori del comando arrivano allo stato UI e sono mostrati sotto Espressione, nella cella Valore e nei tooltip. Aggiunto un test mirato che confronta il messaggio UI con quello restituito da `param.set` per una divisione per zero; testati anche scorciatoia, commento, anteprima e token chiaro/scuro.

Esaminati gli screenshot chiaro 1440×900, scuro 1280×720 e dell'errore nel tema chiaro. `docs/reference/fusion/` non è presente; l'unica immagine disponibile è `docs/reference/fusion360-ui.png`, che mostra la schermata principale senza il dialog dei parametri. Gli screenshot nuovi sono verifiche, non golden approvate.

Verifica finale Windows: build Release riuscita, CTest 12/12, controllo dei livelli verde, fuzz 60 casi senza crash. L'anteprima segnala anche un nome già usato per una nuova riga, senza mostrare il valore del parametro esistente.

## 2026-10-13 — M1

Completate in sei commit separati le tappe Documento, Espressioni, Registro e storia, fuzz dei comandi, client CLI e UI. Il formato `.sagomacad` usa JSON versionato con default e salvataggio atomico; corpi `brep | mesh` e feature `exact | sdf` sono dati del documento, senza implementare geometria SDF. Il parser valuta unità e riferimenti e segnala cicli, incompatibilità dimensionali e divisioni per zero. Il registro espone 10 comandi, undo/redo e coalescenza. Il fuzz ha esercitato 60 casi: 57 errori controllati e 3 chiamate vuote valide. Il client usa il file di sessione per chiamare l'app e supporta script e file headless.

La barra e il menu File si attivano dal registro; S e Ctrl+K aprono la palette. Modifica > Cambia parametri apre la tabella con nome, espressione, valore calcolato e unità. I test di integrazione verificano `larghezza = 40 mm`, `foro = larghezza / 4`, aggiornamento a 80 mm, undo/redo, salvataggio, riapertura e osservazione dal canale UI. Esaminati `m1-palette-light-1440x900.png` e `m1-parameters-dark-1280x720.png` e confrontati con il riferimento disponibile `docs/reference/fusion360-ui.png`; la cartella `docs/reference/fusion/` non è nel workspace. Il pannello parametri è stato abbassato per non sovrapporsi alla barra a 1280×720. Build Release, CTest 11/11 e controllo dei livelli verdi su Windows.

Su Windows `docs/agents.md` e il già esistente `docs/AGENTS.md` indicano lo stesso file: la guida CLI richiesta è in `docs/agent-cli.md` per preservare le istruzioni del progetto. Le nuove immagini sono screenshot di verifica; nessuna golden image è stata aggiornata. M1.5 e M2 non iniziati.

## 2026-10-09 — M0

Creati i nove target CMake per i livelli, il manifest vcpkg, la shell SDL3/OpenGL con docking Dear ImGui, griglia e assi, temi a token e font Inter con licenza. La CLI produce PNG da un contesto OpenGL nascosto; il server TCP loopback implementa `auth`, `ui.inspect` e `ui.screenshot` con percorsi limitati alla radice di automazione. Aggiunti controllo degli include, test e workflow CI Windows/macOS/Linux.

Verifica locale Windows: build Release riuscita, CTest 3/3, `check_layers.py` verde. Snapshot scuro `docs/screenshots/m0.png` a 2880×1800 (277247 byte) e verifica visiva anche del tema chiaro a 960×600. Corretto il font, la barra superiore tagliata e la palette dei tab dopo la prima ispezione. Resta da eseguire la CI remota e da ottenere l'approvazione dell'utente per promuovere `m0.png` a prima golden image. M1 non iniziato.

## 2026-10-09 — Correzione M0

Rinominati progetto, target, binari, namespace e formato futuro in `sagomacad`; l'app mostra “SagomaCad”. Corrette le stringhe accentate, caricati i glifi Inter Latin-1/Extended, fissato il layout senza barre di docking e senza titolo del viewport. Collegato `sagomacad_kernel` a OpenCascade 8.0.1: box 2×3×4, volume GProp misurato 24. Aggiunti test UTF-8, glifi e geometria. Build Release Windows e CTest 6/6 verdi; `check_layers.py` verde. Esaminati i PNG scuro e chiaro a 1440×900 e 1280×720; `m0.png` è la variante scura 1440×900. Cache binaria vcpkg configurata nel workflow. CI remota ancora da avviare al momento di questa voce. M1 non iniziato.

Primo avvio CI: Linux si è fermato nella dipendenza vcpkg `libxcrypt` perché mancavano gli strumenti Autotools di sistema. Il workflow ora installa tali strumenti e le librerie di sviluppo OpenGL/X11 necessarie; su Windows usa il generatore Visual Studio e CTest Release. La matrice corretta è in fase di rilancio.

Secondo avvio CI: `libxcrypt` richiede anche `libltdl-dev` su Ubuntu. Aggiunta la dipendenza al workflow; nuova matrice in avvio.

Ultimo controllo locale: spostate le etichette delle sezioni nei token `src/ui/theme.hpp`; build Release, CTest 6/6 e `check_layers.py` verdi. Rigenerati e ispezionati i quattro PNG a 1440×900 e 1280×720 nei temi scuro e chiaro. La matrice GitHub Actions è ancora nella configurazione vcpkg; il controllo remoto riprenderà quando sarà disponibile l'esito.

## 2026-10-09 — Piano accesso agenti

Integrato `PLAN-MCP.md` in `docs/PLAN.md` come sezione “Accesso per agenti AI”; aggiunti client CLI in M1, M1.5 per MCP base e strumenti MCP nei traguardi successivi. Aggiunta in `docs/AGENTS.md` la regola di parità CLI/MCP da M1.5. Eliminato il piano separato. Verificata la struttura dei documenti e la codifica UTF-8; nessun codice modificato. Dopo lo spostamento di PLAN e AGENTS in `docs/`, il test UTF-8 esistente mantiene ancora i vecchi percorsi alla radice: aggiornamento rinviato perché questo task è solo documentale.

## 2026-10-09 — Riferimenti UI

Il piano ora richiede una replica 1:1 del layout, flusso, scorciatoie e comportamento di Fusion 360 con asset nostri e aggiunte Tinkercad circoscritte. AGENTS richiede la lettura di `docs/reference/fusion/` prima di lavorare sulla UI e il confronto degli screenshot con i riferimenti. `docs/reference/` è esclusa da Git. Nessun codice modificato.

## 2026-10-09 — Piano Feature SDF

Integrato `PLAN-SDF.md` in `docs/PLAN.md` come sezione “Feature SDF”, in sostituzione della riga dopo M8. M1 prevede i tipi `brep | mesh` e `exact | sdf` e riserva gli id; M2 e M7 prevedono la scelta dello stadio del corpo per l'export. Eliminato il file separato. Nessuna feature SDF implementata.

## 2026-10-10 — Correzione CI

Nel run GitHub Actions `37992156513`, Linux ha superato 6/6 test; macOS ha fallito solo `control` perché il test presumeva 1440×900 pixel fisici anche sui display Retina. Il test ora valida le dimensioni ammesse del framebuffer. Aggiornato anche il test UTF-8 ai percorsi `docs/PLAN.md` e `docs/AGENTS.md` dopo lo spostamento dei documenti. Build Release Windows locale, CTest 6/6 e controllo dei livelli verdi. In attesa dell'esito della nuova CI remota.

Lo stesso run ha poi mostrato un errore Windows in configurazione: `windows-latest` ha Visual Studio 2026, mentre il workflow richiedeva il generatore Visual Studio 17 2022. La matrice ora usa `windows-2022`, che mantiene il generatore e la versione del toolchain verificati localmente. Il run `38000047782` era ancora in corso al momento della correzione.

## 2026-10-10 — Shell M0, tema chiaro

Il tema chiaro è il predefinito di app, stato UI e CLI. Confrontata la shell con `docs/reference/fusion360-ui.png`: ribbon su più righe, browser più vicino alla struttura Fusion, viewport senza pannello Proprietà quando non c'è un comando attivo, griglia isometrica più tenue, indicatore di vista e barra di navigazione. Il tema scuro resta disponibile. Build Release locale riuscita; esaminati i PNG candidati chiaro e scuro a 1440×900 e 1280×720. Nessun test o CI avviato e nessuna golden image sostituita in attesa dell'utente. Restano statici le icone del ribbon e i controlli di vista: il comportamento appartiene ai traguardi successivi.

## 2026-10-10 — Catalogo toolbar M0

Aggiunto `src/ui/toolbar_catalog.cpp`: 33 voci con ID presenti nel piano, organizzate nelle schede e nei gruppi Fusion. Tutte sono disabilitate in M0 perché il registro comandi non esiste ancora; il tooltip dice «Non ancora disponibile». Il catalogo usa 33 icone Lucide distinte, rasterizzate da LunaSVG. `docs/parity.md` elenca 0 voci attive e 33 mancanti. Schizzo compare solo in modalità schizzo; la CLI snapshot può impostare `sketch_mode` per verificarlo. Salva, annulla e ripeti sono vicini al nome del documento. Browser con chevron e occhi, timeline sottile con controlli a sinistra, navigazione a icone. Vista 3D e view cube invariati.

Build Release locale riuscita. Esaminati `m0-candidate-light-v5-1440x900.png`, `m0-candidate-dark-v5-1280x720.png` e `m0-candidate-sketch-1280x720.png`. Il confronto con il riferimento disponibile `docs/reference/fusion360-ui.png` ha evidenziato etichette sovrapposte nel ribbon; ridotti i caratteri delle etichette e allargati i gruppi prima della verifica finale. La cartella `docs/reference/fusion/` indicata da AGENTS non è presente nel workspace. Nessuna CI avviata, nessuna golden image promossa e nessun lavoro M1 iniziato.

## 2026-10-12 — Correzioni finali alla barra M0

Rimosse le schede Superficie, Lamiera e Plastica. Riorganizzati i sette gruppi Solido secondo la richiesta: Crea, Modifica, Costruisci, Ispeziona, Inserisci, Seleziona, Esporta. Ogni gruppo mostra due o tre comandi principali e la tendina contiene tutte le sue voci. Aggiunto il menu File accanto al documento con Nuovo, Apri, Salva e Salva con nome; annulla e ripeti restano accanto. Nel browser solo le voci con figli mostrano il chevron, e Impostazioni documento non mostra l'occhio. Riservati nel piano gli ID mancanti per le nuove voci, senza implementare i comandi. Il catalogo contiene 36 ID verificati contro `docs/PLAN.md` e 36 icone Lucide distinte; `docs/parity.md` elenca 0 attive e 36 mancanti, tutte disabilitate in M0.

Build Release locale di `sagomacad` e `sagomacad-cli` riuscita. Esaminati gli screenshot finali chiaro 1440×900, scuro 1280×720 e Schizzo contestuale 1280×720, confrontati con il riferimento disponibile `docs/reference/fusion360-ui.png`. La cartella `docs/reference/fusion/` resta assente. Nessuna CI avviata, golden image non sostituita e M1 non iniziato.

## 2026-10-13 — M2: kernel, primitive e vista 3D

Completate in sei tappe con commit separati: adattatore OpenCascade con `Result<T>` e test GProp; `primitive.add` per box, cilindro e sfera con espressioni; rigenerazione B-rep con cache per revisione e errori isolati; tassellazione con id faccia, camera prospettica, orbita/pan/zoom, vista iniziale isometrica e view cube cliccabile; `export.stl` con tolleranza e scelta dello stadio `exact | final`; browser con visibilità dei corpi e timeline con doppio clic per modificare la primitiva. In M2 gli stadi esatto e finale coincidono. Il menu della primitiva e l'export STL della toolbar aprono dialoghi; le azioni chiamano il registro comandi.

Il test GProp misura 32 000 mm³ per il box `larghezza × larghezza × 20 mm` con `larghezza = 40 mm` e 72 000 mm³ dopo il cambio a 60 mm. Il test legge il file STL binario ed entro 1 mm³ ritrova entrambi i volumi, con export rispettivamente da `exact` e `final`. I test Release ora eseguono controlli effettivi anche per camera e STL, senza `assert` disattivato da `NDEBUG`.

Esaminati `docs/screenshots/m2-iso-light.png` a 1440×900 e `docs/screenshots/m2-iso-dark.png` a 1280×720. Il modello è in vista isometrica con superfici ombreggiate e spigoli visibili; nel primo rendering gli spigoli posteriori apparivano attraverso il box e sono stati occultati. Confronto fatto con `docs/reference/fusion360-ui.png`: struttura di ribbon, browser, viewport e timeline corrispondente, mentre il modello di riferimento è una piastra più grande. `docs/reference/fusion/` non esiste nel workspace. Questi PNG sono verifiche, non golden approvate.

Verifica finale Windows: build Release verde, CTest 18/18, `tools/check_layers.py` verde. Non avviati M1.5, M3 o CI remota.

## 2026-10-13 — Correzione modifica feature M2

Il doppio clic nella timeline non apriva il pannello perché il codice richiedeva nello stesso frame il rilascio del pulsante e l'evento di doppio clic, emesso alla pressione. Il test UI ora invia due clic alla feature e verifica l'apertura del pannello; verifica anche i menu contestuali della timeline e del corpo nel browser. La timeline offre Modifica feature, Sopprimi/Ripristina ed Elimina. Il browser offre Modifica feature sul corpo.

Creazione e modifica usano lo stesso pannello sulla destra. In modifica mostra le espressioni originali; il viewport usa una copia del documento con rollback temporaneo alla feature e aggiorna la forma durante la digitazione. Annulla scarta la copia senza modificare la storia. OK esegue `feature.edit` con un solo passo di undo. Aggiunti i comandi `feature.suppress` e `feature.delete`; il client `sagomacad-cli call` richiama `feature.edit` tramite il canale di controllo.

Il test numerico misura 32 000 mm³ prima della modifica della box e 48 000 mm³ dopo aver cambiato la sola larghezza a 60 mm; undo ripristina 32 000 mm³. Confrontati gli screenshot del pannello aperto `m2-feature-edit-light.png` (1440×900) e `m2-feature-edit-dark.png` (1280×720) con il riferimento disponibile `docs/reference/fusion360-ui.png`. Il riferimento mostra solo la schermata principale: manca un'immagine Fusion del pannello di modifica in `docs/reference/fusion/`. Le immagini sono verifiche, non golden approvate. Build Release Windows, CTest 20/20 e controllo dei livelli verdi.
## 2026-10-13 — Cartella export

Creata `export/` nella radice del progetto. `export.stl` salva sempre in questa cartella, sia da UI sia da CLI: usa il nome finale anche se riceve un percorso assoluto e restituisce il percorso effettivo nella risposta. I file generati sono ignorati da Git. Il dialogo indica la destinazione e ha il titolo “Esporta STL”. 3MF e STEP non sono ancora implementati.

Test STL aggiornato: esportazione da un percorso richiesto esterno, controllo del percorso effettivo in `export/` e dei volumi prima e dopo il cambio di parametro. Build Release, CTest 20/20 e controllo dei livelli verdi. Esaminati screenshot del dialogo a 1440×900 in tema chiaro e 1280×720 in tema scuro, confrontati con il riferimento generale `docs/reference/fusion360-ui.png`; `docs/reference/fusion/` resta assente.

## 2026-10-13 — Estensione STL obbligatoria

L'utente ha esportato `test` senza estensione: il contenuto era uno STL binario valido (684 byte, 12 triangoli, volume 32 000 mm³, mesh chiusa), ma Windows lo mostrava come “File” e Bambu Studio non lo apriva. Rinominato il file esistente in `export/test.stl`. Il comando `export.stl` ora aggiunge `.stl` ai nomi senza estensione e sostituisce un'estensione diversa; preserva `.stl` senza distinguere maiuscole/minuscole. Il test usa intenzionalmente un nome senza estensione e verifica nome effettivo, lunghezza del file e volume. Build Release, CTest 20/20 e controllo dei livelli verdi.

## 2026-10-13 — M1.5 e strumenti MCP di M2

Realizzato `sagomacad-cli mcp` su stdio con JSON-RPC 2.0 e nlohmann/json, senza dipendenze nuove. Cerca una sessione dell'app e usa il canale di controllo autenticato quando è disponibile; altrimenti crea una sessione headless. Le tappe sono state chiuse con i commit `18d3310` (batch atomici, un passo di undo), `f493ca9` (server MCP, strumenti di comando e risorse) e `88a12d8` (sottrazione B-rep e proprietà di massa). Gli strumenti di file, rendering e screenshot, i test e la guida `docs/mcp.md` chiudono l'ultima tappa.

Esposti `commands_list`, `command_run`, `batch_run`, `undo`, `redo`, `document_inspect`, `document_open`, `document_save`, `render_view`, `mass_properties`, `export` STL e `ui_screenshot` solo in bridge; disponibili le tre risorse `sagomacad://document`, `sagomacad://commands` e `sagomacad://docs/modeling-guide`. I file richiedono percorsi relativi a una radice consentita; un'esportazione usa la cartella `export/` e la sovrascrittura richiede `overwrite=true`. Gli errori MCP includono codice stabile, messaggio e suggerimento. Il fuzz esercita 94 casi tramite MCP.

Il test di integrazione usa soltanto tool MCP per creare `larghezza = 40 mm`, un box 40×40×20 mm e un cilindro di raggio 5 mm sottratto al centro. GProp misura il volume atteso `32000 − 500π = 30429,2037 mm³` entro 0,001 mm³. Quattro viste restituiscono PNG validi; lo STL binario esportato con tolleranza 0,001 mm ha volume entro 2 mm³. Testati salvataggio, riapertura, limiti dei percorsi, overwrite, batch/undo/redo e bridge con app aperta. Verifica Windows Release: build verde, CTest 24/24, controllo dei livelli verde, fuzz 108 casi senza crash.

Esaminati i PNG di verifica `build/agent-root/mcp-m2-light-final-1440x900.png` e `build/agent-root/mcp-m2-dark-final-1280x720.png`: foro, timeline e temi risultano leggibili. Il confronto è con `docs/reference/fusion360-ui.png`; la cartella prescritta `docs/reference/fusion/` non è presente. Le immagini non sostituiscono alcuna golden image. M3 non iniziato.

## 2026-10-14 — Verifica codice e UI

La suite precedente era verde, ma nuovi test hanno riprodotto modifiche parziali dopo un errore di `primitive.edit`, una vista Alto inclinata e PNG MCP incoerenti fra rendering ripetuti. Corrette transazioni dei comandi journal, riferimenti nelle primitive durante la rinomina dei parametri e rifiuto della cancellazione di parametri usati dalle feature. L'anteprima dei parametri ora usa lo stesso comando su una copia; rinomina e geometria sono verificate anche da CLI e MCP. La modifica di un taglio non rinomina più il corpo sorgente.

Il docking e le schede appartengono alla singola shell. Corretto il rendering di più corpi e il calcolo dell'occlusione degli spigoli: i sei screenshot `docs/screenshots/review-*` mostrano viewport con foro, modifica con rollback e parametri in errore, a 1440×900 chiaro e 1280×720 scuro. Le icone Pan, Orbita, Zoom e Adatta alla finestra ora funzionano; test UI con clic e trascinamento, focus testo per Ctrl+Z e riquadro degli errori. Risultati MCP di tipo lista conformi come oggetti `{items: [...]}`.

Build Windows Release e 24 test verificati in `build/agent-review`, controllo dei livelli verde, fuzz 108 casi diretti e 94 MCP senza crash. Rapporto in `docs/review-2026-10-14.md`. Il riferimento disponibile resta `docs/reference/fusion360-ui.png`; mancano i riferimenti dei dialoghi in `docs/reference/fusion/`. Il renderer mantiene l'ordinamento dei triangoli: il controllo di occlusione copre le scene riprodotte, non tutte le intersezioni possibili. L'app dell'utente in `agent-root` è rimasta aperta; il nuovo eseguibile è nella build `agent-review`. Nessuna golden image modificata e M3 non iniziato.

## 2026-10-14 — Navigazione mouse Fusion

Invertiti i gesti del tasto centrale come richiesto: rotella premuta e trascinata sposta la visuale (pan), Shift + rotella premuta ruota la camera (orbita). Aggiornata la sezione Navigazione del piano. Il test UI invia entrambi i trascinamenti e verifica che il pan cambi solo il centro della vista e l'orbita solo gli angoli; prima della correzione il test falliva. La griglia del documento vuoto usa ora la camera, così la navigazione è visibile anche senza corpi; un test controlla che il disegno cambi dopo il pan. Zoom con la rotella invariato. Build Release e test UI verdi, controllo dei livelli verde; controllati screenshot chiaro 1440×900 e scuro 1280×720 senza aggiornare golden image. Eseguibile aggiornato in `build/agent-review/apps/sagomacad-mouse/sagomacad.exe`, poiché la precedente app è aperta.

## 2026-10-14 — Cubo di navigazione

Usate le tre immagini Fusion fornite dall'utente e conservate in `docs/reference/fusion/` con note, escluse da Git. Il cubo ora ha 6 facce con etichette proiettate, 12 spigoli e 8 vertici smussati, selezione delle sole zone visibili ed evidenziazione azzurra al passaggio del mouse. Home ripristina l'isometrica e inquadra il modello; un clic allinea la camera alla zona selezionata, il trascinamento sinistro ruota la vista. Nelle viste di faccia compaiono le quattro frecce per le viste adiacenti e le due rotazioni di 90°. Gli assi X/Y/Z seguono la camera. Misure, colori ed etichette sono token del tema, icone Home e frecce sono disegnate con geometria propria.

La camera gestisce anche il roll e l'orbita oltre i poli, mantenendo una base valida in Alto e Basso. Il mouse sul cubo non attiva la navigazione del canvas; il pannello di creazione/modifica è posizionato sotto il cubo. Test numerici su tutte le 26 direzioni e test UI con clic, Home, frecce, spigolo, vertice, trascinamento e controllo delle sovrapposizioni nei due temi. Screenshot `docs/screenshots/viewcube-*` a 1440×900 chiaro e 1280×720 scuro confrontati con i riferimenti dell'utente; ritardato il tooltip che copriva l'evidenziazione e rese leggibili tutte le etichette degli assi.

Build Release e CTest 26/26 verdi, controllo dei livelli verde, fuzz 108 casi diretti e 94 MCP senza crash. Eseguibile aggiornato nella cartella normale `build/agent-review/apps/sagomacad/sagomacad.exe`. Nessuna golden image sostituita e M3 non iniziato.

## 2026-10-14 — Correzione cubo e orbita

Le nuove immagini dell'utente mostrano un cubo normale: le fasce di selezione di spigoli e vertici sono evidenziazioni sulle facce. La prima implementazione le aveva disegnate come smussi permanenti. Ora il disegno ha sei facce quadrate complete e il contorno originale; le 26 zone di selezione sono separate e mostrano sovrapposizioni piatte solo al passaggio del mouse. Aggiunto un test che verifica i quattro angoli della faccia TOP senza arretramenti, oltre ai test delle 26 direzioni già presenti.

L'orbita precedente applicava rotazioni sugli assi locali e conservava il vettore up risultante, accumulando roll. L'orbita del canvas e del cubo usa ora yaw attorno alla verticale Z e pitch limitato ai poli, senza roll; la rotazione laterale di 90° rimane un'azione esplicita del cubo. Test di 100 trascinamenti verificano roll nullo, asse Z verticale sullo schermo e assenza di ribaltamento dell'orizzonte.

Rigenerati e controllati screenshot del cubo normale, spigolo evidenziato, TOP e modifica feature nei due temi e dimensioni, confrontati con i nuovi riferimenti. Build Release, 26/26 test e controllo livelli verdi. App corretta in `build/agent-review/apps/sagomacad-cube-fixed/sagomacad.exe`: la versione precedente in `apps/sagomacad` è ancora aperta e non è stata chiusa. Nessuna golden image sostituita, M3 non iniziato.

Chiusura del 15 ottobre: verificati eseguibile corretto, controllo dei livelli e diff; salvate le correzioni e gli screenshot in un commit dedicato.

## 2026-10-15 — Selezione e transizioni del cubo

Separate le dimensioni delle evidenziazioni dalle hitbox: fascia visibile di spigoli/vertici ridotta da 0,18 a 0,12, fascia interattiva ampliata a 0,26 e margine esterno di 4 pixel. La geometria del cubo e le dimensioni delle etichette restano indipendenti dalle zone interattive.

Ogni clic su faccia, spigolo, vertice, freccia e Home ora anima la camera per 0,85 secondi con una curva quintica che parte e termina con velocità e accelerazione nulle. Yaw e roll seguono il percorso angolare più breve; Home interpola anche il centro e la distanza della camera. Trascinamento o zoom interrompono la transizione; un nuovo clic riparte dalla posizione corrente.

Test numerici su posizione iniziale, metà percorso, completamento, indipendenza dalla frequenza dei frame, cancellazione e attraversamento di ±180°. Test UI verificano l'assenza di salti al rilascio del mouse per tutti i controlli esercitati e l'interruzione durante il trascinamento. Verificata la selezione oltre il bordo visibile e la minore area delle evidenziazioni. Build Release, CTest 26/26 e controllo livelli verdi. Rigenerati screenshot nei due temi, inclusa una vista a metà transizione; nessuna golden image sostituita, M3 non iniziato. Eseguibile aggiornato in `build/agent-review/apps/sagomacad/sagomacad.exe`.

## 2026-10-15 — M3: sketcher 2D

Implementato `solver/` come libreria statica con Eigen da vcpkg: equazioni per punti, linee, cerchi, archi, tutti i nove tipi di vincoli e cinque tipi di quote richiesti. Levenberg–Marquardt, Jacobiano numerico, rango SVD, gradi di libertà, ID dei vincoli ridondanti/in conflitto e profili chiusi. Il drag risolve i vincoli e, per un punto con un solo grado di libertà, proietta il movimento nello spazio libero: il cursore non deve cadere esattamente sulla curva. Test su equazioni, rettangolo 40×20 (0 GDL), traslazione vincolata (2 GDL), punto su circonferenza (1 GDL), conflitti, ridondanze, cerchio parametrico e archi. Errori di coordinate/residui verificati entro 1e-5 nei casi numerici.

Modello serializzabile della feature `sketch` su XY/XZ/YZ; entità, vincoli e quote conservano ID ed espressioni. Comandi create/finish/edit/line/rect/circle/arc/constraint.add/dimension.add/dimension.set, più `sketch.inspect` e `sketch.point.move`. Aggiornamento delle quote da parametri, rinomina e protezione dalla cancellazione dei parametri usati, salvataggio/riapertura e undo/redo. I batch di creazione sono atomici, con un solo undo e mantenimento dello schizzo attivo. La regen tratta gli schizzi senza tentare di generare corpi B-rep.

UI verificata prima sui quattro riferimenti Fusion in `docs/reference/fusion/`: selezione dei piani trasparenti con hover e suggerimento, transizione frontale, scheda contestuale con gruppi e Fine schizzo verde, tavolozza, griglia adattiva con misure sugli assi, profili azzurri, simboli e colori del solver. Strumenti in `engine/tools/`, anteprima su copia tramite gli stessi comandi. Scorciatoie L/R/C/D, campi quota al primo clic, Tab/Invio/Esc, doppio clic sulle quote, trascinamento dei punti e riapertura dalla timeline/browser. Test UI con eventi reali: R → clic sull'origine → 40 → Tab → 20 → Invio produce il rettangolo completamente vincolato; il drag mantiene l'orizzontale e crea un solo undo. Provate anche le direzioni negative del rettangolo.

Aggiunto MCP `sketch_inspect`; tutte le operazioni sono accessibili con `command_run` e client CLI. Test MCP: rettangolo 60×40 centrato e cerchio Ø10, 0 GDL e due profili, parametro del diametro modificato con undo/redo, conflitto leggibile, PNG da `render_view`, altri comandi di schizzo e parità CLI. L'esempio di `docs/sketches.md` viene eseguito dal test CLI; aggiornati guida MCP, architettura, piano e parità generata dal registro (20 ID di barra registrati, 17 futuri).

Controllati i 16 screenshot delle quattro fasi a 1440×900 e 1280×720, temi chiaro e scuro: file principali `docs/screenshots/m3-*.png`, varianti e note in `docs/screenshots/m3/`. Corrette griglia troppo fitta, icona mancante e suggerimenti sotto la tavolozza. A 720 pixel le opzioni scorrono e Fine schizzo rimane nel piè di pagina. Nel tema scuro la geometria completamente vincolata usa il testo chiaro per contrasto. Corretto anche il readback delle finestre nascoste: il frame completato viene ridisegnato nel back buffer; un test decodifica il PNG e controlla la presenza dell'etichetta CREA, che prima spariva nella cattura.

Build Windows Release in `build/agent-m3` verde, CTest **30/30**, livelli e UTF-8 verdi, fuzz **188 casi diretti + 162 MCP**, senza crash. Binari aggiornati in `build/agent-m3/apps/sagomacad/` e `build/agent-m3/apps/sagomacad-cli/`; configurazioni MCP Windows aggiornate a questi percorsi. Nessuna CI remota avviata e nessuna golden image sostituita. **M4 non iniziato**: estrusioni/tagli da profili restano nel traguardo successivo.

## 2026-10-15 — Correzione del disegno con il mouse

La segnalazione dell'utente ha evidenziato un errore nel flusso M3: il focus automatico della quota attivava `WantTextInput`, bloccando l'aggiornamento del puntatore; anche il controllo hover del canvas rifiutava il campo attivo. Inoltre le caselle mostravano solo il testo digitato, senza aggiornare le misure correnti. Ora primo clic → anteprima continua → secondo clic conferma funziona per linea, rettangolo e cerchio; l'arco usa centro, primo punto e punto finale. Le caselle sono overlay che lasciano passare il mouse e mantengono l'input da tastiera: mostrano le misure live, il primo carattere sostituisce il valore selezionato e solo una modifica effettiva blocca la quota. Tab cambia campo; una misura non bloccata continua a seguire il puntatore anche durante la digitazione nell'altra.

Le misure ottenute solo dal mouse definiscono la geometria iniziale senza aggiungere quote dimensionali. I flag `driving_*` dei comandi distinguono questo caso dalle espressioni vincolanti e sono verificati anche via MCP. Corretto anche l'angolo di linea digitato senza lunghezza e il raggio dell'arco scelto con il mouse, che resta costante durante la scelta del punto finale. La precisione di visualizzazione delle misure è di tre decimali; la geometria conserva la precisione dei comandi.

Nuovo test UI di regressione: movimento con input attivo, attraversamento dell'overlay, aggiornamento di entrambe le misure, documento invariato durante l'anteprima, conferma clic-clic senza tastiera, arco con mouse e larghezza 40 bloccata con altezza libera. Verifica punti finali, quote effettivamente create e GDL residui; il test falliva prima della correzione. Restano verdi anche R → clic → 40 Tab 20 Invio e i test precedenti. Screenshot in `docs/screenshots/m3-mouse/` nei due temi e dimensioni, guardati e confrontati con i riferimenti Fusion; nessuna golden sostituita.

Build Release separata `build/agent-sketch-mouse`, perché l'utente ha ancora aperto l'eseguibile in `build/agent-m3`: nessuna app chiusa e nessun documento scartato. **31/31 test verdi**, controllo livelli/UTF-8 verdi, fuzz **188 diretti + 162 MCP** senza crash. App corretta: `build/agent-sketch-mouse/apps/sagomacad/sagomacad.exe`. M4 non iniziato.

## 2026-10-16 — LAYOUT, tappa 1: dipendenza e livelli

Clay 0.14 disponibile in vcpkg, fissato dal builtin-baseline esistente. Aggiunti manifest, attribuzione zlib e implementazione isolata sotto ui/. Header e implementazione compilano C++20 con MSVC 19.44 e GCC 13.3 (WSL). Regola clay.h verificata per tutti i livelli inferiori, anche con percorso prefissato. Build Release separata build/agent-layout, CTest 32/32 verde, incluse le prove fuzz esistenti. Quattro screenshot prima della migrazione conservati in build/layout-baseline; golden invariate. Il branch contiene già M3: questa migrazione ne conserva le funzioni, senza espanderle.


## 2026-10-16 — LAYOUT, tappa 2: shell e interazioni

Rimosso il dockspace: Clay dichiara topbar, ribbon e relativi gruppi/pulsanti, browser, viewport, proprietà, timeline, separatori e area contestuale vuota. Rettangoli ShellLayout, decorazioni su ImDrawList e misura dei font ImGui alla scala del frame; widget e ID dei comandi invariati. Arena per istanza limitata a 512 elementi e 1024 parole. Testato anche un budget di 32 elementi: Clay 0.14 emette un comando diagnostico invece di richiamare il callback per questo esaurimento; il wrapper lo inoltra allo stesso handler, mantiene l'ultimo layout valido e recupera nel frame successivo.

Separatori trascinabili, reset al doppio clic, compressione browser/proprietà e preferenze JSON con validazione e sostituzione tramite file temporaneo. Snapshot isolati dalle preferenze personali. ui.inspect espone rettangoli, motore, scala, stato serializzabile e diagnostica. Test su 384 combinazioni di dimensioni, visibilità, compressione e scala; 1280x720, 1920x1080 e 800x600 mantengono viewport minimo 320x200. Verificati trascinamenti reali (browser 243→298, proprietà 253→288, timeline 62→106), riavvio e reset. Gli screenshot prima/dopo il riavvio sono identici. Build Release e CTest 34/34 verdi; fuzz 188 casi diretti e 162 MCP, senza crash. Implementazione Clay e albero del layout compilati anche con GCC 13.3 e Clang 18.1 in WSL.


## 2026-10-16 — LAYOUT, tappa 3: verifica visiva e documentazione

Completato LAYOUT nel piano, aggiornati AGENTS, stack/L6, architettura, guida UI e protocollo ui.inspect. Rafforzato il fallback preservando anche la scheda del ribbon, così i widget non accedono al catalogo della scheda nuova con i rettangoli precedenti. Testati entrambi i pannelli comprimibili e anche i separatori fra le aree nei controlli di sovrapposizione.

Quattro confronti prima/dopo affiancati in docs/screenshots/layout, temi chiaro/scuro a 1280x720 e 1440x900. Pixel diversi senza tolleranza: chiaro 7960/8474 (0,8637%/0,6539%), scuro 3374/2888 (0,3661%/0,2228%). Cambiano la freccia di compressione, i bordi senza docking e arrotondamenti di un pixel nei gruppi del ribbon. Nel contenuto del viewport cambia un solo pixel di antialiasing del cubo, di 1–6 livelli RGB; pannelli, camera e geometria restano nella stessa posizione. Confronti guardati nei quattro formati, scala 2 e finestra 800x600, con i riferimenti Fusion locali. Screenshot di trascinamento, compressione e riavvio allegati alla stessa cartella; prima/dopo il riavvio identici pixel per pixel.

Verifica finale Windows Release: CTest 34/34 in 14 secondi, check_layers e UTF-8 verdi, fuzz 188 diretti + 162 MCP. Header/implementazione Clay e layout.cpp compilati C++20 con MSVC 19.44, GCC 13.3 e Clang 18.1; nessuna suite completa Linux/macOS o CI remota avviata. App aggiornata in build/agent-layout/apps/sagomacad/sagomacad.exe. Golden invariate, nessuna funzione CAD aggiunta. M3 già presente preservato, M4 non iniziato.

## 2026-10-16 — M4, tappa 1: regioni dei profili

Aggiunte regioni selezionabili con contorno esterno e fori immediatamente annidati. Rettangolo con cerchio produce corona e disco; tre contorni annidati producono tre regioni distinte. Gli ID canonici contengono gli ID delle entità, senza coordinate o indici del solver. `sketch.inspect` espone `regions` accanto ai contorni M3. Test dedicato su annidamento, picking, modifiche dimensionali, costruzioni e contorni aperti. Esaminati tutti i riferimenti Fusion 07–10 prima del lavoro UI. Build separata `build/agent-m4`; nessuna app dell'utente chiusa.
Verifica tappa 1: build Windows Release verde, CTest 35/35 (inclusi CLI, MCP, fuzz), controllo livelli e UTF-8 verdi. Restano le tappe 2–6 di M4; M5 non iniziato.

## 2026-10-16 — M4, tappa 2: estrusione

Kernel con facce esatte da linee, cerchi e archi, fori annidati, prisma e booleane Unisci/Taglia/Interseca; tutte le eccezioni OCCT rimangono nel kernel. `feature.extrude` è nel registro unico CLI/MCP, conserva profili per ID, espressioni, direzioni un lato/simmetrica/due lati, distanza del secondo lato, Attraverso tutto e lista dei corpi di destinazione. Inserimento al marker, validazione su copia transazionale, rinomina e protezione dei parametri delle feature.

Test GProp: corona 60×40 meno Ø10, altezza 10 = 23214,601837 mm³; dopo larghezza 80 e Ø20 = 28858,407346 mm³. Operazioni su box con utensile parzialmente sovrapposto: nuovo 1500, unione 4500, taglio 3000, intersezione 1000 mm³. Verificati limiti dei tre versi, Attraverso tutto su corpo alto 50 mm, taglio di due corpi, piano XZ, undo/redo e atomicità degli errori. Build Windows Release e CTest 36/36 verdi; livelli, UTF-8 e fuzz verdi. UI e tappe 3–6 ancora aperte, M5 non iniziato.

## 2026-10-16 — M4, tappa 3: rivoluzione

`feature.revolve` usa le regioni stabili e le quattro operazioni della tappa 2. Asse X/Y/Z dell'origine oppure ID di una linea dello schizzo, anche di costruzione; tipo angolo/completo, espressione angolare e parametri a monte. BRepPrimAPI_MakeRevol isolato nel kernel, input finiti e intervallo (0,360°], errori atomici.

Test GProp: rettangolo XZ fra raggi 5 e 15 mm, alto 10 mm, attorno a Z → 6283,185307 mm³; quarti e metà del volume a 90° e 180°. Asse da linea produce lo stesso risultato. Raddoppiare altezza raddoppia il volume. Unione/taglio/intersezione con cilindro: 2250π, 250π e 750π mm³. Build Release e CTest 37/37 verdi, livelli e fuzz verdi. Tappe 4–6 aperte; M5 non iniziato.

## 2026-10-16 — M4, tappa 4: timeline parametrica

Rigenerazione incrementale per prefisso: chiavi con dati ed espressioni valutate, cache dei soli corpi modificati da ciascun passo. Una modifica dell'estrusione conserva primitive e schizzo precedenti; una quota a monte invalida dal primo schizzo dipendente; parametri estranei non invalidano. Rollback indietro/avanti riusa i risultati invariati. Errori isolati, valutazione successiva, soppressione e cancellazione con marker conservato. `feature.edit` modifica i parametri di estrusioni/rivoluzioni e `feature.get` espone stato, dati ed errore. `timeline.rollback` è disponibile anche da CLI/MCP tramite il registro. Gli schizzi consumati sono nascosti come stato derivato e riappaiono prima della feature o durante la modifica.

Timeline UI con icone Lucide distinte da 20 px, marker verticale trascinabile da 3 px fra le icone, passi dopo il marker grigi, controlli inizio/indietro/play-pausa/avanti/fine a sinistra. Drag coalescente (un undo). Confrontati con il riferimento 10 gli screenshot chiaro 1440×900 e scuro 1280×720, oltre alle altre due combinazioni: stessi controlli e posizione relativa del marker, modello con sola estrusione prima del marker, icone successive attenuate. Differenze grafiche volute: Lucide, tema SagomaCad, marker blu anziché nero. Le etichette delle feature sono nei tooltip anziché occupare la riga. L'errore dopo eliminazione dello schizzo è rosso e il tooltip dice «Schizzo sorgente mancante: sketch-1»; la rivoluzione a valle resta valida. Evidenze in `docs/screenshots/m4-confronto/`, coppia affiancata 10 da completare insieme ai dialoghi nella tappa 5.

Build Windows Release, CTest 39/39, livelli, UTF-8 e fuzz verdi. Test numerici su prefisso in cache, modifica quota, rollback e ripristino; test UI con eventi reali del mouse nei due temi e due dimensioni. Tappe 5–6 aperte; M5 non iniziato.

## 2026-10-16 — M4, tappa 5: dialoghi e verifica visiva 07–10

Pannello flottante destro dichiarato in Clay, sotto il cubo, con piè di pagina fisso e campi scorrevoli quando necessario. E apre Estrudi, Rivoluzione è nel gruppo Crea; doppio clic sulla timeline riapre entrambi. Il motore produce regioni triangolate e anteprime B-rep; la UI proietta e inoltra selezione e drag. Selezione multipla di regioni dello stesso schizzo, freccia di distanza e casella modificabile, anteprima blu/rossa, operazioni con icone Lucide e lista dei corpi. Il campo Asse attiva il picking di X/Y/Z o linee dello schizzo; anello e maniglia aggiornano l'angolo. Anteprima e annullamento non modificano documento o undo. La modifica dello schizzo mostra il modello al suo passo; finire lo schizzo rivaluta i successivi. Una vista frontale passa dolcemente all'isometrica aprendo una feature, rendendo visibile la maniglia.

Confronti affiancati in `docs/screenshots/m4-confronto/`, quattro coppie per scena (chiaro/scuro × 1440×900/1280×720), rigenerabili con `tools/m4_compare.py`:

- **07**: stesso ordine Tipo, Profilo, Inizio, Direzione, Estensione, Distanza, Angolo sformo, Operazione; OK/Annulla in basso a destra. Corona evidenziata separatamente dal disco, freccia e valore vicino alla faccia, solido semitrasparente. Differenze: italiano, Lucide e temi propri; Solido/Piano profilo/sformo zero sono costanti, non nuove opzioni geometriche; 10 mm invece di zero per mostrare il volume. Corrette le giunzioni visibili fra triangoli del riempimento, disabilitando l'antialiasing dei bordi interni.
- **08**: ordine Unisci/Taglia/Interseca/Nuovo corpo, quattro icone distinte, anteprima blu per nuovo corpo e rossa per taglio, campo Corpi con due destinazioni nella cattura supplementare `08-taglia-*`. Differenze: icone Lucide; a 720 px il menu si apre verso l'alto per rimanere nello schermo. Corretti icone/testo sovrapposti, altezza delle voci e dimensioni del pannello.
- **09**: Profilo/Asse/Tipo/Angolo/Direzione/Operazione nello stesso ordine, picking attivato dal campo Asse e clic su Z nel viewport, asse evidenziato, anteprima completa a 360°, maniglia e valore angolare. Differenze: blu semitrasparente e asse arancione; assi d'origine o linee già complanari, senza Proietta asse; nessuna icona Autodesk. Corretta l'etichetta Rivoluzione che andava a capo nel ribbon e il valore mostrato nel tipo Completo.
- **10**: marker fra le icone, passi successivi attenuati e non applicati, icone da 20 px diverse per schizzo/estrusione/rivoluzione, cinque controlli a sinistra. Differenze: marker blu e icone Lucide, nomi nei tooltip; griglia/browser/tema SagomaCad. Verificata anche l'estrusione rossa con errore leggibile e la prosecuzione della rivoluzione successiva.

Build Windows Release verde e CTest 40/40, livelli/UTF-8/fuzz verdi. Test UI nei quattro formati: E, corona e disco separati, drag distanza senza modificare documento, digitazione nella casella del viewport, menu e due corpi, taglio con volume corretto, doppio clic, Rivoluzione dal ribbon, selezione Z, drag da 360° a circa 270° e volume coerente con l'angolo effettivo. Controllati pannello/cubo e piè di pagina anche alla scala 2. Nessuna golden sostituita. Resta la tappa 6 (guida e accettazione CLI/MCP); M5 non iniziato.

## 2026-10-17 — M4, tappa 6: agenti e accettazione finale

Aggiunto il tool MCP `feature_get` con schema e validazione, collegato a `feature.get` del registro comune. La risorsa `sagomacad://docs/modeling-guide` incorpora a build `docs/modeling-guide.md`: lo stesso file viene eseguito dai test, senza guida duplicata nel server. Esempi completi con handshake MCP: schizzo, regioni, corona estrusa, parametri, secondo taglio passante, modifica quota a monte, rivoluzione, modifica angolo, rollback, soppressione ed errore dopo eliminazione della sorgente. Testata la parità CLI con script pubblicato e creazione/modifica di rivoluzione, oltre al bridge MCP sull'app nativa. Aggiornati architettura, guide CLI/MCP e parità (22 ID di barra registrati, 15 futuri).

Via MCP è stata creata dal testo una staffa a L larga 60 mm, gambe 40 mm, spessore 5 mm, due fori passanti Ø8, uno su ciascuna gamba. `mass_properties`: **21997,34517542563 mm³**, area **10400,265482457437 mm²**, limiti 60×40×40 mm (tolleranza bounding box 1,5e-7 mm). Formula: 22500 − 160π. Cambiare larghezza a 80 e foro a 10 rivaluta con i volumi attesi. Documento editabile, risposta JSON e render nativo MCP conservati in `docs/examples/m4/`; test rigenerabile in `build/agent-m4/m4-acceptance`.

L'accettazione con più destinazioni ha aggiunto una regressione per Unisci: due corpi e utensile ponte producono un solo corpo da 3000 mm³, consumando il secondo senza duplicare l'utensile. Verificata anche distanza negativa. La cattura MCP ha riprodotto strisce scure sulle facce opache della staffa: erano triangoli posteriori ordinati per profondità media. Aggiunto backface culling solo per i solidi opachi; le anteprime semitrasparenti mantengono entrambi i lati. Un controllo dei pixel su otto punti complanari attorno al foro verifica uniformità nei due temi e due dimensioni. Ricontrollate le catture di taglio e staffa, rigenerati i confronti con il font Inter del progetto; le differenze 07–10 restano quelle documentate alla tappa 5, eliminati gli artefatti sulle superfici opache.

Verifica finale: **build Windows Release verde, CTest 41/41** in 44,67 s, livelli e UTF-8 verdi, fuzz **212 diretti + 182 MCP**, senza crash. Nessuna CI remota o suite Linux/macOS eseguita. Nessuna golden sostituita, nessuna app dell'utente chiusa. Binari in `build/agent-m4/apps/sagomacad/` e `build/agent-m4/apps/sagomacad-cli/`. Rapporto in `docs/m4-validation.md`. **M4 completato nelle sei tappe; M5 non iniziato.**

## 2026-10-17 — Correzione visibilità creando un secondo schizzo

Il filtro del viewport in modalità schizzo disegnava esclusivamente lo schizzo attivo: crearne un secondo faceva scomparire tutte le geometrie precedenti. Ora gli schizzi precedenti restano visibili secondo rollback, soppressione e consumo da parte delle feature. Il contesto di valutazione segue il passo dello schizzo in modifica, senza applicare a monte il nascondimento prodotto dalle feature successive. Riempimenti, quote, vincoli e selezione appartengono solo allo schizzo attivo, anche quando gli ID locali delle entità coincidono.

Test di regressione sui pixel del bordo del primo rettangolo mentre si crea e seleziona una linea nel secondo schizzo: fallisce prima della correzione e passa dopo. Verificati anche fine schizzo, soppressione/ripristino e rollback/ripristino. Build Windows Release verde, **CTest 41/41** in 21,84 s (compresi livelli e UTF-8). Test UI ripetuto a 1440×900 e 1280×720 nei temi chiaro/scuro; screenshot ispezionati in `docs/screenshots/sketch-visibility/`. Confronto con `docs/reference/fusion/modalita-schizzo.png`: tavolozza, viewport ortogonale e schizzo attivo mantengono il flusso di riferimento; i contorni dello schizzo precedente restano visibili senza quote estranee. Differenze intenzionali: icone Lucide, colori propri e pannello con scorrimento a 720 px. Nessuna golden aggiornata, M5 non iniziato.

## 2026-10-17 — Correzione solidi invisibili durante un nuovo schizzo

La modalità schizzo escludeva anche il rendering delle superfici 3D: il corpo esisteva nel documento e nella mesh, ma il ramo che disegna la griglia globale saltava insieme i solidi. Separato il disegno dei solidi dalla griglia: i corpi precedenti restano visibili sotto geometrie e quote dello schizzo attivo. Restano applicati la visibilità del corpo e il contesto temporale della modifica a monte.

Esteso il test UI con rettangolo 40×20 estruso di 10 mm e nuovo schizzo con cerchio Ø8: la verifica dei pixel della faccia piena fallisce prima della correzione e passa dopo. Controllati anche nascondi/mostra corpo durante lo schizzo, Fine schizzo e modifica della sorgente precedente all'estrusione. Screenshot ispezionati nei due temi a 1440×900 e 1280×720 in `docs/screenshots/solid-visibility/`: solido pieno, cerchio e quota sovrapposti correttamente, tavolozza accessibile. Riferimenti Fusion già esaminati per modalità schizzo e 07; restano colori e icone propri e vista ortogonale durante lo schizzo. Build Windows Release verde, **CTest 41/41** in 22,43 s, inclusi livelli e UTF-8. Nessuna golden aggiornata, M5 non iniziato.

## 2026-10-17 — Audit dei flussi schizzo/estrusione e recupero dei profili

Rapporto completo in `docs/sketch-extrude-audit.md`, con matrice dei flussi, difetti e riproduzioni. Verificato il confine del piano: M4 estrude profili; selezione e operazioni su facce in M5, push/pull in M6. Dopo il chiarimento dell'utente non sono state anticipate queste funzionalità.

Aggiunto `sketch.visibility` con true/false/null (mostra/nascondi/automatico), accessibile da registro, CLI e MCP. La scelta è nei dati dello schizzo e sopravvive al solver, alle modifiche di quota, al salvataggio e a undo/redo. Occhio effettivo e tooltip nel browser; selezione dello schizzo + E e menu contestuale Estrudi permettono il riuso anche della sorgente nascosta. Doppio clic continua a modificare lo schizzo. Corrette selezioni obsolete passando fra schizzi e dialoghi, pulizia su Annulla/Esc, disambiguazione dei profili coplanari con la sorgente scelta nel browser e contesto temporale degli overlay. Un nuovo schizzo si inserisce al marker, senza scartare il suffisso.

Il test UI esteso ha riprodotto un taglio che tentava di sottrarre l'intera piastra dopo Annulla: la vecchia preselezione restava attiva. Dopo la correzione la sequenza completa passa nei quattro formati. Nuovo test `sketch_extrude_workflows`: piastra 8000 mm³, seconda estrusione Unisci 12000 mm³, quota a monte 18000 mm³, foro passante Ø4 con volume 18000 − 60π mm³. Verificati annulla, riapertura, undo/redo, visibilità senza variazioni geometriche, sorgente eliminata, persistenza e inserimento al marker. Esteso anche il test MCP/CLI con il nuovo comando e l'ispezione della visibilità.

**Resta aperto un difetto distinto:** due cerchi Ø20 intersecanti, centri (0,0) e (10,0), producono due regioni intere invece di tre porzioni disgiunte. Il riconoscitore gestisce cicli e annidamenti, non l'arrangiamento delle curve intersecanti. Riproduzione conservata in `docs/examples/sketch-extrude-audit/`, segnalata anche nel piano; non è dichiarata risolta né attribuita a M5.

Build Windows Release verde, **CTest 42/42 in 24,91 s**, livelli e UTF-8 verdi. Fuzz **218 diretti + 187 MCP**, nessun crash. Rieseguito con esito verde il test UI nei quattro formati dopo aggiunta del controllo Esc. Otto screenshot ispezionati in `docs/screenshots/sketch-extrude-audit/`: riuso del profilo e selezione fra schizzi sovrapposti. Confronto con Fusion 07–08 e modalità schizzo: ordine dei campi invariato, occhi funzionali e sorgente recuperabile; differenze intenzionali Lucide, italiano, colori e scorrimento nelle finestre basse. Guida incorporata MCP aggiornata e verificata dal test eseguibile. Nessuna golden aggiornata, nessuna finestra utente chiusa o nuova istanza avviata; i binari ricompilati richiedono il normale riavvio delle istanze già aperte. M5 non iniziato.

## 2026-10-17 — CI multipiattaforma sospesa per scelta dell'utente

La verifica dei lavori è da ora solo locale su Windows: build Release, ctest, controllo dei livelli, fuzz se si modificano i comandi e screenshot se si modifica la UI. Nessun push per avviare la CI, nessuna attesa o lettura dei suoi risultati e nessuna correzione specifica macOS/Linux; il codice resta portabile. Eventuali richiami residui alla CI nei prompt non cambiano questa scelta. Le precedenti voci del devlog restano come cronologia, non come attività remote da riprendere.

Conservato `.github/workflows/ci.yml` e cambiato esclusivamente il trigger da push/pull_request a `workflow_dispatch`: l'utente può avviarlo manualmente da GitHub, con job e matrice originali invariati. Inserita la politica richiesta in `docs/AGENTS.md`; allineati traguardi, test bridge e prompt iniziale di `docs/PLAN.md`, eliminando i requisiti di verifica remota multipiattaforma.

Verifica locale della modifica: confronto automatico del workflow con HEAD (solo trigger diverso), conferma che test e configurazioni CMake non sono cambiati, controllo dei livelli e UTF-8 verdi, diff senza errori di whitespace. Nessuna esecuzione remota, nessun push, nessuna modifica ai test locali o al codice applicativo. Per questa modifica di configurazione e documentazione non sono state ripetute build e suite CTest. Lo sviluppo CAD resta fermo allo stato precedente.

## 2026-10-17 — M5, tappa 1a: infrastruttura del picking prima della UI

Esaminate le immagini Fusion 11 (tre varianti) e 12–16. Aggiunti ID univoci degli spigoli OCCT alla tassellazione: una scatola produce 6 ID faccia e 12 polilinee con ID spigolo, senza duplicare le occorrenze dei wire. Buffer software ID/profondità nel motore, senza dipendenze UI, per facce, spigoli, corpi e schizzi; query in profondità con deduplicazione per Seleziona altro e selezione additiva/toggle per Ctrl o Shift. Test su occlusione, priorità degli spigoli complanari, filtri, raggruppamento per corpo, limiti di allocazione e svuotamento della selezione.

Build Windows Release in `build/agent-m5` verde, CTest **43/43 in 23,53 s**, livelli e UTF-8 verdi; fuzz incluso nella suite verde. Nessuna UI modificata, nessuna golden aggiornata, nessuna CI. Il picking non è ancora collegato al viewport: la tappa 1b seguirà il cancello dei test topologici richiesto prima della UI. Piano di verifica in `docs/m5-validation.md`. M5 in corso, M6 non iniziato.

## 2026-10-17 — M5, tappa 2a: nomi e storia topologica nel kernel

Aggiunti riferimenti serializzabili con nome e firma geometrica (tipo, baricentro, direzione, area/lunghezza), query di facce/spigoli/vertici e frame delle facce piane. Le estrusioni nominano cap_start/cap_end e side(entity); le primitive hanno ruoli geometrici, gli spigoli e vertici derivano dalle facce adiacenti. Trasformazioni e booleane propagano gli alias tramite Modified/Generated/IsDeleted di OCCT, con marcatura delle entità eliminate. Un alias diviso fra più discendenti è ambiguo e produce errore; il fallback geometrico richiede una corrispondenza unica entro tolleranza. Nessun riferimento persistente contiene indici di enumerazione o puntatori OCCT.

Test dedicato: 26 entità univoche della scatola, faccia superiore conservata passando da 40×40×20 a 65×40×35, estrusione diretta della faccia appoggiata alla nuova quota; quattro spigoli verticali risolti per nome, raccordo R3 con 10 facce e volume (larghezza×40 − (4−π)×9)×altezza; smusso D2 con volume (larghezza×40−8)×altezza. Raggio 1000 rifiutato senza crash. Taglio che elimina la sommità: errore Entità eliminata e nessun riaggancio alla nuova faccia. Taglio che divide la sommità: riferimento ambiguo. Verificati Modified, traslazione e fallback unico/rifiutato.

La suite ha trovato un alias di spigolo estraneo alla faccia dopo un profilo con foro: prima di interrogare la storia del generatore si verifica ora l'appartenenza alla sorgente. La regressione model_extrude torna verde. Build Windows Release verde, CTest **44/44 in 23,20 s**, livelli e fuzz della suite verdi. UI ancora invariata: resta da verificare la catena di feature reali schizzo su faccia/foro/raccordo in regen prima di iniziare la UI. M5 in corso, M6 non iniziato.

## 2026-10-17 — M5, tappa 3a: schizzi su faccia nel motore, prima della UI

`sketch.create` accetta `face: {body,name,signature?}` in alternativa a plane. Cattura la firma risolta e proietta automaticamente i bordi lineari/circolari/archi in coordinate locali della faccia. Gli ID delle proiezioni derivano dal nome stabile, non dall'ordine OCCT. Sono geometria di costruzione immutabile, esclusa dai profili ma utilizzabile nei vincoli. Il solver blocca punti e raggi delle proiezioni; lo spostamento diretto è rifiutato. Frame ortonormale e supporto sono dati serializzabili con default compatibili coi vecchi file. La regen aggiorna frame e bordi nel contesto temporale precedente allo schizzo.

Test reale: scatola 40×40×20, schizzo sulla sommità, cilindro Ø10 alto 6; portando larghezza a 60 e altezza a 35, il cilindro mantiene z_min=35 mm e volume 150π mm³, anche dopo serializzazione. Le quattro proiezioni sono fisse, un cerchio aggiuntivo può vincolarsi a un loro punto. Test di eliminazione effettiva a monte: un taglio prima dello schizzo passa da 5 a 40 mm di larghezza e cancella la sommità; schizzo ed estrusione dipendente segnalano errore leggibile, una sfera successiva viene rigenerata. Stesso flusso di appoggio eseguito da CLI e MCP command_run con test dedicato.

Build Windows Release verde, CTest **46/46 in 24,05 s**, inclusi fuzz, livelli e UTF-8. UI ancora invariata: allineamento camera, colore delle proiezioni e confronti visivi verranno integrati dopo i test reali di raccordi e fori richiesti dal cancello topologico. Nessuna CI, M6 non iniziato.

## 2026-10-17 — M5, tappa 4a: estrusione diretta delle facce nel motore

`feature.extrude` accetta `face: {body,name,signature?}` in alternativa a sketch/profiles. Cattura la firma e risolve la faccia nel prefisso della timeline, verificando che sia piana. Calcolo condiviso con le estrusioni da schizzo per direzione singola/simmetrica/due lati, distanze negative, Attraverso tutto, nuovo corpo/unione/taglio/intersezione e destinazioni multiple. Attraverso tutto usa coordinate relative al piano e gli otto vertici della bounding box, anche per normali oblique. `feature.edit` conserva il riferimento o ricattura la firma quando cambia la faccia.

Test su dodici combinazioni operazione/direzione e sulle stesse dopo cambio larghezza/altezza a monte, faccia laterale con normale X, modifica distanza, rifiuto atomico di sorgenti incompatibili. Verificato Attraverso tutto da faccia z=30 verso un altro corpo alto 40: volume 94000 mm³ in una direzione e 92000 mm³ sui due lati. Esteso il test CLI/MCP: estrusione diretta che segue la sommità fino a z=40.

Build Windows Release verde, CTest **47/47 in 23,64 s**, livelli e fuzz inclusi verdi. UI ancora invariata in attesa delle regressioni reali di raccordi e fori. M5 in corso, M6 escluso.

## 2026-10-17 — M5, tappa 5a: raccordo e smusso parametrici nel motore

Registrati `feature.fillet` (F) e `feature.chamfer`: liste di spigoli dello stesso corpo, raggio/distanza come espressione, firme catturate, modifica tramite feature.edit, un passo di undo. La rigenerazione risolve i nomi su ogni valutazione e propaga la storia OCCT. In caso di errore conserva la forma precedente e segnala nella feature nomi stabili e ID degli spigoli coinvolti. Nessuna eccezione OCCT esce dal kernel.

Test reali con quattro spigoli verticali, R3/D3, cambio della scatola da 40×40×20 a 60×40×35: 10 facce e volumi analitici corretti. Verificati modifica a 2 mm e undo, errore atomico di creazione, parametro R portato a 1000 mm con errore leggibile e corpo precedente da 84000 mm³, ritorno a R3. CLI e MCP eseguono sia raccordo sia smusso su quattro nomi stabili e verificano i volumi.

Build Windows Release verde, CTest **48/48 in 26,05 s**, livelli/UTF-8/fuzz verdi. Le scorciatoie sono dichiarate nel registro; maniglie, dialoghi e picking del viewport verranno collegati dopo il test reale del foro, ultimo requisito del cancello prima della UI. M5 in corso, M6 non iniziato.

## 2026-10-17 — M5, tappa 6a: foro e quote persistenti dai bordi nel motore

Registrato `feature.hole` (H): faccia piana stabile, punto locale, diametro/profondità come espressioni o passante. Il clic cattura automaticamente i due bordi lineari più vicini non paralleli; distanze e lato diventano due vincoli serializzati sui loro nomi stabili. Si possono fornire esplicitamente e modificare con feature.edit. La posizione viene risolta dalle due equazioni nella faccia attuale, senza fissare il centro in coordinate globali. Validati appartenenza dei bordi, parallelismo, centro interno alla faccia, dimensioni e risultati OCCT.

Test: Ø8 passante a 10 mm dai due bordi su scatola 40×40×20, poi 60×40×35: centro x=y=10, volume 84000−560π mm³ e 7 facce. Variante cieca profonda 6: volume 84000−96π mm³ e 8 facce. Una quota guidata dal parametro offset passa da 10 a 12 mantenendo l'altra a 10; verificati cache, modifica, persistenza, errore atomico per centro esterno ed errore leggibile quando un taglio a monte elimina la faccia. Il flusso passa anche da CLI e MCP, compresa l'ispezione delle due quote con firme geometriche.

Build Windows Release verde, CTest **49/49 in 25,35 s**, livelli/UTF-8/fuzz verdi. **Cancello prima della UI superato:** sono ora testate le catene reali di schizzi su faccia, estrusioni da faccia, raccordi e fori dopo modifiche a monte, più cancellazione effettiva e prosecuzione della regen. UI ancora invariata fino a questo commit; restano integrazione, strumenti agenti e confronto visivo. M6 non iniziato.

## 2026-10-17 — M5, tappa 7a: query, misure e selezione per gli agenti

Aggiunti comandi geometry.query, inspect.measure, selection.set/get e tool MCP geometry_query, measure, selection_get. Query con filtri corpo/tipo di entità/tipo geometrico/direzione parallela/normale e limite dei risultati; facce, spigoli e vertici restituiscono nomi, firme, descrizioni e bordi delle facce. Misure OCCT esatte della distanza minima e angolo acuto fra piani/rette; angolo null quando non definito. selection_get rifiuta la modalità headless; collegamento al viewport nativo nella prossima tappa UI.

Test con 26 entità della scatola, filtro dei quattro spigoli verticali, distanza 20 mm fra sommità e fondo, angoli 0° e 90°, limiti delle query e vettore nullo rifiutato. Un agente MCP interroga gli spigoli verticali e li raccorda con R2: il corpo ha 10 facce. Parità CLI per query e misure, test della selezione solo nella sessione nativa simulata.

Build Windows Release verde, CTest **50/50 in 25,49 s**, livelli/UTF-8/fuzz verdi. Resta render_view labels e la guida finale, da completare con il renderer della prossima tappa. Nessuna CI; M6 escluso.


## 2026-10-17 — M5, integrazione finale UI, etichette e scorciatoie

Collegati buffer ID/profondità, occlusione e selezione con clic, Ctrl/Shift e pressione prolungata; filtri persistenti per facce, spigoli, corpi e schizzi. Il popup in profondità include due corpi sovrapposti. Schizzi su faccia con camera allineata, proiezioni viola immutabili e coordinate locali anche nel cursore e negli overlay. E usa una faccia selezionata; F/H aprono i rispettivi dialoghi. Raccordo/smusso hanno selezione multipla, lista, anteprima, freccia e casella. Foro: un clic sulla faccia, due quote trascinabili e modificabili, diametro/profondità/passante. Solo OK modifica il documento; il drag resta anteprima. Pannelli dichiarati in Clay e colori/misure nei token. Timeline con icone Lucide e modifica delle nuove feature.

render_view labels=true disegna nomi stabili con richiami e restituisce entity_labels; geometry_query, measure e selection_get sono testati con il bridge reale. Guida incorporata aggiornata con flusso MCP completo: interrogazione dei quattro verticali, raccordo R2, schizzo/cilindro appoggiato, foro Ø8 a 10 dai bordi, cambio quote e render etichettato. Documento e PNG in docs/examples/m5/. Corpo principale finale: **117315,04440784613 mm³**. Alias compatti derivati dai nomi semantici per mantenere leggibili le etichette; conservati gli alias originari. L'audit aggiuntivo verifica tutti i quattro lati del prisma e distingue interno/esterno di una rivoluzione cava dopo cambio quota: MakeWire può sostituire i vertici e le TShape, perciò la curva OCCT condivisa e il suo intervallo mantengono l'identità dell'entità sorgente.

La suite completa ha trovato due regressioni: il test di visibilità campionava un solido sotto hover e il picking della faccia intercettava un profilo mostrato esplicitamente. Il controllo di visibilità sposta ora il puntatore fuori dal viewport; i profili visibili mantengono la priorità, disattivabile dal filtro Schizzi. Corretto anche Esc del popup, che impediva il clic successivo sul ribbon. La guida estesa supera il limite del singolo letterale MSVC: generazione per righe, con identico contenuto verificato dal test della risorsa MCP.

Verifica finale Windows: **build Release verde, CTest 51/51 in 35,64 s**, livelli e UTF-8 verdi, fuzz **260 diretti + 222 MCP**, senza crash. UI nei due temi a 1440×900 e 1280×720: hover faccia/spigolo, Ctrl, pressione lunga, Crea schizzo e camera, E/F/H, drag del raggio e delle quote, digitazione, conferma e volume. Nessuna CI, push, golden sostituita o istanza dell'utente chiusa. Binari in build/agent-m5/apps/. Rapporto docs/m5-validation.md. **M5 completato; M6 non iniziato.**

### Confronti 11–16, una riga per coppia

32 coppie in docs/screenshots/m5-confronto/, rigenerabili con tools/m5_compare.py. Il riferimento è a sinistra e la cattura nativa a destra. Uguali/differenze sotto riguardano i punti richiesti; italiano e icone Lucide restano comuni a tutte le coppie. Il tema scuro è intenzionalmente confrontato con il riferimento chiaro. La cornice, la densità della griglia e l'inquadratura SagomaCad restano quelle dei traguardi precedenti.

| Coppia | Uguale | Differenze e correzioni |
|---|---|---|
| [11-faccia-light-1440](../docs/screenshots/m5-confronto/11-faccia-light-1440.png) | Riempimento tenue e bordo della faccia al passaggio. | Colore del solido e cornice propri; stessa evidenziazione distinta dallo spigolo. Bordo faccia 1,4 px e riempimento separato. Tema chiaro. Altezza 900: pannello completo. |
| [11-faccia-light-1280](../docs/screenshots/m5-confronto/11-faccia-light-1280.png) | Riempimento tenue e bordo della faccia al passaggio. | Colore del solido e cornice propri; stessa evidenziazione distinta dallo spigolo. Bordo faccia 1,4 px e riempimento separato. Tema chiaro. Altezza 720: campi e footer accessibili. |
| [11-faccia-dark-1440](../docs/screenshots/m5-confronto/11-faccia-dark-1440.png) | Riempimento tenue e bordo della faccia al passaggio. | Colore del solido e cornice propri; stessa evidenziazione distinta dallo spigolo. Bordo faccia 1,4 px e riempimento separato. Tema scuro. Altezza 900: pannello completo. |
| [11-faccia-dark-1280](../docs/screenshots/m5-confronto/11-faccia-dark-1280.png) | Riempimento tenue e bordo della faccia al passaggio. | Colore del solido e cornice propri; stessa evidenziazione distinta dallo spigolo. Bordo faccia 1,4 px e riempimento separato. Tema scuro. Altezza 720: campi e footer accessibili. |
| [11-spigoli-light-1440](../docs/screenshots/m5-confronto/11-spigoli-light-1440.png) | Spigolo al passaggio con tratto chiaro più spesso. | Scala e forma della scatola proprie; Ctrl/Shift verificati separatamente. Tratto hover 3 px, distinto dalla selezione blu. Tema chiaro. Altezza 900: pannello completo. |
| [11-spigoli-light-1280](../docs/screenshots/m5-confronto/11-spigoli-light-1280.png) | Spigolo al passaggio con tratto chiaro più spesso. | Scala e forma della scatola proprie; Ctrl/Shift verificati separatamente. Tratto hover 3 px, distinto dalla selezione blu. Tema chiaro. Altezza 720: campi e footer accessibili. |
| [11-spigoli-dark-1440](../docs/screenshots/m5-confronto/11-spigoli-dark-1440.png) | Spigolo al passaggio con tratto chiaro più spesso. | Scala e forma della scatola proprie; Ctrl/Shift verificati separatamente. Tratto hover 3 px, distinto dalla selezione blu. Tema scuro. Altezza 900: pannello completo. |
| [11-spigoli-dark-1280](../docs/screenshots/m5-confronto/11-spigoli-dark-1280.png) | Spigolo al passaggio con tratto chiaro più spesso. | Scala e forma della scatola proprie; Ctrl/Shift verificati separatamente. Tratto hover 3 px, distinto dalla selezione blu. Tema scuro. Altezza 720: campi e footer accessibili. |
| [11-seleziona-altro-light-1440](../docs/screenshots/m5-confronto/11-seleziona-altro-light-1440.png) | Due corpi sovrapposti e lista in profondità vicino al cursore su pressione lunga. | Lista italiana con ID locali; scheda Parents non esposta. Esc chiude il popup e rende di nuovo cliccabile Crea schizzo. Tema chiaro. Altezza 900: pannello completo. |
| [11-seleziona-altro-light-1280](../docs/screenshots/m5-confronto/11-seleziona-altro-light-1280.png) | Due corpi sovrapposti e lista in profondità vicino al cursore su pressione lunga. | Lista italiana con ID locali; scheda Parents non esposta. Esc chiude il popup e rende di nuovo cliccabile Crea schizzo. Tema chiaro. Altezza 720: campi e footer accessibili. |
| [11-seleziona-altro-dark-1440](../docs/screenshots/m5-confronto/11-seleziona-altro-dark-1440.png) | Due corpi sovrapposti e lista in profondità vicino al cursore su pressione lunga. | Lista italiana con ID locali; scheda Parents non esposta. Esc chiude il popup e rende di nuovo cliccabile Crea schizzo. Tema scuro. Altezza 900: pannello completo. |
| [11-seleziona-altro-dark-1280](../docs/screenshots/m5-confronto/11-seleziona-altro-dark-1280.png) | Due corpi sovrapposti e lista in profondità vicino al cursore su pressione lunga. | Lista italiana con ID locali; scheda Parents non esposta. Esc chiude il popup e rende di nuovo cliccabile Crea schizzo. Tema scuro. Altezza 720: campi e footer accessibili. |
| [12-schizzo-su-faccia-light-1440](../docs/screenshots/m5-confronto/12-schizzo-su-faccia-light-1440.png) | Camera allineata, griglia, solido di contesto e bordi proiettati. | Cerchio Ø10 per il cilindro di accettazione invece del rettangolo; proiezioni viola. Frame della faccia in disegno, cursore e solver; riferimento immutabile. Tema chiaro. Altezza 900: pannello completo. |
| [12-schizzo-su-faccia-light-1280](../docs/screenshots/m5-confronto/12-schizzo-su-faccia-light-1280.png) | Camera allineata, griglia, solido di contesto e bordi proiettati. | Cerchio Ø10 per il cilindro di accettazione invece del rettangolo; proiezioni viola. Frame della faccia in disegno, cursore e solver; riferimento immutabile. Tema chiaro. Altezza 720: pannello adattato; tavolozza schizzo scorrevole. |
| [12-schizzo-su-faccia-dark-1440](../docs/screenshots/m5-confronto/12-schizzo-su-faccia-dark-1440.png) | Camera allineata, griglia, solido di contesto e bordi proiettati. | Cerchio Ø10 per il cilindro di accettazione invece del rettangolo; proiezioni viola. Frame della faccia in disegno, cursore e solver; riferimento immutabile. Tema scuro. Altezza 900: pannello completo. |
| [12-schizzo-su-faccia-dark-1280](../docs/screenshots/m5-confronto/12-schizzo-su-faccia-dark-1280.png) | Camera allineata, griglia, solido di contesto e bordi proiettati. | Cerchio Ø10 per il cilindro di accettazione invece del rettangolo; proiezioni viola. Frame della faccia in disegno, cursore e solver; riferimento immutabile. Tema scuro. Altezza 720: pannello adattato; tavolozza schizzo scorrevole. |
| [13-estrudi-faccia-light-1440](../docs/screenshots/m5-confronto/13-estrudi-faccia-light-1440.png) | Profilo da faccia, ordine dei campi, freccia, casella e anteprima. | Faccia superiore intera e distanza 10 invece della porzione a distanza zero. E e OK accettano la faccia; riuso dei profili M4 conservato. Tema chiaro. Altezza 900: pannello completo. |
| [13-estrudi-faccia-light-1280](../docs/screenshots/m5-confronto/13-estrudi-faccia-light-1280.png) | Profilo da faccia, ordine dei campi, freccia, casella e anteprima. | Faccia superiore intera e distanza 10 invece della porzione a distanza zero. E e OK accettano la faccia; riuso dei profili M4 conservato. Tema chiaro. Altezza 720: campi e footer accessibili. |
| [13-estrudi-faccia-dark-1440](../docs/screenshots/m5-confronto/13-estrudi-faccia-dark-1440.png) | Profilo da faccia, ordine dei campi, freccia, casella e anteprima. | Faccia superiore intera e distanza 10 invece della porzione a distanza zero. E e OK accettano la faccia; riuso dei profili M4 conservato. Tema scuro. Altezza 900: pannello completo. |
| [13-estrudi-faccia-dark-1280](../docs/screenshots/m5-confronto/13-estrudi-faccia-dark-1280.png) | Profilo da faccia, ordine dei campi, freccia, casella e anteprima. | Faccia superiore intera e distanza 10 invece della porzione a distanza zero. E e OK accettano la faccia; riuso dei profili M4 conservato. Tema scuro. Altezza 720: campi e footer accessibili. |
| [14-raccordo-light-1440](../docs/screenshots/m5-confronto/14-raccordo-light-1440.png) | Lista multipla, raggio costante, maniglia sullo spigolo, casella e anteprima. | Quattro verticali R3 invece di due orizzontali a zero; campi M5. Quattro righe visibili, freccia con contorno, nomi dei campi nascosti. Tema chiaro. Altezza 900: pannello completo. |
| [14-raccordo-light-1280](../docs/screenshots/m5-confronto/14-raccordo-light-1280.png) | Lista multipla, raggio costante, maniglia sullo spigolo, casella e anteprima. | Quattro verticali R3 invece di due orizzontali a zero; campi M5. Quattro righe visibili, freccia con contorno, nomi dei campi nascosti. Tema chiaro. Altezza 720: campi e footer accessibili. |
| [14-raccordo-dark-1440](../docs/screenshots/m5-confronto/14-raccordo-dark-1440.png) | Lista multipla, raggio costante, maniglia sullo spigolo, casella e anteprima. | Quattro verticali R3 invece di due orizzontali a zero; campi M5. Quattro righe visibili, freccia con contorno, nomi dei campi nascosti. Tema scuro. Altezza 900: pannello completo. |
| [14-raccordo-dark-1280](../docs/screenshots/m5-confronto/14-raccordo-dark-1280.png) | Lista multipla, raggio costante, maniglia sullo spigolo, casella e anteprima. | Quattro verticali R3 invece di due orizzontali a zero; campi M5. Quattro righe visibili, freccia con contorno, nomi dei campi nascosti. Tema scuro. Altezza 720: campi e footer accessibili. |
| [15-smusso-light-1440](../docs/screenshots/m5-confronto/15-smusso-light-1440.png) | Lista, distanza uguale, maniglia e anteprima geometrica. | Quattro verticali D3 invece di uno a zero; campi M5. Campo senza testo interno sporgente e anteprima verificata. Tema chiaro. Altezza 900: pannello completo. |
| [15-smusso-light-1280](../docs/screenshots/m5-confronto/15-smusso-light-1280.png) | Lista, distanza uguale, maniglia e anteprima geometrica. | Quattro verticali D3 invece di uno a zero; campi M5. Campo senza testo interno sporgente e anteprima verificata. Tema chiaro. Altezza 720: campi e footer accessibili. |
| [15-smusso-dark-1440](../docs/screenshots/m5-confronto/15-smusso-dark-1440.png) | Lista, distanza uguale, maniglia e anteprima geometrica. | Quattro verticali D3 invece di uno a zero; campi M5. Campo senza testo interno sporgente e anteprima verificata. Tema scuro. Altezza 900: pannello completo. |
| [15-smusso-dark-1280](../docs/screenshots/m5-confronto/15-smusso-dark-1280.png) | Lista, distanza uguale, maniglia e anteprima geometrica. | Quattro verticali D3 invece di uno a zero; campi M5. Campo senza testo interno sporgente e anteprima verificata. Tema scuro. Altezza 720: campi e footer accessibili. |
| [16-foro-light-1440](../docs/screenshots/m5-confronto/16-foro-light-1440.png) | Faccia selezionata, cilindro rosso, quote dai bordi e campi diametro/estensione/profondità. | Ø8 passante a 10 dai bordi invece di Ø11 cieco; campi del foro semplice M5. Quote distanziate, drag e digitazione verificati; Passante nel viewport. Tema chiaro. Altezza 900: pannello completo. |
| [16-foro-light-1280](../docs/screenshots/m5-confronto/16-foro-light-1280.png) | Faccia selezionata, cilindro rosso, quote dai bordi e campi diametro/estensione/profondità. | Ø8 passante a 10 dai bordi invece di Ø11 cieco; campi del foro semplice M5. Quote distanziate, drag e digitazione verificati; Passante nel viewport. Tema chiaro. Altezza 720: campi e footer accessibili. |
| [16-foro-dark-1440](../docs/screenshots/m5-confronto/16-foro-dark-1440.png) | Faccia selezionata, cilindro rosso, quote dai bordi e campi diametro/estensione/profondità. | Ø8 passante a 10 dai bordi invece di Ø11 cieco; campi del foro semplice M5. Quote distanziate, drag e digitazione verificati; Passante nel viewport. Tema scuro. Altezza 900: pannello completo. |
| [16-foro-dark-1280](../docs/screenshots/m5-confronto/16-foro-dark-1280.png) | Faccia selezionata, cilindro rosso, quote dai bordi e campi diametro/estensione/profondità. | Ø8 passante a 10 dai bordi invece di Ø11 cieco; campi del foro semplice M5. Quote distanziate, drag e digitazione verificati; Passante nel viewport. Tema scuro. Altezza 720: campi e footer accessibili. |


## 2026-10-17 — Identità visiva, infrastruttura e tavole della fase 9

Scelte esplicite dell’utente: riferimento SolidCraft al posto degli screenshot di ispirazione, tema chiaro, verde pistacchio e densità compatta. Riferimenti e regole di approvazione in `docs/design/README.md`. Importati senza alterazioni 268 SVG, palette, alias e licenze dal commit SolidCraft `72145d4aba05eab66e7a49b383bee9c9f5764da3`; import ripetibile con verifica byte per byte. Renderer C++/lunasvg per ruoli, alias, dimensione DPI, cache texture e placeholder; GAP cancella fino alla trasparenza, compreso il riempimento della propria forma quando è un bordo. Test di rasterizzazione su tutti i master nei due temi.

Catalogo: 37 comandi unici con icona/accento, due fallback Lucide espliciti. Parità generata e controllata senza riscrittura in modalità `--check`. Tema: 77 token registrati, accento UI chiaro #93c572/scuro #b1ce8a; contrasto testo 12.78:1/11.74:1, secondario 5.49:1/6.51:1. Finestre attraverso `ui/frame.cpp`; pannelli dei comandi e tavolozza Schizzo dichiarati in Clay, dialog limitati dai token e finestre flottanti centrate. Test con coordinate fisse della ricerca aggiornato al rettangolo reale, conservando controlli di digitazione/undo. Corretto anche un sormonto visivo dell’intestazione sul pulsante di compressione del browser.

Nuovo `sagomacad-cli design-board`, anche MCP `design_board`: tavole PNG 1600×1800 con tutti i token, catalogo 16/24/32 px e ruoli disabilitati, ribbon, browser, timeline e dialog. Aree della tavola dichiarate in Clay. Tavole nei due temi ispezionate; verificata anche scala 2. Screenshot shell/finestra parametri a 1440×900 e 1280×720 nei due temi controllati.

Verifica locale Windows: build Release isolata in `build/agent-phase9` (CLI in agent-root occupato, lasciato attivo); **57/57 ctest** passati in 40.24 s, controllo livelli, import SVG, parità, regola finestre e UTF-8 verdi. Fuzz: 260 casi su 42 comandi e 222 casi MCP. Test MCP della tavola verifica immagini, argomenti invalidi e invarianza del documento/storia. Nessuna verifica o esecuzione CI remota.

Resta aperta l’approvazione dell’utente sulle due tavole: non sono state copiate in `docs/design/approvate/`. Solo dopo la conferma si applicano le icone a ribbon/browser/timeline/menu (9.8), si completa la documentazione e si verifica la fase. Branch `phase/9`, nessun merge su main.


## 2026-10-17 — Correzione degli sfondi del tema chiaro

Precisazione dell’utente: bianco puro in tutta l’app, pistacchio solo come accento. Corretti sfondo generale, viewport, ribbon, pannelli, intestazioni, dialog e campi; testi e bordi neutri. Aggiunto controllo dei token per sfondi bianchi e neutrali senza dominante verde. Contrasto testo 14.55:1, secondario 5.74:1. Tavola chiara rigenerata; quattro screenshot nei due temi e alle due dimensioni ispezionati. Build Release e 57/57 test locali passati. Resta l’approvazione delle tavole prevista dal task 9.7.


## 2026-10-17 — Chiusura fase 9 dopo l’approvazione

L’utente conferma «ok ci siamo» sull’app con sfondi bianchi: tavole chiara/scura archiviate in `docs/design/approvate/`. Applicato il renderer comune SolidCraft a ribbon (26 px), menu e palette (16 px), browser (16 px) e timeline (20 px), con cache per tema/accento/pixel e ruoli passivi per disabilitati. Lucide resta per navigazione e fallback espliciti. Verificati lookup del catalogo e master assegnati; corretta la resa delle etichette disabilitate. La palette mantiene righe compatte.

Documentata la procedura per master originali in `assets/icons/sagomacad/svg/`, separati dall’import immutato SolidCraft: il compilatore include la cartella e rifiuta collisioni con master/alias importati, con test. Colori solo da palette/token, approvazione esplicita delle immagini e fabbrica unica delle finestre documentati.

Verifica di uscita Windows: build Release in `build/agent-phase9`; **57/57 ctest passati in 40.94 s**, livelli, import byte per byte, parità, regola finestre, UTF-8 e fuzz verdi (260 casi comandi, 222 MCP). Otto screenshot Solido/Schizzo a 1440×900 e 1280×720 nei due temi, palette dei comandi e ribbon a DPI 2 ispezionati contro le tavole approvate. Nessuna golden preesistente modificata. Lavoro della fase 9 concluso; la fase 10 richiede il proprio PRP e i relativi prerequisiti.

## 2026-10-18 — Correzioni M5, visibilità

Registrata fase 9.5 prima della 10. Schizzi visibili per default; preferenza locale opzionale, comandi preferences.get/set e checkbox File → Preferenze. Occultamento nello stesso undo della feature. Test rettangolo/cerchio separati, riuso, persistenza e MCP; test preesistenti di occultamento ora attivano esplicitamente la preferenza e il reset visibilità verifica il nuovo default. Build Release, livelli e 58/58 ctest verdi (40.62 s). Restano operazione automatica e quote sui tratti nella stessa fase.

### Operazione automatica estrusione

Decisione condivisa motore/UI con contatto esatto e volume di intersezione OCCT. Senza operation esplicita: contatto verso esterno Unisci, ingresso nel materiale Taglia, assenza contatto Nuovo corpo. Scelta manuale e feature riaperta restano ferme; suggest in sola lettura da CLI/MCP. Dodici casi sulle sei facce con i due segni verificano un solo corpo, volume e area esterna GProp senza superficie interna; anche profili a contatto/isolati e override. I vecchi casi che richiedono corpi indipendenti ora lo dichiarano esplicitamente. Corretta inoltre la prima scrittura delle preferenze se il file non esiste. Build/livelli/parità/frame e 59/59 ctest verdi (40.66 s).

### Quote sui tratti

Rimosso il pannello unico: input distinti ancorati a descrittori SketchMeasure prodotti dal motore; quote con linee di riferimento, tratto attivo pistacchio, Tab/Invio e vincoli solo per misure digitate. Doppio clic apre un input selezionato sulla quota, Invio modifica, Esc annulla. Sistemati focus fra finestre sovrapposte, primo clic degenere, misura del raggio bloccata e separazione fra quota angolare e lunghezza. I descrittori comuni valgono per nuovi strumenti senza pannelli ad hoc. ui.inspect espone i rettangoli delle quote. Sedici scenari mouse/tastiera da JSON, 48 PNG (disegno/creazione/modifica, 4 strumenti, 2 temi, 2 dimensioni), campionamento del colore accento sui tratti, vincoli persistenti e roundtrip. Confrontati con riferimenti 40/41 e tavole proprie. Suite completa 60/60 verde (46.99 s), livelli e regola frame verdi; successivo controllo mirato dopo aver limitato l’ancoraggio della quota di altezza ai soli segmenti collegati. Resta la verifica finale della fase correttiva.

### Verifica finale correzioni M5 (fase 9.5)

Build Release Windows in build/agent-phase9 e **61/61 ctest passati in 49.19 s**. Aggiunto controllo del clic reale sul cerchio inutilizzato, E e OK della seconda estrusione. Test nativo autenticato m5_control: preferenze, profili, rettangoli quote, screenshot e unione da faccia laterale. Scatola 10×20×30 + 7 mm: un corpo, 10200 mm³ e 2900 mm² esterni; dodici casi su facce/segni e override. Ultima correzione visiva: bordo del checkbox disattivato leggibile sul bianco, sempre da token. Livelli, frame rule, UTF-8 (229 sorgenti prima degli ultimi documenti), parità e fuzz verdi: 278 casi comandi, 45 comandi, 237 MCP, 272 rifiutati e 6 vuoti validi. Sedici scenari quote generano 48 PNG; quattro screenshot preferenze. Otto confronti disegno/modifica rettangolo nei due temi e alle due dimensioni, più due preferenze, archiviati in docs/design/verifica-m5 con nota di confronto ai riferimenti 40/41 e alle tavole. Golden approvate intatte. Aggiornati protocollo, guida eseguibile MCP, UI design, PRP, tasks e handoff. Nessun lavoro della fase 10 avviato.

## 2026-10-18 — Booleane senza giunzioni complanari

Verifica prima della correzione: la L di regressione (scatola 20×20×10, estrusione laterale 20 e rialzo 20×20×10 da schizzo su faccia) aveva già **1 corpo**, volume 12000 mm³, ma **14 facce**. Nessun errore di bersaglio o duplicazione di corpi. Screenshot prima della modifica acquisiti nei due temi a 1440×900 e 1280×720.

Aggiunto kernel/unify.cpp: ShapeUpgrade_UnifySameDomain con facce/spigoli, SafeInputMode e propagazione History() tramite lo stesso meccanismo delle booleane. Fusione, sottrazione, intersezione, fori dei profili e intersezioni di misura passano dalla pulizia. OCCT 8 non espone IsDone() su questa classe: controllo Shape/History non nulli e BRepCheck_Analyzer dopo Build; IsDone resta sui builder booleani. Standard_Failure o errore/nullità/invalidità della pulizia mantengono il risultato originale e i suoi nomi.

Test GProp e topologia: estrusione laterale **1 corpo, 6 facce, 8000 mm³**; L **1 corpo, 8 facce, 12000 mm³**, area 3600 mm²; cilindri coassiali con parete unica; foro passante da due tagli con 7 facce, volume 8000−40π e area 2800+32π; intersezione con 6 facce e 6000 mm³. Verificati alias many-to-one delle facce e dello spigolo esteso, eliminazione della vecchia faccia interna, schizzo sul piano fuso, roundtrip. Raccordo R2 sul bordo della L: modifica della prima estrusione 20→30, stesso riferimento da X=40 a X=50, Z=10 e lunghezza 20 invariati; volume 14000−20(4−π). Stessi flussi e conteggi verificati da MCP e CLI.

Build Release Windows e **63/63 ctest verdi in 51.86 s**; livelli, regola frame, UTF-8 e fuzz verdi (278 casi, 45 comandi, 237 MCP). Screenshot dopo la correzione senza giunzioni artificiali, nei due temi e due dimensioni, confrontati con quelli prima: docs/design/verifica-booleane/ contiene otto PNG, confronto, documento L.sagomacad e dettagli. Golden approvate intatte. Fase 10 non iniziata.

## 2026-10-18 — Vista Home dopo Fine schizzo

Il comando UI Fine schizzo avvia la transizione alla vista Home usando la durata del tema; conserva centro e zoom. Il test di interazione ora preme il pulsante reale e verifica chiusura dello schizzo, orientamento Home e inquadratura invariata. Screenshot ispezionati nei due temi a 1440×900 e 1280×720, in build/finish-home-evidence. Build Release e **63/63 ctest verdi in 51.80 s**; livelli e regola finestre verdi. Fase 10 non iniziata.

## 2026-10-18 — Comandi attivi evidenziati nei menu

Ribbon e tendine condividono lo stato selezionato, derivato dai pannelli e dagli strumenti attivi: Estrudi, Rivoluzione, Raccordo, Smusso, Foro, primitive, parametri, selezione, dialog file e strumenti di schizzo. Sfondo accent_soft e bordo pistacchio dai token, hover distinto; nessuna selezione residua dopo la chiusura. Funziona anche da scorciatoia e riapertura della feature. Test di interazione su Estrudi, Rivoluzione, Raccordo, Foro e cambio Linea/Cerchio/Rettangolo; controllo dei pixel del ribbon con il mouse nel viewport. Screenshot di ribbon e tendina Crea nei due temi a 1440x900 e 1280x720 ispezionati in build/menu-selection-evidence. Build Release, 63/63 ctest, livelli, regola finestre e UTF-8 verdi. Golden approvate intatte; fase 10 non iniziata.

## 2026-10-19 — Viewport GPU, infrastruttura iniziale

Framebuffer OpenGL 3.3 con depth a 24 bit e MSAA fino a 4 campioni, texture risolta presentata dalla callback ImGui condivisa dalla shell e dagli snapshot. VBO per corpo: orbita e metadati senza ricaricare la geometria. Ripristino dello stato GL e distruzione prima del contesto SDL; ripiego CPU con diagnostica. Build Release isolata `build/agent-phase10`, 64/64 ctest in 53.07 s; livelli, frame e UTF-8 verdi. Screenshot `build/phase10-module-{light,dark}.png` controllati nei due temi/dimensioni. Griglia, anteprime, luci e linee avanzate rimangono ai task successivi del PRP. La validazione 10.2 omette dimensions obbligatorio: richiesta decisione per correggere il comando.

Luce del task 10.2 implementata (key/fill/specular, sRGB, normale nulla piatta e tratteggio sezione disattivato di default), con attribuzione SolidCraft. Build Release e test gl_viewport verdi, inclusa conservazione esatta dei colori piatti nei due temi. Suite con nuova luce: 63/64; sketch_interaction cerca il colore prodotto dalla formula CPU precedente (righe 114–116). Controllo conservato. Il comando di validazione del PRP fallisce per dimensions mancante; in attesa di decisione utente sulla correzione della validazione. Screenshot supplementari check-shading.png e check-shading-dark.png ispezionati. Nessun merge su main.

2026-10-19: correzione validazione 10.2 autorizzata; build Release, 64/64 ctest in 50.42 s. Screenshot nei due temi/dimensioni verificati. Il controllo di visibilita riconosce il colore del corpo senza dipendere dalla vecchia formula CPU.

## 2026-10-19 - Linee, spigoli e sagome GPU

Linee espanse in screen space con clipping near; test su 1.2/3/5 px, due distanze e risoluzioni. Depth bias delle facce: continuita dei bordi a tre inclinazioni, inclusi .001 e .035 rad. Normali per vertice prodotte dal kernel; sagome da N dot V e derivate, con direzione prospettica per frammento. Il test senza spigoli ha trovato una tassellazione troppo grossolana per il contorno: deflessione angolare ridotta da .35 a .10 rad. Flags separati per facce/spigoli/sagome. Build Release, 67/67 ctest in 53.31 s; livelli e UTF-8 verdi. Snapshot nei due temi/dimensioni controllati: check-lines, check-grazing, check-silhouette. Script riproducibili in docs/examples/m5/. Restano griglia, anteprime, handle e scenari come da PRP.

## 2026-10-19 - Griglia adattiva GPU

Piano infinito nel fragment shader, sfondo uniforme, livelli a fattore cinque e dissolvenza continua dei pesi. Antialiasing e filtro dei livelli sotto risoluzione, attenuazione con inclinazione/distanza. Griglia dopo il depth del modello, con bias coplanare: test pixel dall’alto e dal basso nei due temi senza infiltrazioni nel solido. Piano degli schizzi supportati e passo condiviso con lo snap. Token per spessori e piani origine; percorso CPU conservato come ripiego. Build Release, 68/68 ctest in 57.41 s, livelli verdi. Dieci screenshot a distanze 15/45/140/450/1400 nei due temi, due dimensioni, confrontati con 43/44 e tavole approvate; vicino/lontano mostrati in chat. Scenario griglia-zoom predisposto per il runner del task 10.10.

## 2026-10-19 - Anteprime, selezione e tratti sulla GPU

Layer GPU con cache per anteprime, facce/spigoli selezionati e sotto il mouse, profili e tratti di schizzo. Prepass di profondita delle anteprime trasparenti: la griglia non attraversa il volume, i colori del corpo sottostante restano disponibili per il compositing. Riempimenti prima di tutti i tratti, allineamento al pixel per linee assiali; risolte due regressioni reali M5 senza modificare i controlli. Picking ID dalla stessa tessellazione della vista, ricostruzione differita mentre la camera si muove. Build Release, 69/69 ctest in 54.22 s; gl_layers verifica griglia sotto trasparenza e selezione posteriore occlusa. Picking/M5/model interaction, livelli/frame/UTF-8 verdi; screenshot M4/M5 nei due temi e dimensioni ispezionati.

## 2026-10-19 - Snapshot, render_view e prestazioni

Metadata renderer/MSAA esposti da ui.inspect e render_view; test MCP reale su iso/top/front/right a 640x400, PNG e argomenti invalidi verificati anche con pytest. Benchmark della stessa pipeline in contesto nascosto: sfera sintetica di 200000 triangoli, 1440x900, MSAA4, 12 frame di warm-up e 120 frame di orbita sincronizzati con glFinish. NVIDIA GeForce RTX 3070 Ti, OpenGL 3.3 NVIDIA 617.42: 3038.21 fps medi, 2110.15 minimi, p95 2483.24; un solo upload. Sono tempi del renderer, senza il limite di frame della finestra. Build Release, 70/70 ctest in 55.01 s; pytest render_view 1/1.

### Fase 10 - handle e input UI
Registro per frame dei controlli visibili, input deterministico e bridge MCP ui_inspect/ui_call. Test reali su dialogo primitive, volume 40000 mm3, campi chiusi, resize e input ostili. Release: 71/71 test (55.69 s); pytest handles 1/1; livelli validi. Screenshot M5 verificati nei due temi e dimensioni.

### Fase 10 - runner scenari JSON
CLI scenario run usa Shell nascosta, input reali, attese a frame e tempo virtuale. Controlli numerici motore, screenshot e riferimenti ai risultati delle chiamate. Fixture storici preservati. Release 74/74 (60.59 s), test input ostili del runner e livelli validi. Scenario rettangolo: 4 entita, 2 quote, 0 GDL; griglia nei due temi.
