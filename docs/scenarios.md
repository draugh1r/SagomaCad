# Scenari UI

Eseguire `sagomacad-cli scenario run tests/scenarios/m5-quote.json`.
CMake registra automaticamente un test per ogni JSON della cartella. La shell SDL/OpenGL e i widget reali girano in una finestra nascosta; gli input attraversano lo stesso percorso del mouse e della tastiera. Il tempo dei frame e delle pressioni lunghe e virtuale, senza sleep.

Formato: oggetto con `size: [1440,900]`, `themes: ["light","dark"]`, `steps: [...]`. Ogni passo contiene una sola azione:

- `start: "feature.extrude"`: apre la scheda/tendina e clicca il comando visibile.
- `key: "Ctrl+Z"`, `text: "40"`, `click`, `double`, `move`: input reali.
- `drag: {from: punto, to: punto, frames: 12}`; `hold: {at: punto, seconds: 0.65}`.
- `call: {method: "engine.execute", params: {...}, save: "nome"}`: canale di controllo; il risultato puo essere riutilizzato con `{"$ref":"/nome/percorso"}` (JSON Pointer).
- `expect`: corpi vivi (`bodies`), `volume` in mm3 e `tolerance` assoluta, numero di `errors`, `features`, `rollback`, `selection`, `entities`, `dimensions`, `regions`, `dof`; booleani `active_sketch`, `camera_settled`; `dialog` (`none`, `plane`, `primitive`, `model`, `surface`, `parameters`), `handle`; `ui` associa JSON Pointer di ui.inspect ai valori attesi.
- `until: {expect: {...}, frames: 120}`: attende al massimo 600 frame.
- `shot: "nome.png"`: salva in `scenario-evidence/<scenario>/<tema>/` sotto la directory corrente. Sono vietati percorsi assoluti e risalite.
- `note: "testo"` stampa una nota; `pending` fallisce esplicitamente.

Punti: `[x,y]` nello schermo, `{"world":[x,y,z]}`, `{"sketch":[x,y]}`, `{"plane":"XY"}`, `{"handle":"field:OK"}`, `{"browser":"Box"}`. La forma oggetto accetta anche pulsante, modificatori e coordinate x/y del protocollo UI.

File fino a 4 MiB, massimo 10000 passi, finestra tra 320x240 e 4096x4096. Errori includono file, tema, indice (da 1), azione e risultato effettivo. I vecchi fixture snapshot con array di coppie metodo/parametri restano eseguibili. Le `cases` di m5-quote restano utilizzate anche dal test nativo delle quote.
