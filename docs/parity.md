# Parità della barra — M5.5

Generato da `tools/update_parity.py` leggendo il catalogo e interrogando il registro reale.

**27 comandi registrati, 10 mancanti** tra gli ID unici della barra. Le disponibilità contestuali (undo, redo, export) sono valutate a ogni frame; le voci mancanti sono disabilitate con il tooltip «Non ancora disponibile».

Schizzo è contestuale. I comandi di disegno aprono strumenti, le quote e i vincoli usano la selezione; Fine schizzo chiude la modalità. File, palette, timeline, CLI e MCP passano dallo stesso registro.

| Area | Voce | ID | Stato | Icona |
|---|---|---|---|---|
| Menu file | Nuovo | `document.new` | Registrato | SolidCraft/new (Solid) |
| Menu file | Apri | `document.open` | Registrato | SolidCraft/open (Solid) |
| Menu file | Salva | `document.save` | Registrato | SolidCraft/save (Solid) |
| Menu file | Salva con nome | `document.saveAs` | Registrato | Lucide/file-pen-line (fallback) |
| Barra documento | Annulla | `edit.undo` | Registrato | SolidCraft/undo (Solid) |
| Barra documento | Ripeti | `edit.redo` | Registrato | SolidCraft/redo (Solid) |
| Solido / Crea | Schizzo | `sketch.create` | Registrato | SolidCraft/sketch (Sketch) |
| Solido / Crea | Estrudi | `feature.extrude` | Registrato | SolidCraft/extrude (Solid) |
| Solido / Crea | Rivoluzione | `feature.revolve` | Registrato | SolidCraft/revolve (Solid) |
| Solido / Crea | Foro | `feature.hole` | Registrato | SolidCraft/hole (Solid) |
| Solido / Crea | Primitiva | `primitive.add` | Registrato | SolidCraft/box (Solid) |
| Solido / Modifica | Premi/tira | `face.pushPull` | Non ancora disponibile | SolidCraft/presspull (Solid) |
| Solido / Modifica | Raccordo | `feature.fillet` | Registrato | SolidCraft/fillet (Solid) |
| Solido / Modifica | Smusso | `feature.chamfer` | Registrato | SolidCraft/chamfer (Solid) |
| Solido / Modifica | Cambia parametri | `param.list` | Registrato | SolidCraft/params (Solid) |
| Solido / Costruisci | Piano | `construction.plane` | Non ancora disponibile | SolidCraft/plane (Construction) |
| Solido / Costruisci | Asse | `construction.axis` | Non ancora disponibile | SolidCraft/axis (Construction) |
| Solido / Costruisci | Punto | `construction.point` | Non ancora disponibile | SolidCraft/point (Construction) |
| Solido / Ispeziona | Misura | `inspect.measure` | Registrato | SolidCraft/measure (Solid) |
| Solido / Ispeziona | Sezione | `inspect.section` | Non ancora disponibile | SolidCraft/section (Solid) |
| Solido / Inserisci | Importa mesh | `mesh.import` | Non ancora disponibile | SolidCraft/import (Mesh) |
| Solido / Inserisci | Importa STEP | `import.step` | Non ancora disponibile | SolidCraft/import (Solid) |
| Solido / Seleziona | Seleziona | `selection.set` | Registrato | SolidCraft/select (Solid) |
| Solido / Seleziona | Scorri | `selection.cycle` | Non ancora disponibile | Lucide/mouse-pointer-2 (fallback) |
| Solido / Esporta | STL | `export.stl` | Registrato | SolidCraft/export (Mesh) |
| Solido / Esporta | 3MF | `export.3mf` | Non ancora disponibile | SolidCraft/export (Mesh) |
| Solido / Esporta | STEP | `export.step` | Non ancora disponibile | SolidCraft/export (Solid) |
| Schizzo / Crea | Linea | `sketch.line` | Registrato | SolidCraft/line (Sketch) |
| Schizzo / Crea | Rettangolo | `sketch.rect` | Registrato | SolidCraft/rect (Sketch) |
| Schizzo / Crea | Cerchio | `sketch.circle` | Registrato | SolidCraft/circle (Sketch) |
| Schizzo / Crea | Arco | `sketch.arc` | Registrato | SolidCraft/arc3 (Sketch) |
| Schizzo / Modifica | Sposta punto | `sketch.point.move` | Registrato | SolidCraft/sketch_move (Sketch) |
| Schizzo / Modifica | Cambia quota | `sketch.dimension.set` | Registrato | SolidCraft/dimension (Sketch) |
| Schizzo / Vincoli | Vincolo | `sketch.constraint.add` | Registrato | SolidCraft/constrainer (Constraint) |
| Schizzo / Vincoli | Quota | `sketch.dimension.add` | Registrato | SolidCraft/dimension (Sketch) |
| Schizzo / Vincoli | Cambia quota | `sketch.dimension.set` | Registrato | SolidCraft/dimension (Sketch) |
| Schizzo / Ispeziona | Quota | `sketch.dimension.add` | Registrato | SolidCraft/dimension (Sketch) |
| Schizzo / Ispeziona | Stato solver | `sketch.inspect` | Registrato | SolidCraft/inspect (Sketch) |
| Schizzo / Inserisci | Importa mesh | `mesh.import` | Non ancora disponibile | SolidCraft/import (Mesh) |
| Schizzo / Inserisci | Importa STEP | `import.step` | Non ancora disponibile | SolidCraft/import (Solid) |
| Schizzo / Seleziona | Seleziona | `selection.set` | Registrato | SolidCraft/select (Solid) |
| Schizzo / Seleziona | Scorri | `selection.cycle` | Non ancora disponibile | Lucide/mouse-pointer-2 (fallback) |
| Schizzo / Fine Schizzo | Fine schizzo | `sketch.finish` | Registrato | SolidCraft/finish (Sketch) |

## Altri comandi disponibili da CLI/MCP e palette

`body.visibility`, `document.inspect`, `feature.delete`, `feature.edit`, `feature.extrude.suggest`, `feature.get`, `feature.suppress`, `geometry.query`, `model.mass_properties`, `param.delete`, `param.set`, `preferences.get`, `preferences.set`, `primitive.edit`, `selection.get`, `sketch.edit`, `sketch.visibility`, `timeline.rollback`
