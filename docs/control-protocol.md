# Protocollo di controllo M0

Avviare `sagomacad --control 7878 --control-token-file .private/control.token --automation-root screenshots`. Il server ascolta solo `127.0.0.1` e accetta JSON a riga singola. La prima richiesta di ogni connessione deve essere `{"id":1,"method":"auth","params":{"token":"..."}}`; una risposta riuscita contiene `result.authenticated: true`. Prima dell'autenticazione le altre richieste restituiscono `error.message`.

`ui.inspect` restituisce dimensioni logiche, tema, visibilità dei pannelli, griglia, assi e stato del layout fisso (`fixed_layout`, `tabs_visible`, `viewport.title_visible`). `ui.screenshot` accetta `{"path":"nome.png"}` e salva un PNG nella radice di automazione. Il percorso è relativo e non può contenere `..`. I metodi non supportati restituiscono un errore JSON. Il resto del protocollo del piano arriverà nei traguardi appropriati.
