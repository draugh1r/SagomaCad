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
