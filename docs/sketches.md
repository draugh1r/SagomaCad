# Schizzi 2D — M3

## Nell'app

**Solido > Crea > Schizzo** mostra i tre piani dell'origine. Clic su un piano: la camera si allinea con una transizione e compare la scheda Schizzo. La tavolozza controlla griglia, aggancio, profili, punti, quote, vincoli e geometrie di costruzione. Nelle finestre basse le opzioni scorrono; **Fine schizzo** rimane nel piè di pagina.

Il disegno parte con il primo clic: muovendo il mouse, la geometria in anteprima e le misure nelle caselle si aggiornano continuamente. Il secondo clic conferma direttamente. Non occorre inserire alcuna quota. Digitare sostituisce la misura attiva e la blocca; Tab passa alla successiva, mentre le misure non bloccate continuano a seguire il cursore. Invio conferma anche da tastiera. Solo le quote digitate diventano vincoli dimensionali: le geometrie disegnate esclusivamente con il mouse restano libere nelle dimensioni.

- `R`: rettangolo. Clic sull'origine, `40`, Tab, `20`, Invio crea il rettangolo 40×20 completamente vincolato.
- `L`: linea; dopo il primo clic inserisci lunghezza e angolo.
- `C`: cerchio; inserisci il diametro, anche un parametro come `foro`.
- Arco nel menu Crea: centro, punto iniziale, punto finale; campi raggio e angolo finale.
- `D`: quota. Seleziona una geometria oppure due elementi con Shift+clic. Tipi: lunghezza, distanza, diametro, raggio e angolo. Doppio clic su una quota per modificarne l'espressione.
- Esc annulla il tratto; un secondo Esc esce dallo strumento.

Geometria libera blu, vincolata nera nel tema chiaro e chiara nel tema scuro, conflitti rossi con gli ID nella tavolozza. Le ridondanze sono elencate separatamente. Trascina un punto libero: il resto segue i vincoli, con un solo passo di undo. Doppio clic sullo schizzo nella timeline o nel browser per riaprire la modifica.

## Comandi

Gli schizzi precedenti e i solidi già creati restano visibili mentre si disegna.
Una feature nasconde automaticamente solo il suo schizzo sorgente: l'occhio
nel browser lo rende nuovamente visibile e selezionabile. Seleziona lo schizzo
e premi E per riutilizzarne i profili; doppio clic continua ad aprirne la modifica.
Il comando `sketch.visibility {sketch, visible: true|false|null}` salva la scelta
manuale o ripristina quella automatica (`null`).

Coordinate numeriche in mm nel piano: XY = (X,Y), XZ = (X,Z), YZ = (Y,Z). Quote come stringhe con espressioni e unità; numeri senza unità in mm, oppure gradi per un angolo. Riutilizza gli ID restituiti dai comandi.

| Comando | Parametri |
|---|---|
| `sketch.create` | `plane: "XY"|"XZ"|"YZ"`, `name?` |
| `sketch.finish`, `sketch.edit`, `sketch.inspect` | `sketch?: id` |
| `sketch.line` | `x1,y1,x2,y2`, `length?`, `angle?`, `anchor?: false` |
| `sketch.rect` | `width,height` come espressioni, `x?:0,y?:0`, `centered?:false`, `anchor?:true` |
| `sketch.circle` | `diameter` come espressione, `x?:0,y?:0`, `anchor?:true` |
| `sketch.arc` | `radius,start_angle,end_angle` come espressioni, `x?:0,y?:0`, `anchor?:false` |
| `sketch.constraint.add` | `kind`, `refs:[id,...]` |
| `sketch.dimension.add` | `kind`, `refs:[id,...]`, `expression` |
| `sketch.dimension.set` | `dimension:id`, `expression` |
| `sketch.point.move` | `point:id`, `x,y`; usare `coalesce` sul canale di controllo per un drag |

Tutti tranne create accettano `sketch?: id`, altrimenti usano lo schizzo attivo. La CLI `call --headless file` carica una nuova sessione: passa l'ID esplicitamente. Con `run` e MCP la sessione resta attiva tra le chiamate. I comandi di disegno accettano anche `construction?: bool`; il rettangolo accetta `flip_x`/`flip_y` per disegnare dal primo angolo nelle direzioni negative.

Per riprodurre il disegno libero da CLI/MCP usa `driving_width:false`/`driving_height:false` sul rettangolo, `driving_diameter:false` sul cerchio e `driving_radius:false`/`driving_start_angle:false`/`driving_end_angle:false` sull'arco. Questi flag valgono `true` per default nelle chiamate dirette, così le espressioni degli esempi restano parametriche; la UI li attiva solo per le misure digitate. I valori con flag `false` definiscono la geometria iniziale senza aggiungere una quota.

