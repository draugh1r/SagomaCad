# Parità della barra — M4

Generato da `tools/update_parity.py` leggendo il catalogo e interrogando il registro reale.

**22 comandi registrati, 15 mancanti** tra gli ID unici della barra. Le disponibilità contestuali (undo, redo, export) sono valutate a ogni frame; le voci mancanti sono disabilitate con il tooltip «Non ancora disponibile».

Schizzo è contestuale. I comandi di disegno aprono strumenti, le quote e i vincoli usano la selezione; Fine schizzo chiude la modalità. File, palette, timeline, CLI e MCP passano dallo stesso registro.

| Area | Voce | ID | Stato |
|---|---|---|---|
| Menu file | Nuovo | `document.new` | Registrato |
| Menu file | Apri | `document.open` | Registrato |
| Menu file | Salva | `document.save` | Registrato |
| Menu file | Salva con nome | `document.saveAs` | Registrato |
| Barra documento | Annulla | `edit.undo` | Registrato |
| Barra documento | Ripeti | `edit.redo` | Registrato |
| Solido / Crea | Schizzo | `sketch.create` | Registrato |
| Solido / Crea | Estrudi | `feature.extrude` | Registrato |
| Solido / Crea | Rivoluzione | `feature.revolve` | Registrato |
| Solido / Crea | Foro | `feature.hole` | Non ancora disponibile |
| Solido / Crea | Primitiva | `primitive.add` | Registrato |
| Solido / Modifica | Premi/tira | `face.pushPull` | Non ancora disponibile |
| Solido / Modifica | Raccordo | `feature.fillet` | Non ancora disponibile |
| Solido / Modifica | Smusso | `feature.chamfer` | Non ancora disponibile |
| Solido / Modifica | Cambia parametri | `param.list` | Registrato |
| Solido / Costruisci | Piano | `construction.plane` | Non ancora disponibile |
| Solido / Costruisci | Asse | `construction.axis` | Non ancora disponibile |
| Solido / Costruisci | Punto | `construction.point` | Non ancora disponibile |
| Solido / Ispeziona | Misura | `inspect.measure` | Non ancora disponibile |
| Solido / Ispeziona | Sezione | `inspect.section` | Non ancora disponibile |
| Solido / Inserisci | Importa mesh | `mesh.import` | Non ancora disponibile |
| Solido / Inserisci | Importa STEP | `import.step` | Non ancora disponibile |
| Solido / Seleziona | Seleziona | `selection.set` | Non ancora disponibile |
| Solido / Seleziona | Scorri | `selection.cycle` | Non ancora disponibile |
| Solido / Esporta | STL | `export.stl` | Registrato |
| Solido / Esporta | 3MF | `export.3mf` | Non ancora disponibile |
| Solido / Esporta | STEP | `export.step` | Non ancora disponibile |
| Schizzo / Crea | Linea | `sketch.line` | Registrato |
| Schizzo / Crea | Rettangolo | `sketch.rect` | Registrato |
| Schizzo / Crea | Cerchio | `sketch.circle` | Registrato |
| Schizzo / Crea | Arco | `sketch.arc` | Registrato |
| Schizzo / Modifica | Sposta punto | `sketch.point.move` | Registrato |
| Schizzo / Modifica | Cambia quota | `sketch.dimension.set` | Registrato |
| Schizzo / Vincoli | Vincolo | `sketch.constraint.add` | Registrato |
| Schizzo / Vincoli | Quota | `sketch.dimension.add` | Registrato |
| Schizzo / Vincoli | Cambia quota | `sketch.dimension.set` | Registrato |
| Schizzo / Ispeziona | Quota | `sketch.dimension.add` | Registrato |
| Schizzo / Ispeziona | Stato solver | `sketch.inspect` | Registrato |
| Schizzo / Inserisci | Importa mesh | `mesh.import` | Non ancora disponibile |
| Schizzo / Inserisci | Importa STEP | `import.step` | Non ancora disponibile |
| Schizzo / Seleziona | Seleziona | `selection.set` | Non ancora disponibile |
| Schizzo / Seleziona | Scorri | `selection.cycle` | Non ancora disponibile |
| Schizzo / Fine Schizzo | Fine schizzo | `sketch.finish` | Registrato |

## Altri comandi disponibili da CLI/MCP e palette

`body.visibility`, `document.inspect`, `feature.delete`, `feature.edit`, `feature.get`, `feature.suppress`, `model.mass_properties`, `param.delete`, `param.set`, `primitive.edit`, `sketch.edit`, `timeline.rollback`
