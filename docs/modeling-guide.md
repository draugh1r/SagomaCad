# Modellazione parametrica — M4 e M5

Questa guida è anche la risorsa MCP `sagomacad://docs/modeling-guide`.
UI, CLI e MCP usano lo stesso registro. `commands_list` documenta i parametri;
`command_run` esegue un comando, `feature_get` ne legge dati ed errore,
`sketch_inspect` restituisce contorni e regioni, `mass_properties` misura i solidi.
Le lunghezze sono in mm e i volumi in mm³; le espressioni ammettono unità e nomi
di parametri. Un angolo senza unità è in gradi.

## Profili e comandi

Estrudi usa profili chiusi di schizzo oppure una faccia piana selezionata.
M5 aggiunge schizzi su faccia, raccordi, smussi e fori con riferimenti stabili;
il push/pull diretto appartiene a M6. Per cambiare una distanza già confermata,
fare doppio clic sull'estrusione nella timeline e modificarne il valore.

Dopo una feature riuscita il suo schizzo viene nascosto automaticamente, non
eliminato. Nel browser ogni schizzo ha un occhio: cliccarlo per mostrare di nuovo
i profili. Selezionare lo schizzo e premere E, oppure usare il menu contestuale
Estrudi, permette di riutilizzarlo anche quando è nascosto. Un profilo unico è
preselezionato; con più regioni selezionare quelle desiderate nel viewport.
Una seconda estrusione Unisci parte ancora dal piano del profilo: una distanza
di 15 mm su una piastra estrusa di 10 mm porta lo spessore totale a 15 mm.
Nuovo corpo crea invece un corpo distinto, anche se si sovrappone al precedente.

CLI e MCP espongono `sketch.visibility {sketch, visible}` con `true` per mostrare,
`false` per nascondere e `null` per tornare alla gestione automatica. La scelta
si salva nel documento, sopravvive alle modifiche delle quote e supporta undo/redo.
`sketch.inspect` restituisce `visible` effettivo e `visibility` (`true`, `false`
o `null`). La visibilità non cambia il volume né la disponibilità geometrica
della sorgente per le feature dipendenti. Rollback e soppressione restano prioritari.

Ogni regione ha `id`, `boundary` (ID delle entità del contorno esterno) e `holes`
(contorni interni immediati). Un rettangolo con un cerchio dentro dà una corona
e un disco: estrudere la corona preserva il foro. Gli ID dipendono dalle entità,
non da coordinate o posizione nella lista; conservarli dopo le modifiche dimensionali.
`profiles` accetta gli ID delle regioni oppure liste degli ID dei contorni esterni.
I fori sono risolti dallo schizzo rivalutato. Le entità di costruzione sono escluse.

| Comando | Parametri principali |
|---|---|
| `feature.extrude` | `sketch`, `profiles`, `distance` (espressione); `direction`: `one_side`, `symmetric`, `two_sides`; `second_distance`; `extent`: `distance`, `through_all`; `operation`: `new`, `join`, `cut`, `intersect`; `target_bodies` |
| `feature.revolve` | `sketch`, `profiles`, `axis`: `X`, `Y`, `Z` o ID linea dello stesso schizzo; `type`: `angle`, `full`; `angle`; `operation`, `target_bodies` |
| `feature.edit` | `feature_id`, `params`: oggetto con le sole modifiche; per primitive resta disponibile `dimensions` |
| `sketch.dimension.set` | `sketch`, `dimension`, `expression` |
| `timeline.rollback` | `index`: numero di passi applicati, fra 0 e lunghezza della timeline |
| `feature.suppress` | `feature_id`, `suppressed`: booleano |
| `feature.delete` | `feature_id` |
| `feature.get` | `feature_id`; accessibile anche come tool MCP `feature_get` |

Nuovo corpo non ammette destinazioni; le altre operazioni richiedono una lista
esplicita. Unisci fonde tutte le destinazioni nel primo corpo, consumando gli altri;
rollback e soppressione li ripristinano. Taglia e Interseca agiscono sulle singole
destinazioni. Un errore non lascia modifiche parziali.

