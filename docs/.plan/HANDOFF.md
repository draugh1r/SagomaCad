# Handoff — Identità visiva di SagomaCad
Aggiornato: 2026-10-17 · Agente: Codex · Branch: main

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

## Per ripartire
La build Release verificata è `build/agent-phase9`; applicazione e CLI sono nelle rispettive sottocartelle di `apps/`. La verifica finale comprende 57/57 ctest, livelli, import, parità, regola finestre, UTF-8 e fuzz (260 casi comandi, 222 MCP). Gli screenshot di confronto della sessione sono sotto `build/phase9-final-*` e `build/check-ribbon.png`.

Prima di agire leggi i file elencati sopra e verifica ogni affermazione sul codice: è contesto, non verità.
