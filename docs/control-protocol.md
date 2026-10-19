# Protocollo di controllo M1

Avviare `sagomacad --control 7878 --control-token-file .private/control.token --automation-root screenshots`. Il server ascolta solo `127.0.0.1` e accetta JSON a riga singola. La prima richiesta di ogni connessione deve essere `{"id":1,"method":"auth","params":{"token":"..."}}`; una risposta riuscita contiene `result.authenticated: true`. Prima dell'autenticazione le altre richieste restituiscono `error.message`.

`ui.inspect` restituisce dimensioni logiche, tema, visibilità dei pannelli, griglia, assi e stato del layout fisso (`fixed_layout`, `tabs_visible`, `viewport.title_visible`). `ui.screenshot` accetta `{"path":"nome.png"}` e salva un PNG nella radice di automazione. Il percorso è relativo e non può contenere `..`. I metodi non supportati restituiscono un errore JSON. Il resto del protocollo del piano arriverà nei traguardi appropriati.

Da M1 l'app apre il canale anche senza `--control`: sceglie una porta locale e scrive `.sagomacad/session.json` con porta e percorso del token. `engine.commands` restituisce il catalogo del registro; `engine.execute` accetta `{"command":"param.set","params":{"name":"larghezza","expression":"40 mm"}}`. Il campo opzionale `coalesce` raggruppa chiamate consecutive in un solo passo di undo. `sagomacad-cli call` usa lo stesso protocollo autenticato; si vedano gli esempi in `docs/agent-cli.md`.

## Ispezione del layout Clay (LAYOUT)

`ui.inspect` mantiene i campi precedenti e aggiunge:

| Campo | Significato |
|---|---|
| `layout_engine` | `"clay"` |
| `docking` | `false`: la shell non usa più docking |
| `layout_scale` | Scala applicata ai token e al font (1 o 2) |
| `shell_rects` | Array `{id, x, y, width, height}` dei rettangoli della shell |
| `layout_state` | Stato serializzabile con versione, misure, visibilità e compressione |
| `layout_error` | Stringa vuota se il frame è valido; diagnostica in caso di fallback |

Gli ID sono `topbar`, `ribbon`, `browser`, `viewport`, `properties`, `timeline`,
`contextual`, `header_separator`, `browser_separator`, `properties_separator` e
`timeline_separator`. Le aree nascoste hanno dimensioni zero; `contextual` ha
larghezza zero perché riservata. I rettangoli sono esterni ai pannelli: il canvas
3D è il contenuto del rettangolo `viewport`, con il padding del tema. Le coordinate
sono relative al client SDL nello spazio ImGui, già moltiplicate per `layout_scale`;
non necessariamente coincidono con i pixel del framebuffer su display Retina.

```json
{
  "layout_engine": "clay", "docking": false, "layout_scale": 1,
  "shell_rects": [{"id": "topbar", "x": 0, "y": 0, "width": 1280, "height": 66}],
  "layout_state": {
    "version": 1, "browser_width": 243, "properties_width": 253,
    "timeline_height": 62, "browser": true, "properties": false, "timeline": true,
    "browser_collapsed": false, "properties_collapsed": false
  },
  "layout_error": ""
}
```

L’esempio abbrevia `shell_rects`; la risposta reale contiene tutti gli ID.
Il test `control` verifica i nuovi campi sul canale autenticato; `layout_interaction`
verifica trascinamento, doppio clic, compressione, riavvio e ridimensionamento.

## Preferenze e quote M5

`engine.execute` espone `preferences.get`, `preferences.set` (`hide_sketch_after_feature`: booleano, default false) e `feature.extrude.suggest` (sorgente/distanza/direzione → `operation`, `target_bodies`, senza mutazioni). Sono gli stessi comandi raggiungibili da CLI e MCP `command_run`. Le preferenze native vengono salvate localmente; la visibilità applicata dalla feature si salva nel documento e nel suo undo.

