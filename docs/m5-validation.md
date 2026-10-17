# M5 — accettazione locale Windows

Completato il 17 ottobre 2026, senza iniziare M6.
Build dedicata: `build/agent-m5`, configurazione **Release**.

## Verifiche

- Build Release verde; **CTest 51/51 in 35,64 s**.
- Controlli dei livelli e UTF-8 verdi.
- Fuzz: **260 casi diretti e 222 MCP**, nessun crash; 42 comandi.
- UI verificata a **1440×900 e 1280×720**, temi chiaro e scuro.
- Nessuna CI, nessun push, nessuna golden sostituita.

## Tappe e commit

| Tappa | Commit | Verifica |
|---|---|---|
| 1a — ID e buffer software ID/profondità | d7b5e65 | 43 test |
| 2a — nomi e storia OCCT | b452c1f | 44 test |
| 3a — schizzi su faccia e proiezioni | 03da338 | 46 test |
| 4a — estrusione diretta della faccia | ca70c3b | 47 test |
| 5a — raccordo e smusso | 6fbc8b7 | 48 test |
| 6a — foro e quote dai bordi | c256397 | 49 test |
| 7a — strumenti agenti | afe992b | 50 test |
| Integrazione UI delle tappe 1/3–6, etichette e scorciatoie 8 | commit finale | 51 test |

La prima parte di ogni feature è stata verificata nel motore prima della UI.
Il cancello topologico completo è stato superato al commit c256397.

## Accettazione numerica

| Caso | Risultato |
|---|---|
| Schizzo sulla sommità, cilindro Ø10 alto 6, scatola portata a z=35 | z_min cilindro = 35 mm; volume = 150π mm³; persistenza verificata |
| R3 sui quattro spigoli verticali; scatola 60×40×35 | 10 facce, volume **83729,60168588077 mm³** |
| Foro Ø8 passante a 10 mm dai due bordi; scatola 60×40×35 | centro (10,10), 7 facce, volume **82240,70811398972 mm³** |
| Foro Ø8 cieco profondo 6 | 8 facce; volume 84000 − 96π mm³ |
| Quota del foro guidata da espressione | passa da 10 a 12 mm; l'altra resta a 10 |
| Raccordo impossibile R1000 | errore con nomi e ID degli spigoli; conserva il corpo precedente; ripristino a R3 riuscito |
| Faccia superiore eliminata da un taglio a monte | errore «Entità eliminata», nessun riaggancio alla nuova sommità, regen successiva prosegue |
| Faccia divisa in due | riferimento ambiguo rifiutato |
| Rivoluzione cava | due facce cilindriche con nomi distinti, risolte dopo cambio della quota |

L'esempio [accettazione.sagomacad](examples/m5/accettazione.sagomacad), prodotto
interamente via MCP dalla guida eseguibile, contiene una scatola 60×40×50,
R2 sui quattro spigoli verticali, foro Ø8 passante a 10 mm dai bordi e un cilindro
Ø10 alto 6 appoggiato sulla sommità come corpo distinto. Il corpo principale
misura **117315,04440784613 mm³**. Il [render con etichette](examples/m5/etichette.png)
mostra gli stessi nomi restituiti da geometry_query.

## Picking e UI

Ogni triangolo porta l'ID OCCT della faccia, ogni polilinea quello dello spigolo.
Buffer software ID/profondità con interpolazione prospettica, occlusione,
Ctrl/Shift, query in profondità, pressione prolungata e filtri persistenti.
I profili di schizzo mostrati esplicitamente mantengono la loro selezionabilità.

Schizzo su faccia: camera allineata, frame rivalutato, bordi viola immutabili
utilizzabili nei vincoli. Estrudi accetta la faccia come profilo e conserva
opzioni, freccia, campo nel viewport e anteprima. Raccordo/smusso hanno lista
multipla, espressione, anteprima e maniglia. Foro: H, un clic sulla faccia,
due quote trascinabili e modificabili, diametro, profondità e passante.
Le anteprime operano su copie; solo OK scrive il comando e l'undo.

Gli alias compatti derivano dai nomi semantici, non dall'ordine OCCT; i nomi
originari restano alias validi. I lati delle estrusioni e delle rivoluzioni
conservano l'entità dello schizzo. Una firma geometrica di riserva deve essere
unica e avere un'origine ancora disponibile; entità eliminate e divisioni
ambigue producono errore.

## Confronti visivi

[32 coppie affiancate](screenshots/m5-confronto/) per le otto scene 11a/11b/11c,
12, 13, 14, 15, 16, nei quattro formati. Il devlog riporta una riga per coppia.
Le immagini Fusion restano inalterate. Rigenerazione:

```powershell
build/agent-m5/Release/m5_interaction_test.exe build/agent-m5/m5-evidence
python tools/m5_compare.py
```

Differenze documentate: icone Lucide, italiano, temi e cornice SagomaCad;
cerchio Ø10 per il caso dello schizzo, faccia superiore intera per Estrudi,
quattro spigoli verticali R3/D3 e foro Ø8 passante a quota 10. I pannelli
espongono i campi richiesti da M5; i controlli avanzati di Fusion nelle immagini
non costituiscono nuove modalità geometriche in questo traguardo.

## Riproduzione per agenti

Guida incorporata MCP e verificata dai test: [modeling-guide.md](modeling-guide.md).
Tool: geometry_query, measure, selection_get (solo bridge), render_view con
labels=true. Stessi comandi nel registro CLI. Test reali sia headless sia
bridge, inclusi nomi disegnati e selezione della finestra.

Binari: `build/agent-m5/apps/sagomacad/sagomacad.exe` e
`build/agent-m5/apps/sagomacad-cli/sagomacad-cli.exe`.
Le istanze già aperte continuano a usare il loro eseguibile precedente.