La simmetrica usa la distanza totale, metà per lato. Due lati usa `distance` nel
verso positivo e `second_distance` in quello negativo. I normali dei piani XY/XZ/YZ
sono rispettivamente +Z/−Y/+X. Una distanza negativa inverte Un lato.
Attraverso tutto deriva l'estensione dai limiti delle destinazioni; la distanza
indica il verso. Con simmetrica o due lati attraversa entrambi i versi.

Gli schizzi consumati vengono nascosti dalla vista. Modificare una quota o un
parametro rivaluta dalla prima feature dipendente; le feature in errore conservano
un messaggio e la valutazione prosegue. Eliminare lo schizzo sorgente non elimina
silenziosamente l'estrusione: `feature_get.error` descrive il riferimento mancante.

## Client MCP completo

Eseguire i blocchi Python seguenti nello stesso processo. Il primo avvia il server
stdio, anche senza finestra. Sostituire `CLI` con il percorso dell'eseguibile se
non è nel PATH. Gli esempi creano documenti nuovi; usare una radice di lavoro propria.

```python
import json, math, subprocess
from pathlib import Path

CLI = globals().get("CLI", "sagomacad-cli")
WORK = Path(globals().get("WORK", ".")).resolve()
process = subprocess.Popen(
    [CLI, "mcp", "--headless", "--allow-root", str(WORK)],
    stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    text=True, encoding="utf-8", bufsize=1)
request_id = 0

def rpc(method, params=None):
    global request_id
    request_id += 1
    request = {"jsonrpc": "2.0", "id": request_id, "method": method,
               "params": params or {}}
    process.stdin.write(json.dumps(request) + "\n")
    process.stdin.flush()
    response = json.loads(process.stdout.readline())
    assert response["id"] == request_id and "result" in response, response
    return response["result"]

rpc("initialize", {"protocolVersion": "2025-06-18", "capabilities": {},
                   "clientInfo": {"name": "modeling-guide", "version": "1"}})
process.stdin.write('{"jsonrpc":"2.0","method":"notifications/initialized"}\n')
process.stdin.flush()

def tool(name, args=None):
    result = rpc("tools/call", {"name": name, "arguments": args or {}})
    assert not result.get("isError"), result
    return result["structuredContent"]

def command(command_id, **params):
    return tool("command_run", {"command": command_id, "params": params})

def region(sketch, boundary):
    regions = tool("sketch_inspect", {"sketch": sketch})["regions"]
    return next(r["id"] for r in regions if set(r["boundary"]) == set(boundary))

def check_volume(body, expected):
    measured = tool("mass_properties", {"body": body})["volume_mm3"]
    assert abs(measured - expected) < 1e-3, (measured, expected)
    return measured
```

## Piastra forata, taglio e quota a monte

