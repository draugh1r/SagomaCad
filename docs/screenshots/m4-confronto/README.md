# Confronti M4

Ogni file `*-confronto.png` accosta il riferimento Fusion alla cattura nativa di
SagomaCad. Solo il riferimento è ridimensionato per allinearne l'altezza alla
cattura; la cattura SagomaCad rimane invariata. `tools/m4_compare.py` rigenera le
coppie dai PNG originali e dai riferimenti locali in `docs/reference/fusion/`.

Le quattro scene sono disponibili in tema chiaro e scuro a 1440×900 e 1280×720.
I test `model_interaction` e `model_timeline_ui` producono le catture con eventi
del mouse e della tastiera, oltre a verificare comandi e volumi.

| Coppia | Corrispondenze verificate | Differenze |
|---|---|---|
| 07 — Estrudi | Pannello destro sotto il cubo; Tipo, Profilo, Inizio, Direzione, Estensione, Distanza, sformo nullo, Operazione; conferma/annulla in fondo; corona selezionata, disco distinto; freccia, casella, solido semitrasparente | Testo italiano e icone Lucide; Solido, Piano profilo e sformo zero sono costanti; distanza 10 mm per mostrare il solido, rispetto allo zero del riferimento |
| 08 — Operazione | Unisci, Taglia, Interseca, Nuovo corpo nello stesso ordine; icone distinte; anteprima blu per nuovo corpo, rossa per taglio; campo Corpi con più destinazioni | Simboli Lucide e tema SagomaCad; a 720 px il menu si apre verso l'alto per restare nello schermo; cattura supplementare `08-taglia-*` con due corpi |
| 09 — Rivoluzione | Profilo, Asse, Tipo, Angolo, Direzione, Operazione; selezione dell'asse dal campo e dal viewport; asse evidenziato, anello a 360°, maniglia e valore angolare | Anteprima semitrasparente blu, asse arancione e icone Lucide; assi dell'origine e linee dello schizzo già complanari, senza il campo Proietta asse di Fusion |
| 10 — Rollback | Marker fra le icone, feature successive grigie, solo solido precedente applicato; icone distinte da 20 px; cinque controlli a sinistra | Marker blu invece di nero e simboli Lucide; etichette nei tooltip; griglia, browser e tema già propri di SagomaCad |

Le catture `timeline-error-*` mostrano l'estrusione rossa e il messaggio di schizzo
mancante, con una rivoluzione successiva ancora valida. Le differenze corrette
durante la verifica sono annotate nel devlog: triangolazione visibile nel profilo,
testo e icone sovrapposti, etichetta Rivoluzione su due righe, posizione dei pulsanti.
