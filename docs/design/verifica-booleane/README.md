# Booleane: facce unificate e riferimenti stabili

Verifica del 18 ottobre 2026. Screenshot propri di SagomaCad, non golden approvate.

Riproduzione: scatola 20×20×10, estrusione laterale +20, schizzo rettangolare 20×20 sul piano superiore ed estrusione +10, sempre Unisci sul corpo iniziale. Lo schizzo è nascosto esplicitamente per isolare gli spigoli del solido.

**Prima:** un corpo, 14 facce, 12000 mm³. **Dopo:** un corpo, 8 facce, 12000 mm³, area esterna 3600 mm². Il bersaglio della booleana era già corretto: le giunzioni derivavano dalle facce complanari non unificate. I bordi che restano sono gli spigoli reali della L.

| Tema | Dimensione | Prima | Dopo |
|---|---|---|---|
| Chiaro | 1440×900 | [PNG](before-light-1440x900.png) | [PNG](after-light-1440x900.png) |
| Scuro | 1440×900 | [PNG](before-dark-1440x900.png) | [PNG](after-dark-1440x900.png) |
| Chiaro | 1280×720 | [PNG](before-light-1280x720.png) | [PNG](after-light-1280x720.png) |
| Scuro | 1280×720 | [PNG](before-dark-1280x720.png) | [PNG](after-dark-1280x720.png) |

Documento di prova: [L.sagomacad](L.sagomacad). Comandi riproducibili in `tests/scenarios/boolean-l.json`.

## Verifiche numeriche e riferimenti

- Scatola con estrusione laterale: 1 corpo, 6 facce, 8000 mm³.
- L: 1 corpo, 8 facce, 12000 mm³, area 3600 mm².
- Due cilindri coassiali: 3 facce totali, un’unica parete cilindrica, volume 500π mm³.
- Foro passante realizzato con due tagli successivi: 7 facce, parete cilindrica unica, volume 8000−40π mm³ e area 2800+32π mm².
- Intersezione: 6 facce, 6000 mm³.
- Gli alias delle due facce confluite risolvono alla stessa faccia; lo spigolo di bordo esteso conserva il riferimento precedente alla fusione.
- Raccordo R2 sullo spigolo esterno della L: cambiando la prima estrusione da 20 a 30 mm, lo stesso riferimento segue lo spigolo da X=40 a X=50, con lunghezza 20 mm e Z=10 invariati. Volume finale 14000−20(4−π) mm³. Anche lo schizzo sulla faccia unificata e il salvataggio/riapertura restano validi.

Test `boolean_unify` e `boolean_unify_agents`: i secondi verificano gli stessi risultati da MCP e CLI. La pulizia usa `History()` per propagare i nomi; un errore nella pulizia conserva il risultato originale della booleana. Questa versione di OCCT non fornisce `IsDone()` su `ShapeUpgrade_UnifySameDomain`: dopo `Build()` vengono controllati forma non nulla, storia e `BRepCheck_Analyzer`; `IsDone()` resta verificato sui costruttori delle booleane.
