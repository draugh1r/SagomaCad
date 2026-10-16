# Verifica M4 — 17 ottobre 2026

Sei tappe con build, test e commit separati; build Windows Release in
`build/agent-m4`. Suite finale **41/41** (44,67 s nell'ultima esecuzione),
controllo livelli e UTF-8 verdi, fuzz **212 diretti + 182 MCP**, nessun crash.
Non è stata avviata una CI remota o una suite Linux/macOS. M5 non iniziato.

| Accettazione | Verifica |
|---|---|
| Corona estrusa, profili annidati e riferimenti stabili | `profile_regions`, `model_extrude`: rettangolo 60×40 con Ø10, altezza 10 → 23214,601837 mm³ |
| Modifica larghezza e foro | 80×40 con Ø20, altezza 10 → 28858,407346 mm³; undo/redo e quote a monte verificati |
| Secondo schizzo in Taglia | Guida eseguita via MCP: ulteriore Ø5 passante; nuova altezza e quota a monte aggiornano entrambi i tagli |
| Rivoluzione XZ attorno a Z | Raggi 5/15, altezza 10, 360° → 6283,185307 mm³; anche 90°/180°, asse da linea e operazioni |
| Sorgente eliminata | `feature_get.error` indica Schizzo sorgente mancante; icona rossa e tooltip, feature indipendenti successive applicate; undo ripristina |
| Staffa a L da testo via MCP | Due fori Ø8, volume 21997,34517542563 mm³; documento e risposta in `docs/examples/m4/` |
| Incrementale | Modifica della feature conserva il prefisso; parametro invalida dal primo dipendente; parametri estranei non invalidano; rollback conserva la cache successiva |
| CLI/MCP | Eseguiti tutti i blocchi della guida, script CLI pubblicato, creazione/modifica di rivoluzione da CLI, MCP headless e bridge con app nativa |

La guida servita da `sagomacad://docs/modeling-guide` viene incorporata dallo stesso
file Markdown usato dai test: nessuna seconda copia mantenuta manualmente.

I test UI usano eventi reali di mouse e tastiera: selezione separata di corona e
disco, E, freccia, casella del viewport, quattro operazioni, due destinazioni,
Rivoluzione dal gruppo Crea, picking dell'asse, drag dell'angolo, doppio clic,
rollback e soppressione. Le anteprime non scrivono nel documento. Verificati temi
chiaro/scuro a 1440×900 e 1280×720; layout del pannello controllato anche a scala 2.

I sedici confronti 07–10 e le note sulle differenze sono in
`docs/screenshots/m4-confronto/` e nel devlog. Corrette triangolazione visibile,
menu sovrapposto, etichetta a capo e allineamento dei pulsanti. La cattura MCP
della staffa ha inoltre riprodotto triangoli posteriori dipinti su superfici
opache anteriori: corretto il backface culling, con controllo dei pixel su punti
complanari attorno al foro. Le anteprime trasparenti conservano entrambi i lati.

Nessuna golden image sostituita e nessuna app dell'utente chiusa. Eseguibili:
`build/agent-m4/apps/sagomacad/sagomacad.exe` e
`build/agent-m4/apps/sagomacad-cli/sagomacad-cli.exe`.
