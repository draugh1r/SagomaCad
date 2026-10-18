# Verifica delle correzioni M5 — 18 ottobre 2026

Evidenze della fase correttiva 9.5. Sono screenshot di verifica, non nuove golden approvate. I riferimenti privati Fusion sono rimasti in `docs/reference/fusion/`; nessuna loro immagine è stata copiata qui.

## Confronto con 40 e 41

| Riferimento comportamentale | Riscontro in SagomaCad |
|---|---|
| 40: larghezza sotto il tratto orizzontale e altezza separata a lato | `drawing-rect-*`: input separati, linee di estensione e frecce; nessun pannello comune |
| 40: quota segue il tratto, Tab cambia misura | Gli scenari spostano il mouse, digitano 40, Tab, 20; il tratto attivo usa il pistacchio, verificato anche sui pixel del PNG |
| 41: quota persistente e campo direttamente sulla quota | `editing-rect-*`: doppio clic e testo selezionato nel campo ancorato alla quota; Invio aggiorna il vincolo |
| Aspetto proprio approvato | Tema chiaro bianco, accento pistacchio; tema scuro e densità coerenti con le tavole del 17 ottobre |

Le otto immagini del rettangolo coprono disegno e modifica a **1440×900 e 1280×720**, nei **due temi**. Le due immagini `preferences-*` mostrano la preferenza disattivata e il controllo leggibile anche sul bianco.

La suite genera anche le immagini di creazione e quelle di linea, cerchio e arco: **48 PNG** sotto `build/agent-phase9/m5-dimensions`, più quattro PNG delle preferenze in `m5-preferences`. Gli scenari in `tests/scenarios/m5-quote.json` verificano 16 combinazioni di strumento/tema/dimensione, vincoli, Tab/Invio, editor in loco e serializzazione.

## Regressioni geometriche e di controllo

- Rettangolo e cerchio separati nello stesso schizzo: estrusione del rettangolo, schizzo ancora visibile, clic reale sul cerchio, E e conferma della seconda estrusione. Verificati inoltre undo e persistenza della preferenza.
- Scatola 10×20×30, estrusione di 7 mm dalla faccia laterale: un corpo, **10200 mm³**, area esterna **2900 mm²**. I test GProp su tutte le sei facce con entrambi i segni verificano volume e area, rilevando eventuali superfici interne; testati contatto/isolamento e scelta manuale.
- CLI e MCP condividono preferenze e suggerimento di operazione. `m5_control` ripete i flussi geometrici nell’app nativa attraverso il canale autenticato, controlla i rettangoli delle quote e cattura un PNG.

Verifica Windows Release: **61/61 ctest**, livelli, regola finestre, UTF-8, parità e fuzz verdi. Fuzz: 278 casi comandi, 237 casi MCP. Dettagli e motivazioni in `log/devlog.md` e nel PRP della fase.
