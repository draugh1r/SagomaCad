"""Generate toolbar parity from the C++ catalog and the real command registry."""
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/ui/toolbar_catalog.cpp").read_text(encoding="utf-8")
commands = {}
for name, body in re.findall(r"constexpr ToolbarCommand (\w+)\[\] = \{(.*?)\};", source, re.S):
    commands[name] = re.findall(r'\{"([^"]+)",\s*"([^"]+)",\s*"([^"]+)"\}', body)
groups = {}
for name, body in re.findall(r"constexpr ToolbarGroup (\w+)\[\] = \{(.*?)\};", source, re.S):
    groups[name] = re.findall(r'\{"([^"]+)",\s*(\w+),\s*\d+\}', body)
with tempfile.TemporaryDirectory() as folder:
    reply = subprocess.run([sys.argv[1], "call", "engine.commands", "--headless",
                            str(Path(folder) / "parity.sagomacad")], check=True,
                           capture_output=True, text=True, encoding="utf-8")
    registry = {c["id"]: c for c in json.loads(reply.stdout)["result"]}
rows = []
def add(area, entries):
    for command, label, icon in entries:
        rows.append((area, label, command, "Registrato" if command in registry else "Non ancora disponibile"))
add("Menu file", commands["file_menu"])
add("Barra documento", commands["edit_actions"])
for tab, contextual, group in re.findall(r'\{"([^"]+)",\s*(true|false),\s*(\w+)\}', source):
    for name, entries in groups.get(group, []):
        add(tab.title() + " / " + name.title(), commands[entries])
ids = {r[2] for r in rows}
active = len(ids & registry.keys())
output = ["# Parità della barra — M5", "",
          "Generato da `tools/update_parity.py` leggendo il catalogo e interrogando il registro reale.", "",
          f"**{active} comandi registrati, {len(ids)-active} mancanti** tra gli ID unici della barra. Le disponibilità contestuali (undo, redo, export) sono valutate a ogni frame; le voci mancanti sono disabilitate con il tooltip «Non ancora disponibile».", "",
          "Schizzo è contestuale. I comandi di disegno aprono strumenti, le quote e i vincoli usano la selezione; Fine schizzo chiude la modalità. File, palette, timeline, CLI e MCP passano dallo stesso registro.", "",
          "| Area | Voce | ID | Stato |", "|---|---|---|---|"]
output += [f"| {area} | {label} | `{command}` | {state} |" for area, label, command, state in rows]
output += ["", "## Altri comandi disponibili da CLI/MCP e palette", "",
           ", ".join(f"`{name}`" for name in sorted(registry.keys()-ids)), ""]
(root / "docs/parity.md").write_text("\n".join(output), encoding="utf-8")
print(f"Parity: {active} registered, {len(ids)-active} missing")
