# Verifica dei flussi schizzo ed estrusione — 17 ottobre 2026

## Ambito del piano

M4 realizza estrusioni **da profili di schizzo** e loro modifica parametrica.
Il picking delle facce, gli schizzi su faccia e i riferimenti topologici sono in
M5; `face.pushPull` è in M6. Non sono stati anticipati durante questo audit.
Selezionare una faccia del solido e premere E non è quindi un flusso già disponibile.

Per modificare un'estrusione esistente: doppio clic sulla relativa icona nella
timeline. Per riusare la stessa geometria: mostrare lo schizzo con l'occhio nel
browser, selezionare il profilo e premere E. In alternativa selezionare lo schizzo
nel browser e premere E, oppure scegliere Estrudi nel suo menu contestuale.
Con una sola regione viene preselezionata; con più regioni si sceglie nel viewport.
Una nuova estrusione parte dal piano dello schizzo, non dalla faccia superiore:
Unisci a 15 mm su una piastra da 10 mm dà uno spessore totale di 15 mm.

## Difetti riprodotti e corretti

| Difetto | Causa | Correzione e verifica |
|---|---|---|
| Schizzi precedenti invisibili entrando nel secondo | Filtro che disegnava solo l'attivo | Già corretto in `fc911bf`; test sui pixel del bordo del precedente |
| Solidi invisibili mentre si disegna | Rendering superfici nel ramo escluso in modalità schizzo | Già corretto in `e2032e9`; test sui pixel della faccia e sul nascondi/mostra corpo |
| Schizzo usato dall'estrusione apparentemente perso | Nascondimento automatico senza controllo per ripristinarlo | Occhio effettivo, tooltip, selezione e menu Estrudi nel browser |
| E ereditava profili di un altro schizzo | Stato di selezione condiviso senza controllo della sorgente | Verifica della sorgente, rimozione riferimenti obsoleti e preselezione del solo profilo unico |
| Dopo Annulla il taglio successivo poteva usare il vecchio schizzo | Preselezione globale rimasta attiva dopo la chiusura | Annulla ed Esc puliscono selezioni e gesti; test UI fino al successivo taglio |
| Profili coplanari di schizzi diversi selezionati in ordine arbitrario | Pareggio di profondità risolto dall'ordine degli ID | Lo schizzo scelto nel browser ha priorità; selezione sorgente anche a dialogo aperto |
| Nuovo schizzo creato alla fine anche con marker intermedio | Inserimento sempre con `push_back` | Inserimento al marker e conservazione del suffisso della timeline |
| Dialogo Estrudi ancora aperto entrando in un altro schizzo | Stati degli editor non esclusivi | Ingresso nello schizzo chiude il dialogo e le selezioni precedenti |
| Contorni successivi mostrati modificando una feature a monte | Overlay e solido valutati in contesti temporali diversi | Browser e overlay usano lo stesso contesto della feature/schizzo in modifica |

La visibilità è una scelta distinta dalla soppressione: nascondere uno schizzo
non rimuove geometria dalle feature dipendenti. `sketch.visibility` accetta `true`,
`false` o `null` (automatico), supporta undo/redo ed è persistente nel file.
`sketch.inspect` e MCP `sketch_inspect` espongono scelta ed esito effettivo.

## Flussi verificati

| Flusso | Evidenza automatizzata |
|---|---|
| Rettangolo con quote, cerchio parametrico, vincoli, spostamento punti | `sketch_solver`, `sketch_commands`, `sketch_mouse`, `sketch_interaction` |
| Schizzi multipli senza perdita di geometria e solidi visibili durante il disegno | `sketch_interaction`, pixel del viewport |
| Corona e disco selezionabili separatamente; fori preservati | `profile_regions`, `model_extrude`, `model_interaction` |
| Estrusione di uno schizzo senza nascondere quelli indipendenti | `sketch_extrude_workflows` |
| Mostra sorgente consumata, seleziona, estrudi di nuovo con Unisci | `model_interaction`, mouse e tastiera, volume verificato |
| Riutilizzo da browser anche con sorgente nascosta | `model_interaction`, `sketch_extrude_workflows` |
| Disambiguazione fra schizzi coplanari sovrapposti | `model_interaction`, selezione browser e clic viewport |
| Un lato, distanza negativa, simmetrica, due lati, attraverso tutto | `model_extrude` |
| Nuovo corpo, Unisci, Taglia, Interseca; destinazioni multiple | `model_extrude`, `model_interaction` |
| Modifica dell'estrusione e delle quote a monte | `model_timeline`, `sketch_extrude_workflows`, `model_interaction` |
| Annulla, Esc, undo/redo e ripristino della visibilità | `model_interaction`, `sketch_extrude_workflows`, test MCP |
| Rollback, inserimento al marker, soppressione e sorgenti eliminate | `model_timeline`, `model_timeline_ui`, `sketch_extrude_workflows` |
| Salvataggio e riapertura con visibilità manuale | `sketch_extrude_workflows`, `sketch_mcp` |
| Nuovo comando e ispezione via CLI e MCP | `sketch_mcp`; registro condiviso `command_run` |
| Sorgenti/quote/parametri non validi senza modifica parziale | `model_extrude`, `model_timeline`, `sketch_extrude_workflows`, fuzz |

La nuova sequenza numerica estrude un rettangolo 40×20 di 10 mm (**8000 mm³**),
riusa lo stesso profilo con Unisci fino a 15 mm (**12000 mm³**), cambia larghezza
a 60 mm (**18000 mm³**) e taglia un foro Ø4 passante (**18000 − 60π mm³**).
Nascondere lo schizzo del taglio lascia invariato quest'ultimo volume.

## Limite geometrico ancora aperto

**Contorni che si intersecano all'interno dello stesso schizzo:** il riconoscitore
attuale ricava cicli chiusi e annidamenti, ma non suddivide le curve alle loro
intersezioni. Riproduzione con due cerchi Ø20, centri (0,0) e (10,0):
`sketch.inspect` restituisce **2 regioni intere**, mentre la suddivisione del piano
ne richiede **3** (due parti esterne e lente comune). Non va confuso con due
schizzi distinti sovrapposti, disambiguati dal browser in questo intervento.

Script e risultato della riproduzione sono in `docs/examples/sketch-extrude-audit/`.
Questo difetto non è risolto dalle correzioni di visibilità. Richiede un lavoro
dedicato sui profili, con riferimenti stabili per i tratti generati; non è
classificato come funzionalità di selezione facce M5. Non si dichiara pertanto
parità completa con il riconoscimento dei profili di Fusion per contorni arbitrari.

## Evidenze visive e validazione

Screenshot del riuso e della selezione fra schizzi sovrapposti in
`docs/screenshots/sketch-extrude-audit/`, tema chiaro/scuro × 1440×900/1280×720.
Confronto con riferimenti Fusion modalità schizzo e 07–08: ordine dei campi,
operazioni e conferma/annullamento conservati; aggiunti occhi nel browser e
recupero della sorgente. Restano intenzionali icone Lucide, colori propri,
etichette italiane e scorrimento del pannello nelle finestre basse.

Build Windows Release verde, **CTest 42/42** in 24,91 s; livelli e UTF-8 verdi.
Fuzz: **218 casi diretti e 187 MCP**, nessun crash. Il test UI esteso è stato
eseguito nei quattro formati e verifica anche Esc e la chiusura esclusiva degli editor.
I dettagli della validazione sono registrati nel devlog.
Nessuna golden aggiornata. M5 non iniziato.
