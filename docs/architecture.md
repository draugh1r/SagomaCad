# Architettura M0

I target `sagomacad_core`, `sagomacad_doc`, `sagomacad_solver`, `sagomacad_regen`, `sagomacad_mesh`, `sagomacad_engine`, `sagomacad_automation` e `sagomacad_ui` sono librerie `INTERFACE`. `sagomacad_kernel` è una libreria statica collegata a OpenCascade tramite vcpkg. In M0 contiene soltanto una prova minima: crea un box e ne calcola il volume con GProp. `sagomacad_shell` contiene la shell nativa condivisa dai due eseguibili: finestra, rendering Dear ImGui/OpenGL e server di controllo.

`tools/check_layers.py` controlla gli include in `src/`. Il rendering nativo è confinato nella shell di `apps/common`, mentre `src/ui` fornisce esclusivamente stato e token. Questo mantiene la futura UI sotto `src/ui` indipendente dalle librerie grafiche.
