# Staffa a L creata via MCP

[staffa-L.sagomacad](staffa-L.sagomacad) è il documento parametrico ottenuto
eseguendo il flusso della [guida di modellazione](../../modeling-guide.md).
Richiesta di partenza: staffa larga 60 mm, gambe da 40 mm, spessore 5 mm, un foro
passante Ø8 sulla base e uno sul montante.

Il documento contiene quattro schizzi e quattro feature: base estrusa, montante
unito, taglio della base e taglio del montante. I parametri `larghezza`, `spessore`
e `foro` sono modificabili; i profili conservano riferimenti alle entità.

`mass_properties` sul server MCP restituisce:

- Volume: **21997,34517542563 mm³**, uguale a `22500 − 160π` entro 1e-4 mm³.
- Area: **10400,265482457437 mm²**.
- Limiti: **60 × 40 × 40 mm**, entro 1,5e-7 mm.

Risposta completa in [mass-properties.json](mass-properties.json), cattura nativa
del tool MCP `render_view` in [staffa-L.png](staffa-L.png). Il test
`modeling_agents` rigenera modello, immagine e misure nella cartella di build.
