# Disegno con il mouse — correzione M3

Screenshot ottenuti dal test UI `sketch_mouse_test`, con eventi reali di puntatore e tastiera, prima del clic di conferma:

- `mouse-line-preview.png`: segmento e valori di lunghezza/angolo che seguono il cursore;
- `mouse-rect-preview.png`: rettangolo con larghezza/altezza aggiornate dal mouse;
- `mouse-circle-preview.png`: cerchio con diametro aggiornato dal mouse;
- `mouse-rectangle-width-locked.png`: larghezza digitata 40, altezza ancora controllata dal mouse.

Le sottocartelle indicano tema e dimensioni: chiaro/scuro, 1440×900/1280×720. Confrontati con `docs/reference/fusion/quote-durante-disegno.png` e `modalita-schizzo.png`: anteprima immediata dal primo punto, misure vicino alla geometria, quota attiva sostituibile digitando, Tab per passare all'altra, conferma al secondo clic senza passaggi aggiuntivi. Le misure visualizzate non diventano quote vincolanti finché non vengono digitate.

```powershell
& .\build\agent-sketch-mouse\Release\sketch_mouse_test.exe docs/screenshots/m3-mouse/light-1440x900
& .\build\agent-sketch-mouse\Release\sketch_mouse_test.exe docs/screenshots/m3-mouse/light-1280x720 1280x720 light
& .\build\agent-sketch-mouse\Release\sketch_mouse_test.exe docs/screenshots/m3-mouse/dark-1440x900 1440x900 dark
& .\build\agent-sketch-mouse\Release\sketch_mouse_test.exe docs/screenshots/m3-mouse/dark-1280x720 1280x720 dark
```
