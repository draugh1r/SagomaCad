# Handoff — Viewport GPU e scenari UI
Aggiornato: 2026-10-19 20:56 UTC · Agente: Codex · Branch: main

## Stato
La fonte autorevole resta `docs/.plan/tasks.json`. Il criterio di uscita della fase 10 e documentato in `docs/design/verifica-viewport/README.md` e nel devlog; checkpoint e merge sono su main. Nessun codice lasciato a meta, nessun lavoro della fase 11 iniziato.

## Decisioni prese in sessione (e perché)
- L’utente ha sostituito gli screenshot di ispirazione della fase 9 con il riferimento SolidCraft in `../solidcraft-ref`. Questa eccezione riguarda quella fase, non i task 0 successivi.
- Tema predefinito chiaro: **bianco puro in tutta l’app**, testi/bordi neutri, pistacchio esclusivamente come accento. La prima proposta con superfici verdi è stata corretta su richiesta dell’utente.
- L’utente ha confermato «ok ci siamo» dopo aver aperto l’app corretta. Le tavole sono archiviate in `docs/design/approvate/`. Sono il riferimento per gli screenshot successivi; non vanno rigenerate automaticamente.
- Le icone importate mantengono gli accenti semantici di categoria; il pistacchio della shell non ricolora tutte le icone. SVG SolidCraft immutati, provenienza e licenze in `assets/icons/solidcraft/` e `ATTRIBUTION.md`. I master originali futuri hanno cartella separata `assets/icons/sagomacad/svg/`, con collisioni rifiutate dal compilatore.

## Trappole e vicoli ciechi
- `bash` nella PATH puntava al launcher WSL e rimaneva bloccato. Gli script del framework sono stati eseguiti con `C:/Program Files/Git/bin/bash.exe`.
- CMake/ctest non erano nella PATH: eseguibili in `C:/Users/User/AppData/Roaming/Python/Python313/site-packages/cmake/data/bin/`.
- Un CLI preesistente in `build/agent-root` era occupato da un processo attivo. La build isolata della sessione è `build/agent-phase9`, con dipendenze vcpkg già disponibili in `build/agent-root/vcpkg_installed` e toolchain nel repository vcpkg adiacente. I processi preesistenti non sono stati interrotti.
- GAP non è bianco di sfondo: cancella forme precedenti e, se è uno stroke, anche il fill della propria forma. Il preprocessore e i test coprono entrambi i casi.
- Il test della ricerca della palette legge il rettangolo `palette_search`, evitando coordinate fisse dopo la centratura delle finestre.

## File da leggere
- `docs/design/README.md`: scelte, approvazione e procedura per icone/colori.
- `docs/.plan/tasks.json` e PRP della fase richiesta: dipendenze e validazioni.
- `src/ui/icons.*`, `tools/palette_to_json.py`: renderer e compilazione degli asset.
- `src/ui/frame.*`, `src/ui/layout.cpp`: fabbrica finestre e aree Clay.
- `docs/ui-design.md`, `log/devlog.md`: comportamento della shell ed evidenze.

## Correzioni M5 richieste prima della fase 10
- Schizzi visibili per default; preferences.get/set controllano l’occultamento al momento della creazione, salvato nello stesso undo della feature. `visible: null` torna visibile: vecchi scenari di occultamento ora attivano la preferenza o impostano false.
- L’operazione automatica usa il contatto fra profilo e corpo e il volume di intersezione della spazzata, non il solo segno numerico. Sulle facce minime un valore positivo può puntare verso l’esterno grazie alla normale orientata. Le feature salvano l’operazione scelta; riaprire una feature la conserva. Gli esempi di cilindri indipendenti ora dichiarano operation=new.
- Gli input live sono finestre sovrapposte distinte: `NoFocusOnAppearing` impedisce che la seconda rubi il focus alla prima. Anche la geometria inizialmente degenere deve produrre descrittori di misura per accettare subito testo. Non rimuovere queste due protezioni.
- `SketchMeasure` è il contratto per i futuri strumenti: quote/ancoraggi/riferimenti nel motore, un solo ciclo di rendering nella shell. Scenari JSON M5 dedicati, senza anticipare l’infrastruttura generale prevista nella fase 10.
- Evidenze e confronto: `docs/design/verifica-m5/README.md`; screenshot completi sotto `build/agent-phase9/m5-dimensions` e `m5-preferences`. Le golden approvate sono rimaste intatte.

