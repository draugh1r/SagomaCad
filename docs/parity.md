# Parità dei comandi della UI — M1

Il catalogo della barra è definito in `src/ui/toolbar_catalog.cpp`. Ogni voce ha un ID presente in `docs/PLAN.md` e un'icona Lucide distinta. Lo stato di ciascuna voce viene dal registro dei comandi: **7 voci registrate, 29 mancanti**. Le voci mancanti sono disabilitate e mostrano «Non ancora disponibile». Annulla e Ripeti sono registrati ma si abilitano solo quando la storia contiene la rispettiva operazione.

La palette (S o Ctrl+K) mostra inoltre i tre comandi registrati che non hanno una voce dedicata nella barra: `document.inspect`, `param.set` e `param.delete`. `param.list` apre il pannello Modifica > Cambia parametri, dove si possono impostare ed eliminare parametri.

La scheda Schizzo è contestuale e compare solo quando `sketch_mode` è attivo. Le schede Superficie, Lamiera e Plastica sono state rimosse. Mesh, Gestisci e Utilità restano predisposte senza gruppi. Ogni gruppo mostra due o tre comandi principali; il menu sotto il gruppo contiene tutte le sue voci. Gli ID nuovi per le sole voci UI sono riservati nel piano, senza implementazione del comando.

| Area | Voce | ID previsto in PLAN.md | Stato M1 |
| --- | --- | --- | --- |
| Menu file | Nuovo | `document.new` | Attivo |
| Menu file | Apri | `document.open` | Attivo |
| Menu file | Salva | `document.save` | Attivo |
| Menu file | Salva con nome | `document.saveAs` | Attivo |
| Barra documento | Annulla | `edit.undo` | Contestuale |
| Barra documento | Ripeti | `edit.redo` | Contestuale |
| Solido / Crea | Schizzo | `sketch.create` | Mancante |
| Solido / Crea | Estrudi | `feature.extrude` | Mancante |
| Solido / Crea | Rivolvi | `feature.revolve` | Mancante |
| Solido / Crea | Foro | `feature.hole` | Mancante |
| Solido / Crea | Primitiva | `primitive.add` | Mancante |
| Solido / Modifica | Premi/tira | `face.pushPull` | Mancante |
| Solido / Modifica | Raccordo | `feature.fillet` | Mancante |
| Solido / Modifica | Smusso | `feature.chamfer` | Mancante |
| Solido / Modifica | Cambia parametri | `param.list` | Attivo |
| Solido / Costruisci | Piano | `construction.plane` | Mancante |
| Solido / Costruisci | Asse | `construction.axis` | Mancante |
| Solido / Costruisci | Punto | `construction.point` | Mancante |
| Solido / Ispeziona | Misura | `inspect.measure` | Mancante |
| Solido / Ispeziona | Sezione | `inspect.section` | Mancante |
| Solido / Inserisci | Importa mesh | `mesh.import` | Mancante |
| Solido / Inserisci | Importa STEP | `import.step` | Mancante |
| Solido / Seleziona | Seleziona | `selection.set` | Mancante |
| Solido / Seleziona | Scorri | `selection.cycle` | Mancante |
| Solido / Esporta | STL | `export.stl` | Mancante |
| Solido / Esporta | 3MF | `export.3mf` | Mancante |
| Solido / Esporta | STEP | `export.step` | Mancante |
| Schizzo / Disegna | Linea | `sketch.line` | Mancante |
| Schizzo / Disegna | Rettangolo | `sketch.rect` | Mancante |
| Schizzo / Disegna | Cerchio | `sketch.circle` | Mancante |
| Schizzo / Vincoli | Vincolo | `sketch.constraint.add` | Mancante |
| Schizzo / Vincoli | Quota | `sketch.dimension.add` | Mancante |
| Schizzo / Vincoli | Cambia quota | `sketch.dimension.set` | Mancante |
| Schizzo / Gestisci | Arco | `sketch.arc` | Mancante |
| Schizzo / Gestisci | Modifica | `sketch.edit` | Mancante |
| Schizzo / Gestisci | Termina | `sketch.finish` | Mancante |