```python
command("document.new")
command("param.set", name="larghezza", expression="60 mm")
command("param.set", name="foro", expression="10 mm")
sketch = command("sketch.create", plane="XY")["sketch"]
rectangle = command("sketch.rect", sketch=sketch, width="larghezza",
                    height="40 mm", centered=True)
command("sketch.circle", sketch=sketch, diameter="foro")
profile = region(sketch, rectangle["entities"])
command("sketch.finish", sketch=sketch)
plate = command("feature.extrude", sketch=sketch, profiles=[profile],
                distance="10 mm", operation="new")
body = plate["body_id"]
check_volume(body, (60*40 - math.pi*5**2)*10)

command("param.set", name="larghezza", expression="80 mm")
command("param.set", name="foro", expression="20 mm")
check_volume(body, (80*40 - math.pi*10**2)*10)

cut_sketch = command("sketch.create", plane="XY")["sketch"]
circle = command("sketch.circle", sketch=cut_sketch, x=20, y=0, diameter="5 mm")
cut_profile = region(cut_sketch, [circle["entity"]])
command("sketch.finish", sketch=cut_sketch)
cut = command("feature.extrude", sketch=cut_sketch, profiles=[cut_profile],
              distance="10 mm", extent="through_all", operation="cut",
              target_bodies=[body])
check_volume(body, (80*40 - math.pi*(10**2 + 2.5**2))*10)

command("feature.edit", feature_id=plate["feature_id"], params={"distance": "15 mm"})
check_volume(body, (80*40 - math.pi*(10**2 + 2.5**2))*15)
width_dimension = tool("sketch_inspect", {"sketch": sketch})["dimensions"][0]["id"]
command("sketch.dimension.set", sketch=sketch, dimension=width_dimension,
        expression="larghezza + 10 mm")
check_volume(body, (90*40 - math.pi*(10**2 + 2.5**2))*15)

# Il marker è un conteggio, non l'ID di una feature.
command("timeline.rollback", index=2)
check_volume(body, (90*40 - math.pi*10**2)*15)
assert not tool("feature_get", {"feature_id": cut["feature_id"]})["applied"]
command("timeline.rollback", index=4)
command("feature.suppress", feature_id=cut["feature_id"], suppressed=True)
check_volume(body, (90*40 - math.pi*10**2)*15)
command("feature.suppress", feature_id=cut["feature_id"], suppressed=False)
check_volume(body, (90*40 - math.pi*(10**2 + 2.5**2))*15)
```

## Rivoluzione e riferimento mancante

```python
ring_sketch = command("sketch.create", plane="XZ")["sketch"]
ring_rectangle = command("sketch.rect", sketch=ring_sketch, x=5,
                         width="10 mm", height="10 mm")
ring_profile = region(ring_sketch, ring_rectangle["entities"])
command("sketch.finish", sketch=ring_sketch)
ring = command("feature.revolve", sketch=ring_sketch, profiles=[ring_profile],
               axis="Z", type="full", operation="new")
check_volume(ring["body_id"], math.pi*(15**2 - 5**2)*10)
command("feature.edit", feature_id=ring["feature_id"],
        params={"type": "angle", "angle": "180 deg"})
check_volume(ring["body_id"], math.pi*(15**2 - 5**2)*10/2)

# In alternativa axis può essere l'ID restituito da sketch.line nello stesso schizzo.
command("feature.delete", feature_id=sketch)
failure = tool("feature_get", {"feature_id": plate["feature_id"]})
assert "Schizzo sorgente mancante" in failure["error"]
check_volume(ring["body_id"], math.pi*(15**2 - 5**2)*10/2)
tool("undo")
assert not tool("feature_get", {"feature_id": plate["feature_id"]})["error"]
```

## Dal testo a una staffa a L con due fori

Richiesta: «Staffa a L, larga 60 mm, gambe da 40 mm, spessore 5 mm. Un foro
passante Ø8 sulla base a (30,25), uno sul montante a X=30, Z=25».
Le gambe si sovrappongono nel piede; il volume atteso è
`60*40*5*2 − 60*5*5 − 2*π*4²*5 = 21997,345175 mm³`.

