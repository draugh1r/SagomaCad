# MCP di SagomaCad (M1.5 + strumenti M2)

Il server locale è `sagomacad-cli mcp`. Usa JSON-RPC 2.0 su stdio, una richiesta JSON per riga; stdout contiene solo risposte MCP. Cerca automaticamente il canale di controllo dell'app aperta e si autentica con il token del file di sessione. Se l'app non è raggiungibile, avvia una sessione headless in memoria. `--headless` forza la sessione senza finestra; `--headless file.sagomacad` carica quel documento. Le modifiche headless restano in memoria finché non si usa `document_save`.

Binario Windows di questa build:

```text
C:\Users\User\Desktop\sviluppo\SagomaCad\build\agent-root\apps\sagomacad-cli\sagomacad-cli.exe
```

Per forzare il bridge, avvia l'app e passa `--bridge PORT --token-file PERCORSO`. L'app scrive porta e percorso del token nel suo `.sagomacad/session.json`; si può usare anche `--session-file PERCORSO`. La ricerca automatica controlla la directory corrente e la directory dell'app accanto alla CLI. I file MCP sono limitati alla radice del progetto; `--allow-root CARTELLA` sostituisce la radice predefinita. Gli input dei tool file sono relativi alla prima radice consentita: percorsi assoluti, `..` e symlink che escono dalla radice sono rifiutati. `export` scrive solo in `export/` del progetto se tale cartella appartiene a una radice consentita, e richiede `overwrite: true` per sostituire un file. `document_open` e `document_save` accettano solo `.sagomacad` e richiedono `overwrite: true` se il file esiste. I comandi file invocati tramite `command_run` sono bloccati: usare i tool dedicati.

## Claude Desktop su Windows

Apri `%APPDATA%\Claude\claude_desktop_config.json` e aggiungi la voce a `mcpServers` (conserva eventuali altre voci). Riavvia Claude Desktop. La posizione e la struttura `mcpServers` sono descritte nella [guida ufficiale MCP per Claude Desktop](https://modelcontextprotocol.io/docs/2026-07-28/develop/connect-local-servers).

```json
{
  "mcpServers": {
    "sagomacad": {
      "command": "C:\\Users\\User\\Desktop\\sviluppo\\SagomaCad\\build\\agent-root\\apps\\sagomacad-cli\\sagomacad-cli.exe",
      "args": ["mcp"]
    }
  }
}
```

## Claude Code su Windows

Da PowerShell, esegui:

```powershell
claude mcp add --transport stdio --scope user sagomacad -- "C:\Users\User\Desktop\sviluppo\SagomaCad\build\agent-root\apps\sagomacad-cli\sagomacad-cli.exe" mcp
claude mcp list
```

Il separatore `--`, il trasporto stdio e i comandi di verifica seguono la [documentazione ufficiale di Claude Code](https://code.claude.com/docs/en/mcp). Dentro Claude Code, `/mcp` mostra lo stato della connessione.

## Codex su Windows

Aggiungi in `%USERPROFILE%\.codex\config.toml`:

```toml
[mcp_servers.sagomacad]
command = 'C:\Users\User\Desktop\sviluppo\SagomaCad\build\agent-root\apps\sagomacad-cli\sagomacad-cli.exe'
args = ['mcp']
cwd = 'C:\Users\User\Desktop\sviluppo\SagomaCad'
```

Oppure usa `codex mcp add sagomacad -- "C:\Users\User\Desktop\sviluppo\SagomaCad\build\agent-root\apps\sagomacad-cli\sagomacad-cli.exe" mcp`, poi `codex mcp list`. La sintassi e la configurazione condivisa tra CLI ed estensione sono nella [documentazione ufficiale Codex MCP](https://learn.chatgpt.com/docs/extend/mcp?surface=cli).

## Tool e risorse

`commands_list` mostra gli ID e i parametri del registro comune a UI, CLI e MCP. `command_run` chiama un ID; `batch_run` esegue fino a 64 comandi modificabili come un unico passo di undo, in modo atomico quando `stop_on_error` è vero. `undo`, `redo`, `document_inspect`, `document_open` e `document_save` coprono il documento. `mass_properties` restituisce `volume_mm3`, `area_mm2`, `centroid_mm` e `bbox_mm`. `render_view` restituisce un'immagine PNG base64 da `iso`, `top`, `front` o `right`, con `size` opzionale come `640x480`. `export` supporta STL; `ui_screenshot` è disponibile solo quando il bridge è connesso all'app. Le risorse sono `sagomacad://document`, `sagomacad://commands` e `sagomacad://docs/modeling-guide`.

Esempio di sequenza dei tool MCP:

```json
{"name":"command_run","arguments":{"command":"param.set","params":{"name":"larghezza","expression":"40 mm"}}}
{"name":"command_run","arguments":{"command":"primitive.add","params":{"kind":"box","dimensions":{"width":"larghezza","depth":"larghezza","height":"20 mm"}}}}
{"name":"command_run","arguments":{"command":"primitive.add","params":{"kind":"cylinder","operation":"subtract","body":"body-1","dimensions":{"radius":"5 mm","height":"20 mm"},"position":{"x":"larghezza / 2","y":"larghezza / 2","z":"0 mm"}}}}
{"name":"mass_properties","arguments":{"body":"body-1"}}
{"name":"render_view","arguments":{"view":"iso","size":"640x480"}}
{"name":"export","arguments":{"format":"stl","path":"pezzo.stl","body":"body-1"}}
```

Il foro dell'esempio ha **diametro 10 mm** (raggio 5 mm). Il volume atteso è `40×40×20 − π×5²×20 ≈ 30 429,204 mm³`. I fallimenti di tool restituiscono `isError`, un codice stabile, un messaggio e, quando possibile, un suggerimento. Gli errori di protocollo usano i codici JSON-RPC standard.

I risultati strutturati MCP sono sempre oggetti: `commands_list` usa `{commands: [...]}`; i comandi che restituiscono una lista, come `param.list`, la espongono in `{items: [...]}`. Il contenuto testuale conserva il JSON originale del risultato del motore.
