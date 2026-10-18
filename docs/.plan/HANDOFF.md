# Handoff — Identità visiva e correzioni M5
Aggiornato: 2026-10-18 · Agente: Codex · Branch: main

## Stato
La fonte autorevole dello stato è `docs/.plan/tasks.json`. Nessun codice lasciato a metà nella fase 9; la verifica di uscita è documentata nell’ultima voce di `log/devlog.md`.

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
- La fase 9.5 ha milestone M5; mantiene i numeri delle fasi preesistenti e la fase 10 dipende dal suo task finale. Nessuna autorizzazione a iniziare la fase 10 in questa richiesta.
- Schizzi visibili per default; preferences.get/set controllano l’occultamento al momento della creazione, salvato nello stesso undo della feature. `visible: null` torna visibile: vecchi scenari di occultamento ora attivano la preferenza o impostano false.
- L’operazione automatica usa il contatto fra profilo e corpo e il volume di intersezione della spazzata, non il solo segno numerico. Sulle facce minime un valore positivo può puntare verso l’esterno grazie alla normale orientata. Le feature salvano l’operazione scelta; riaprire una feature la conserva. Gli esempi di cilindri indipendenti ora dichiarano operation=new.
- Gli input live sono finestre sovrapposte distinte: `NoFocusOnAppearing` impedisce che la seconda rubi il focus alla prima. Anche la geometria inizialmente degenere deve produrre descrittori di misura per accettare subito testo. Non rimuovere queste due protezioni.
- `SketchMeasure` è il contratto per i futuri strumenti: quote/ancoraggi/riferimenti nel motore, un solo ciclo di rendering nella shell. Scenari JSON M5 dedicati, senza anticipare l’infrastruttura generale prevista nella fase 10.
- Evidenze e confronto: `docs/design/verifica-m5/README.md`; screenshot completi sotto `build/agent-phase9/m5-dimensions` e `m5-preferences`. Le golden approvate sono rimaste intatte.

## Giunzioni dopo booleane
- Il caso L aveva un corpo corretto ma 14 facce; non era un errore di bersaglio. `src/kernel/unify.cpp` pulisce i risultati con History, propagando alias e cancellazioni. SafeInputMode e fallback preservano il risultato grezzo se la pulizia fallisce.
- OCCT 8 non ha IsDone per ShapeUpgrade_UnifySameDomain: vengono controllati forma, storia e validità BRep. Non aggiungere una chiamata a un metodo assente.
- Il test boolean_unify verifica anche alias di spigoli allungati e raccordo sulla L che segue una modifica 20→30; boolean_unify_agents ripete tramite MCP e CLI. Confronti e modello in docs/design/verifica-booleane/. Ultima verifica documentata nel devlog: 63/63 ctest.

## Per ripartire
La build Release verificata è `build/agent-phase9`; applicazione e CLI sono nelle rispettive sottocartelle di `apps/`. La verifica della fase 9 comprendeva 57/57 ctest; la verifica M5 successiva è documentata in docs/design/verifica-m5/README.md e nell’ultima voce del devlog (61/61 ctest, livelli, parità, regola finestre, UTF-8 e fuzz). Gli screenshot di confronto della sessione sono sotto `build/phase9-final-*` e `build/check-ribbon.png`.

Prima di agire leggi i file elencati sopra e verifica ogni affermazione sul codice: è contesto, non verità.
