# Parità dei comandi della UI — M0

Il catalogo della barra è definito in `src/ui/toolbar_catalog.cpp`. Ogni voce ha un ID presente in `docs/PLAN.md` e un'icona Lucide distinta. Una voce diventa attiva solo se il suo ID compare nel registro dei comandi. M0 non possiede ancora quel registro: **0 voci attive, 36 mancanti**. Le voci mancanti sono disabilitate e mostrano «Non ancora disponibile».

La scheda Schizzo è contestuale e compare solo quando `sketch_mode` è attivo. Le schede Superficie, Lamiera e Plastica sono state rimosse. Mesh, Gestisci e Utilità restano predisposte senza gruppi. Ogni gruppo mostra due o tre comandi principali; il menu sotto il gruppo contiene tutte le sue voci. Gli ID nuovi per le sole voci UI sono riservati nel piano, senza implementazione del comando.

| Area | Voce | ID previsto in PLAN.md | Stato M0 |
| --- | --- | --- | --- |
| Menu file | Nuovo | `document.new` | Mancante |
| Menu file | Apri | `document.open` | Mancante |
| Menu file | Salva | `document.save` | Mancante |
| Menu file | Salva con nome | `document.saveAs` | Mancante |
| Barra documento | Annulla | `edit.undo` | Mancante |
| Barra documento | Ripeti | `edit.redo` | Mancante |
| Solido / Crea | Schizzo | `sketch.create` | Mancante |
| Solido / Crea | Estrudi | `feature.extrude` | Mancante |
| Solido / Crea | Rivolvi | `feature.revolve` | Mancante |
| Solido / Crea | Foro | `feature.hole` | Mancante |
| Solido / Crea | Primitiva | `primitive.add` | Mancante |
| Solido / Modifica | Premi/tira | `face.pushPull` | Mancante |
| Solido / Modifica | Raccordo | `feature.fillet` | Mancante |
| Solido / Modifica | Smusso | `feature.chamfer` | Mancante |
| Solido / Modifica | Parametri | `param.list` | Mancante |
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