## Giunzioni dopo booleane
- Il caso L aveva un corpo corretto ma 14 facce; non era un errore di bersaglio. `src/kernel/unify.cpp` pulisce i risultati con History, propagando alias e cancellazioni. SafeInputMode e fallback preservano il risultato grezzo se la pulizia fallisce.
- OCCT 8 non ha IsDone per ShapeUpgrade_UnifySameDomain: vengono controllati forma, storia e validità BRep. Non aggiungere una chiamata a un metodo assente.
- Il test boolean_unify verifica anche alias di spigoli allungati e raccordo sulla L che segue una modifica 20→30; boolean_unify_agents ripete tramite MCP e CLI. Confronti e modello in docs/design/verifica-booleane/. Ultima verifica documentata nel devlog: 63/63 ctest.

## Dettagli della fase 10
- Build isolata `build/agent-phase10`; applicazione e CLI in `apps/sagomacad/` e `apps/sagomacad-cli/`. I processi di altre build non sono stati interrotti. Per i test nativi usare `--settings-file` e `--session-file` temporanei.
- L'utente ha autorizzato dimensions={width:"40 mm",depth:"30 mm",height:"20 mm"} nel comando di validazione 10.2; sono misure di esempio, non valori imposti alla UI. Ha autorizzato anche l'adeguamento del controllo di visibilita che dipendeva dalla vecchia formula di luce CPU.
- Griglia uniforme senza gradiente, bianca nel tema chiaro, livelli ogni cinque passi e snap coerente. Il confronto dal basso ha trovato un difetto sfuggito ai test assiali: il piano della griglia copriva le pareti. Il task di correzione 102 aggiunge la regressione obliqua; grid.frag usa depth=1 e GL_EQUAL sui campioni liberi, conservando facce e prepass delle anteprime.
- Nel test delle quote su faccia, scrivere lo stesso numero del valore live non vincolava la quota. CallbackEdit ora registra l'intenzione di digitazione anche quando il testo finale e identico. Non tornare al solo IsItemEdited. Le entita di riferimento della faccia sono incluse in sketch.inspect (quattro bordi piu quattro linee nuove).
- Gli handle esistono solo per elementi effettivamente disegnati. `primitive.add` va aperto da menu:CREA prima di toolbar:primitive.add. La shell di controllo usa input reali e tempo virtuale; i metodi sono documentati in docs/control-protocol.md. Scenari in docs/scenarios.md, un CTest per file JSON.
- `core/planar` e indipendente da doc/kernel/solver. Divide linee, cerchi e archi con intersezioni analitiche e percorre semispigoli. Le regioni intersecate hanno selettori opachi @+/@- con identita di entita e rami, mai coordinate; il kernel ricostruisce gli archi esatti. I contorni non divisi mantengono gli ID precedenti. Se una regione scompare la timeline segnala errore e prosegue: nessun ripiego sulla regione piu vicina.
- Il benchmark misura il renderer nascosto con glFinish, senza il limite della finestra. I numeri e le condizioni sono nel devlog e nelle evidenze; non descriverli come fps dell'intera UI.
- Golden e tavole approvate immutate. Immagini e JSON finali in docs/design/verifica-viewport; gli artefatti generati del runner sono ignorati in scenario-evidence/ o sotto build/.

## Per ripartire
La build Release verificata e `build/agent-phase10`. Il log dettagliato e `build/agent-phase10/Testing/Temporary/LastTest.log`; controlli finali in `build/phase10-tests.log`. La fase richiesta successiva e identificata in tasks.json; il relativo PRP descrive riferimenti e validazioni da usare. Questo handoff conserva il contesto tecnico, non un'autorizzazione a cambiare le tavole approvate.

Prima di agire leggi i file elencati sopra e verifica ogni affermazione sul codice: e contesto, non verita.
