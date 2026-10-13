# Devlog

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
