# SagomaCad da terminale — M4

Avviare `sagomacad` dalla cartella di lavoro. L'app apre il canale di controllo su `127.0.0.1`, scrive `.sagomacad/session.json` con porta e percorso del token, e protegge il token in un file separato. `sagomacad-cli call` legge automaticamente questi dati. `--port N --token-file PATH` li sostituiscono quando serve; `--session-file PATH` sceglie un file di sessione diverso.

```sh
sagomacad-cli call engine.commands
sagomacad-cli call engine.execute '{"command":"param.set","params":{"name":"larghezza","expression":"40 mm"}}'
sagomacad-cli call engine.execute '{"command":"param.set","params":{"name":"foro","expression":"larghezza / 4"}}'
sagomacad-cli call engine.execute '{"command":"param.list"}'
sagomacad-cli call engine.execute '{"command":"document.saveAs","params":{"path":"prova.sagomacad"}}'
sagomacad-cli call ui.screenshot '{"path":"prova.png"}'
```

Il percorso di `ui.screenshot` è relativo alla radice di automazione dell'app. Ogni risposta riuscita è JSON su stdout; gli errori sono su stderr con codice di uscita non zero. `engine.commands` restituisce ID, etichette, parametri, scorciatoie e stato `enabled` dal registro unico dell'app.

Per modificare una primitiva, usa l'ID restituito da `primitive.add` come `feature_id`. La modifica è un solo passo di undo nell'app in esecuzione:

```sh
sagomacad-cli call engine.execute '{"command":"primitive.add","params":{"kind":"box","dimensions":{"width":"larghezza","depth":"larghezza","height":"20 mm"}}}'
sagomacad-cli call engine.execute '{"command":"feature.edit","params":{"feature_id":"primitive-1","dimensions":{"width":"60 mm"}}}'
sagomacad-cli call engine.execute '{"command":"edit.undo"}'
```

## Senza app aperta

`--headless FILE.sagomacad` carica il file, oppure crea un documento vuoto se non esiste. Le modifiche riuscite vengono salvate nel file. Ogni invocazione `call` apre una nuova sessione: per conservare la storia di undo/redo durante più passi, usare `run` con uno script in un solo processo.

```sh
sagomacad-cli call engine.execute '{"command":"param.list"}' --headless prova.sagomacad
sagomacad-cli run --script-file passi.json --headless prova.sagomacad
```

Esempio di `passi.json`:

```json
[
  ["engine.execute", {"command":"param.set","params":{"name":"larghezza","expression":"80 mm"}}],
  ["engine.execute", {"command":"param.list"}],
  ["engine.execute", {"command":"edit.undo"}],
  ["engine.execute", {"command":"param.list"}],
  ["engine.execute", {"command":"edit.redo"}]
]
```

`run --script '[...]'` accetta anche lo script JSON in linea. `engine.commands` indica quali comandi sono realmente disponibili.

## Profili e timeline

`sketch.inspect` restituisce `regions`: l'ID di una regione o la lista degli ID
del suo contorno esterno si passa a `feature.extrude.profiles` o
`feature.revolve.profiles`. Le modifiche passano da
`feature.edit {feature_id, params: {...}}`; `timeline.rollback {index}` applica
solo il prefisso richiesto e `feature.suppress {feature_id, suppressed}` cambia
la soppressione. `feature.get {feature_id}` espone anche gli errori.

Script CLI completo, flussi MCP equivalenti e volumi attesi sono in
[Modellazione parametrica](modeling-guide.md). Il test esegue direttamente lo
script pubblicato, oltre a creare e modificare una rivoluzione da CLI.


## M5: interrogare riferimenti stabili

Gli stessi comandi MCP sono disponibili dal registro CLI. Per esempio:

```powershell
sagomacad-cli call engine.execute '{"command":"geometry.query","params":{"kind":"edge","type":"line","parallel_to":[0,0,1]}}' --headless pezzo.sagomacad
```

Conservare entities[].ref intero e usarlo in feature.fillet/feature.chamfer,
feature.hole, sketch.create o feature.extrude. inspect.measure accetta a e b
come riferimenti e restituisce distance_mm e angle_degrees. selection.get
richiede una connessione all'app; non inventa una selezione headless.
La guida MCP incorporata contiene i flussi completi e i test ne eseguono gli esempi.