`ui.inspect.sketch` include `active_field` e le mappe di rettangoli `live_fields` (indice campo) e `dimension_rects` (ID quota), più `dimension_edit_rect` per il campo aperto dal doppio clic. Coordinate logiche `[x,y,width,height]`, proiettate nel viewport; consentono verifiche senza coordinate fisse.

## Snapshot con camera riproducibile

`--script` accetta un array JSON inline oppure il percorso di un file JSON (massimo 4 MiB). `ui.set` nello script snapshot accetta `camera: {yaw?, pitch?, roll?, distance?, target?: [x,y,z]}`: angoli in radianti, distanza in mm. La camera esplicita viene applicata dopo il fit iniziale. Numeri non finiti, distanze inferiori a 0.05 e coordinate oltre 1e7 vengono rifiutati. Esempi: `docs/examples/m5/camera-radente.json` e `cilindro-raccordi.json`.

## Handle nominati e input

`ui.inspect.handles` e un oggetto indicizzato per nome, ricreato a ogni frame. Ogni record e `point` (`x`,`y`), `rect` (`x`,`y`,`width`,`height`,`enabled`) oppure `count` (`value`). `ui.at {handle}` restituisce il punto o il centro del rettangolo; un conteggio non e cliccabile. Gli elementi chiusi o non disegnati non conservano handle vecchi. Le coordinate sono quelle logiche ImGui, alla scala corrente.

Nomi principali: `toolbar:<id comando>`, `menu:CREA`, `menu:MODIFICA`, `menu:File`, `tab:GESTISCI`, `row:<etichetta o id>`, `visibility:<id>`, `fold:<etichetta>`, `timeline:<id feature>`, `timeline:marker`, `timeline:control:<azione>`, `field:<etichetta o chiave>`, `field:live:<indice>`, `handle:arrow`, `handle:radius`, `handle:dimension:<id>`. Il cubo usa `cube:top`, `cube:top-front`, `cube:top-front-right`, `cube:home` e `cube:control:<azione>` per le frecce. Le voci di una tendina vengono pubblicate quando la tendina e aperta: Primitiva si raggiunge cliccando prima `menu:CREA`, poi `toolbar:primitive.add`.

- `ui.click {handle}` oppure `{x,y,button?:0|1|2,count?:1|2,modifiers?:["Ctrl","Shift"]}`: invia eventi reali ai widget e avanza i frame necessari.
- `ui.pointer {events:[{kind:"down"|"move"|"up",x,y,button?}],modifiers?}`: gesti, trascinamenti e pressioni prolungate. I pulsanti restano premuti tra chiamate fino a `up`.
- `ui.key {key:"Enter"}` o `{key:"Ctrl+Z"}`; `ui.type {text}`: tastiera e testo UTF-8.
- `ui.frame {seconds?:0.0166667}`: avanza un frame, fra 1 microsecondo e 1 secondo; utile per attese deterministiche e animazioni.
- `ui.set {theme?,view?,camera?,panels?,tool?}`: stato della vista. `panels` contiene browser/properties/timeline booleani; `tool` seleziona uno strumento dello schizzo attivo.
- `ui.resize {width,height}`: dimensioni logiche, da 320x240 a 4096x4096.

`ui.at` e gli eventi accettano anche `world:[x,y,z]`, `sketch:[x,y]`, `plane:"XY"|"XZ"|"YZ"` o `browser:"etichetta"` al posto di x/y. Numeri non finiti, handle mancanti, conteggi usati come punti e pulsanti disabilitati restituiscono errori leggibili. Le stringhe e il numero di eventi sono limitati.

CLI: `sagomacad-cli call ui.click '{"handle":"menu:CREA"}'`, poi `call ui.click '{"handle":"toolbar:primitive.add"}'`. Via MCP bridge: `ui_inspect {}` legge lo stesso registro e `ui_call {method:"ui.click",params:{handle:"..."}}` usa gli stessi eventi. In MCP headless questi due tool richiedono un bridge; il runner degli scenari usa direttamente una shell nascosta.

Per test isolati l'app accetta `--settings-file <percorso>` insieme a `--session-file`: le prove non modificano le preferenze o la sessione dell'utente.
