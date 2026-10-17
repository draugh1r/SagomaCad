# Esempio M5 prodotto via MCP

Aprire accettazione.sagomacad con la build agent-m5.
La guida modeling-guide.md genera il documento e il render etichette.png;
il test modeling_agents ne esegue il client MCP con handshake reale.

Scatola finale: 60×40×50 mm, R2 sui quattro spigoli verticali, foro Ø8
passante a 10 mm dai due bordi; corpo principale 117315,04440784613 mm³.
Il cilindro Ø10 alto 6 è un secondo corpo appoggiato a z=50 mm, generato
da uno schizzo riferito alla faccia superiore. Cambiare m5_altezza e
m5_larghezza rigenera appoggio, raccordi e quote del foro.

etichette.png viene dal tool render_view con labels=true.