```python
command("document.new")
command("param.set", name="larghezza", expression="60 mm")
command("param.set", name="spessore", expression="5 mm")
command("param.set", name="foro", expression="8 mm")
base_sketch = command("sketch.create", plane="XY")["sketch"]
base_rect = command("sketch.rect", sketch=base_sketch, width="larghezza", height="40 mm")
base_profile = region(base_sketch, base_rect["entities"])
command("sketch.finish", sketch=base_sketch)
base = command("feature.extrude", sketch=base_sketch, profiles=[base_profile],
               distance="spessore", operation="new", name="Base staffa")
bracket_body = base["body_id"]
upright_sketch = command("sketch.create", plane="XZ")["sketch"]
upright_rect = command("sketch.rect", sketch=upright_sketch, width="larghezza", height="40 mm")
upright_profile = region(upright_sketch, upright_rect["entities"])
command("sketch.finish", sketch=upright_sketch)
upright = command("feature.extrude", sketch=upright_sketch, profiles=[upright_profile],
                  distance="-spessore", operation="join", target_bodies=[bracket_body])
check_volume(bracket_body, 22500)

for plane in ("XY", "XZ"):
    hole_sketch = command("sketch.create", plane=plane)["sketch"]
    circle = command("sketch.circle", sketch=hole_sketch, x=30, y=25, diameter="foro")
    hole_profile = region(hole_sketch, [circle["entity"]])
    command("sketch.finish", sketch=hole_sketch)
    command("feature.extrude", sketch=hole_sketch, profiles=[hole_profile],
            distance="spessore" if plane == "XY" else "-spessore",
            extent="through_all", operation="cut", target_bodies=[bracket_body])
bracket_volume = check_volume(bracket_body, 22500 - 160*math.pi)
print("Staffa a L con due fori:", bracket_volume, "mm3")
tool("document_save", {"path": "staffa-L.sagomacad"})

command("param.set", name="larghezza", expression="80 mm")
check_volume(bracket_body, 30000 - 160*math.pi)
command("param.set", name="foro", expression="10 mm")
check_volume(bracket_body, 30000 - 250*math.pi)
```

## Facce, spigoli e riferimenti stabili (M5)

`geometry_query` restituisce `entities[].ref`: conservarlo intero, con firma,
invece degli ID numerici di tassellazione. Filtri: `body`, `kind` (face/edge/vertex),
`type` (plane/line/circle/cylinder), `parallel_to`, `normal`, `limit`.
`measure {a,b}` restituisce distanza minima e angolo; l'angolo è null quando
non definito. `selection_get {}` legge la selezione reale solo in modalità bridge.
`render_view` accetta `labels: true` e restituisce anche i nomi disegnati.

`sketch.create {face: ref}` crea proiezioni immutabili utilizzabili nei vincoli;
`feature.extrude {face: ref, distance: expression, ...}` mantiene le opzioni delle
estrusioni da schizzo. `feature.fillet {edges: [ref,...], radius: expression}`
e `feature.chamfer {edges: [ref,...], distance: expression}` lavorano sul corpo
degli spigoli. `feature.hole {face: ref, position: {x,y}, diameter: expression,
through_all: true}` cattura due quote dai bordi. `feature_get.params.references`
contiene i bordi, il lato e le distanze modificabili con `feature.edit.params`.
Un riferimento eliminato o ambiguo genera un errore leggibile nella feature.

Il flusso seguente esegue «raccorda con raggio 2 tutti gli spigoli verticali»,
crea anche uno schizzo appoggiato e un foro quotato, poi cambia le misure a monte.
Salva il documento precedente per poter continuare gli altri esempi.

