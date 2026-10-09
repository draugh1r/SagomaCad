# Devlog

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