Vincoli: `coincident`, `horizontal`, `vertical`, `parallel`, `perpendicular`, `tangent`, `equal`, `midpoint`, `fix`. Passa ID dei punti o delle entità secondo il vincolo. Per `midpoint`, prima il punto e poi la linea. `fix` cattura la posizione attuale. Quote: `distance`, `length`, `diameter`, `radius`, `angle`. Un angolo si riferisce a una linea rispetto all'orizzontale oppure a due linee.

`sketch.inspect` e MCP `sketch_inspect` restituiscono punti, entità, vincoli, quote, `degrees_of_freedom`, `state`, `residual`, `conflicting`, `redundant`, `fixed_points`, `profiles`. Le quote in conflitto sono conservate per diagnosi e undo: controlla `solver.state` e `solver.error` anche dopo un comando riuscito. Un'espressione non valida viene rifiutata senza cambiare il documento. Stati: `free`, `constrained`, `conflict`, `error`.

## Esempio MCP

Argomenti del tool `command_run`, in ordine:

```json
{"command":"param.set","params":{"name":"foro","expression":"10 mm"}}
{"command":"sketch.create","params":{"plane":"XY"}}
{"command":"sketch.rect","params":{"centered":true,"width":"60 mm","height":"40 mm"}}
{"command":"sketch.circle","params":{"diameter":"foro"}}
```

Chiama `sketch_inspect` con `{}`: zero gradi di libertà, due profili chiusi. `centered:true` centra nell'origine. `anchor:false` lascia libera la posizione; la UI ancora automaticamente il primo punto se coincide con l'origine. Le linee agganciate a punti esistenti ricevono vincoli coincidenti. `batch_run` con i comandi di creazione in `steps` produce un solo undo. Chiama `sketch.finish` e `document_save` per conservare il lavoro.

## Client CLI

Salva come `sketch-session.json`:

```json
[
  ["engine.execute",{"command":"param.set","params":{"name":"foro","expression":"10 mm"}}],
  ["engine.execute",{"command":"sketch.create","params":{"plane":"XY"}}],
  ["engine.execute",{"command":"sketch.rect","params":{"centered":true,"width":"60 mm","height":"40 mm"}}],
  ["engine.execute",{"command":"sketch.circle","params":{"diameter":"foro"}}],
  ["engine.execute",{"command":"sketch.inspect","params":{}}]
]
```

```powershell
& .\build\agent-m3\apps\sagomacad-cli\sagomacad-cli.exe run --script-file .\sketch-session.json --headless .\prova.sagomacad
& .\build\agent-m3\apps\sagomacad-cli\sagomacad-cli.exe call engine.execute '{"command":"sketch.inspect","params":{"sketch":"sketch-1"}}' --headless .\prova.sagomacad
```

Estrusioni e tagli da profili appartengono a M4.

## Visibilità dopo una feature

Lo schizzo resta visibile per default: i profili non usati restano selezionabili per altre estrusioni. File → Preferenze offre “Nascondi lo schizzo dopo la creazione di una feature”, disattivata al primo avvio e salvata nelle impostazioni locali. `preferences.get` e `preferences.set` (`hide_sketch_after_feature`: booleano) sono disponibili da registro, CLI e MCP `command_run`. L’occultamento, se attivato, fa parte dello stesso undo della feature; mostrare/nascondere dal browser resta possibile. `sketch.visibility` con `visible: null` rimuove la scelta esplicita e torna visibile. La preferenza non cambia retroattivamente i documenti.

## Operazione automatica di estrusione

Se `feature.extrude` omette `operation`, il motore misura il contatto esatto dei profili sorgente con i corpi (tolleranza 1e-7 mm) e la sovrapposizione volumetrica della spazzata: entrando nel materiale propone Taglia; a contatto verso l’esterno Unisci; senza contatto Nuovo corpo. Le normali orientate delle facce determinano l’interno anche sulle facce minime e laterali. Le destinazioni comprendono solo i corpi coinvolti. `feature.extrude.suggest` espone la stessa decisione in sola lettura tramite registro, CLI e MCP. Un `operation` esplicito prevale sempre. Nel dialogo, cambiando distanza/direzione la scelta si aggiorna finché l’utente non sceglie dal menu Operazione; da quel momento resta manuale. Riaprire una feature conserva la sua operazione.
