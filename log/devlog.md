# Devlog

## 2026-10-09 — M0

Creati i nove target CMake per i livelli, il manifest vcpkg, la shell SDL3/OpenGL con docking Dear ImGui, griglia e assi, temi a token e font Inter con licenza. La CLI produce PNG da un contesto OpenGL nascosto; il server TCP loopback implementa `auth`, `ui.inspect` e `ui.screenshot` con percorsi limitati alla radice di automazione. Aggiunti controllo degli include, test e workflow CI Windows/macOS/Linux.

Verifica locale Windows: build Release riuscita, CTest 3/3, `check_layers.py` verde. Snapshot scuro `docs/screenshots/m0.png` a 2880×1800 (277247 byte) e verifica visiva anche del tema chiaro a 960×600. Corretto il font, la barra superiore tagliata e la palette dei tab dopo la prima ispezione. Resta da eseguire la CI remota e da ottenere l'approvazione dell'utente per promuovere `m0.png` a prima golden image. M1 non iniziato.

## 2026-10-09 — Correzione M0

Rinominati progetto, target, binari, namespace e formato futuro in `sagomacad`; l'app mostra “SagomaCad”. Corrette le stringhe accentate, caricati i glifi Inter Latin-1/Extended, fissato il layout senza barre di docking e senza titolo del viewport. Collegato `sagomacad_kernel` a OpenCascade 8.0.1: box 2×3×4, volume GProp misurato 24. Aggiunti test UTF-8, glifi e geometria. Build Release Windows e CTest 6/6 verdi; `check_layers.py` verde. Esaminati i PNG scuro e chiaro a 1440×900 e 1280×720; `m0.png` è la variante scura 1440×900. Cache binaria vcpkg configurata nel workflow. CI remota ancora da avviare al momento di questa voce. M1 non iniziato.

Primo avvio CI: Linux si è fermato nella dipendenza vcpkg `libxcrypt` perché mancavano gli strumenti Autotools di sistema. Il workflow ora installa tali strumenti e le librerie di sviluppo OpenGL/X11 necessarie; su Windows usa il generatore Visual Studio e CTest Release. La matrice corretta è in fase di rilancio.

Secondo avvio CI: `libxcrypt` richiede anche `libltdl-dev` su Ubuntu. Aggiunta la dipendenza al workflow; nuova matrice in avvio.

Ultimo controllo locale: spostate le etichette delle sezioni nei token `src/ui/theme.hpp`; build Release, CTest 6/6 e `check_layers.py` verdi. Rigenerati e ispezionati i quattro PNG a 1440×900 e 1280×720 nei temi scuro e chiaro. La matrice GitHub Actions è ancora nella configurazione vcpkg; il controllo remoto riprenderà quando sarà disponibile l'esito.