```python
import base64

tool("document_save", {"path": "m5-previous.sagomacad"})
command("document.new")
command("param.set", name="m5_larghezza", expression="40 mm")
command("param.set", name="m5_altezza", expression="20 mm")
m5_base = command("primitive.add", kind="box", dimensions={
    "width": "m5_larghezza", "depth": "40 mm", "height": "m5_altezza"})
m5_body = m5_base["body_id"]
m5_top = tool("geometry_query", {"body": m5_body, "kind": "face", "normal": [0,0,1]})["entities"][0]["ref"]
m5_sketch = command("sketch.create", face=m5_top)["sketch"]
m5_circle = command("sketch.circle", x=20, y=20, diameter="10 mm")
m5_region = region(m5_sketch, [m5_circle["entity"]])
m5_cylinder = command("feature.extrude", sketch=m5_sketch, profiles=[m5_region], distance="6 mm")
command("param.set", name="m5_altezza", expression="35 mm")
assert abs(tool("mass_properties", {"body": m5_cylinder["body_id"]})["bbox_mm"]["min"]["z"] - 35) < 1e-5
m5_edges = tool("geometry_query", {"body": m5_body, "kind": "edge", "type": "line", "parallel_to": [0,0,1]})["entities"]
assert len(m5_edges) == 4
m5_fillet = command("feature.fillet", edges=[e["ref"] for e in m5_edges], radius="2 mm")
m5_top = tool("geometry_query", {"body": m5_body, "kind": "face", "normal": [0,0,1]})["entities"][0]["ref"]
m5_hole = command("feature.hole", face=m5_top, position={"x":10,"y":10}, diameter="8 mm", through_all=True)
command("param.set", name="m5_larghezza", expression="60 mm")
command("param.set", name="m5_altezza", expression="50 mm")
check_volume(m5_body, (2400 - (4-math.pi)*4 - 16*math.pi)*50)
m5_wall = tool("geometry_query", {"body":m5_body,"kind":"face","type":"cylinder"})["entities"]
assert any(abs(e["geometry"]["center"][0]-10)<1e-5 and abs(e["geometry"]["center"][1]-10)<1e-5 for e in m5_wall)
m5_bottom = tool("geometry_query", {"body":m5_body,"kind":"face","normal":[0,0,-1]})["entities"][0]["ref"]
assert abs(tool("measure", {"a":m5_top,"b":m5_bottom})["distance_mm"]-50)<1e-5
m5_image = rpc("tools/call", {"name":"render_view","arguments":{"view":"iso","size":"960x720","labels":True}})
assert m5_image["structuredContent"]["entity_labels"]
(WORK / "m5-labels.png").write_bytes(base64.b64decode(next(c["data"] for c in m5_image["content"] if c["type"]=="image")))
tool("document_save", {"path":"m5-acceptance.sagomacad"})
tool("document_open", {"path":"m5-previous.sagomacad"})
```

Chiudere il client:

```python
process.stdin.close()
assert process.wait(timeout=10) == 0, process.stderr.read()
```

## Stessi comandi da CLI

`sagomacad-cli run --script-file passi.json --headless piastra.sagomacad`
esegue uno script in un processo, mantenendo undo/redo. Questo esempio parte da
un documento nuovo; gli ID sotto sono deterministici solo in questo esempio.
Per un documento esistente usare gli ID restituiti da creazione e ispezione.

```json
[
  ["engine.execute", {"command":"document.new"}],
  ["engine.execute", {"command":"param.set","params":{"name":"larghezza","expression":"60 mm"}}],
  ["engine.execute", {"command":"param.set","params":{"name":"foro","expression":"10 mm"}}],
  ["engine.execute", {"command":"sketch.create","params":{"plane":"XY"}}],
  ["engine.execute", {"command":"sketch.rect","params":{"width":"larghezza","height":"40 mm","centered":true}}],
  ["engine.execute", {"command":"sketch.circle","params":{"diameter":"foro"}}],
  ["engine.execute", {"command":"feature.extrude","params":{"sketch":"sketch-1","profiles":[["e1","e2","e3","e4"]],"distance":"10 mm"}}],
  ["engine.execute", {"command":"param.set","params":{"name":"larghezza","expression":"80 mm"}}],
  ["engine.execute", {"command":"feature.edit","params":{"feature_id":"feature-2","params":{"distance":"15 mm"}}}],
  ["engine.execute", {"command":"model.mass_properties","params":{"body":"body-2"}}],
  ["engine.execute", {"command":"timeline.rollback","params":{"index":1}}],
  ["engine.execute", {"command":"feature.get","params":{"feature_id":"feature-2"}}],
  ["engine.execute", {"command":"timeline.rollback","params":{"index":2}}],
  ["engine.execute", {"command":"feature.suppress","params":{"feature_id":"feature-2","suppressed":true}}],
  ["engine.execute", {"command":"feature.get","params":{"feature_id":"feature-2"}}],
  ["engine.execute", {"command":"feature.suppress","params":{"feature_id":"feature-2","suppressed":false}}]
]
```
