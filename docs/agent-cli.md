# SagomaCad da terminale — M1

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

`run --script '[...]'` accetta anche lo script JSON in linea. I comandi di schizzo, estrusione, misura e rendering del modello appartengono ai traguardi successivi; `engine.commands` indica quali sono realmente disponibili.
