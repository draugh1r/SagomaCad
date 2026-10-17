# M5 — verifica locale Windows

Lavoro in corso, M6 escluso. Build dedicata: `build/agent-m5`, configurazione Release.
Nessuna esecuzione CI, push o verifica su altre piattaforme.

## Ordine di lavoro

1. Dati di picking e buffer ID/profondità; test prima dell'integrazione UI.
2. Nomi topologici, storia OCCT e regressioni a monte, **prima di modificare la UI**.
3. Schizzi su faccia e proiezioni vincolabili.
4. Estrusione diretta della faccia.
5. Raccordo e smusso multipli.
6. Foro e quote dai bordi.
7. Query/misure/selezione/etichette CLI e MCP, guida eseguibile.
8. Integrazione UI, scorciatoie e confronti 11–16.

Ogni tappa verificata termina con build Release, CTest, livelli, fuzz se
sono cambiati comandi, voce nel devlog e commit. La prima tappa è divisa
in infrastruttura e integrazione UI per rispettare il cancello dei test
topologici richiesto prima di qualsiasi modifica alla UI.

## Riferimenti esaminati

Esaminate `11-selezione-01/02/03.png` e `12`–`16` in `docs/reference/fusion/`.
11: faccia con riempimento attenuato, spigolo con tratto chiaro più spesso;
lista di profondità presso il cursore su pressione prolungata.
12: vista ortogonale alla faccia e contesto del solido, bordi proiettati distinti.
13: campo Profilo direttamente dalla faccia, freccia e valore nel viewport.
14: lista di spigoli, raggio costante, freccia sullo spigolo e casella vicina.
15: lista di spigoli e distanza uguale, selezione e anteprima contestuali.
16: posizionamento su faccia, due riferimenti, estensione e campi dimensionali.
Le icone restano Lucide.

## Stato

Infrastruttura di picking in verifica. Le tappe successive e i confronti visivi
non sono ancora completati.
