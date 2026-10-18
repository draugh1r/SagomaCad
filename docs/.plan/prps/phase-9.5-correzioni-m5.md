# Fase 9.5: correzioni di comportamento M5

Fase inserita su richiesta dell’utente prima della fase 10, senza rinumerare le fasi esistenti. Milestone **M5**. La fase 10 dipende dalla verifica finale di questa fase.

## Riferimenti
- `docs/reference/fusion/40-quote-rettangolo-disegno.png`: campi separati accanto ai tratti.
- `docs/reference/fusion/41-quote-modifica.png`: modifica sopra la quota persistente.
- `docs/reference/fusion/42-estrudi-da-faccia.png`: operazione Unisci da faccia.
- Specifiche della conversazione del 18 ottobre 2026; aspetto da `docs/design/approvate/`, bianco e accento pistacchio.
- PLAN.md: M5, interfaccia, test e verifica; RULES.md: comandi, livelli, GProp, screenshot.

## Criteri di uscita
1. Preferenza “Nascondi lo schizzo dopo la creazione di una feature”, disattivata al primo avvio e persistente. Rettangolo e cerchio separati nello stesso schizzo: estrudere solo il rettangolo lascia il cerchio visibile, selezionabile ed estrudibile. Visibilità manuale, undo e rigenerazione restano coerenti.
2. Estrusione automatica: Unisci per profilo a contatto con un corpo verso l’esterno, Taglia verso l’interno, Nuovo corpo senza contatto. Distanza e direzione aggiornano l’operazione fino alla scelta esplicita dell’utente, che rimane ferma. Comandi e UI condividono la decisione geometrica. Scatola estrusa da faccia laterale: un corpo, volume/area/facce GProp coerenti e nessuna faccia interna.
3. Nessun pannello unico Larghezza/Altezza. Ogni misura di rettangolo, linea, cerchio e arco è ancorata al proprio tratto e segue la geometria; il tratto attivo usa l’accento. Tab cambia campo, Invio conferma; testo digitato crea un vincolo persistente. Doppio clic sulla quota apre il campo sulla quota stessa. Contratto comune estendibile ai futuri strumenti.

## Task (ID globali in tasks.json)
### 9.5.0 Riferimenti e regressioni
Guardare 40–42 e le tavole approvate. Individuare codice e test; registrare evidenze. Verifica: file presenti e immagini lette.
### 9.5.1 Preferenza e visibilità
Comandi di preferenza ispezionabili da CLI/MCP, persistenza locale, checkbox nelle preferenze. Non nascondere implicitamente tutti gli schizzi consumati. Test di riuso dei profili e persistenza.
### 9.5.2 Operazione di estrusione
Decisione nel motore/kernel su contatto e sovrapposizione reali; UI con stato automatico/manuale. Test su facce orientate, segno, profili isolati/a contatto, override, errori e GProp.
### 9.5.3 Quote sulla figura
Metadati di misura dagli strumenti; input separati ancorati e highlight del tratto. Editor sulla quota con doppio clic. Estendere scenari mouse/tastiera, verificare vincoli e modifica persistente.
### 9.5.4 Verifica finale
Build Release e intera suite ctest verdi; `python tools/check_layers.py`, regola frame, parità comandi e fuzz. Scenari via comandi/canale di controllo, screenshot chiaro/scuro a 1440×900 e 1280×720, confronto con 40–41 e tavole approvate. Aggiornare documentazione e devlog con numeri e limiti reali. Nessun aggiornamento automatico delle golden. Checkpoint e merge solo dopo criteri superati.
