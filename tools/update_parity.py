"""Generate toolbar parity from the C++ catalog and the real command registry."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile
import tomllib

root = Path(__file__).resolve().parents[1]
source = (root / "src/ui/toolbar_catalog.cpp").read_text(encoding="utf-8")
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('executable', nargs='?', type=Path)
parser.add_argument('--check', action='store_true')
args = parser.parse_args()
cli = args.executable
if cli is None:
    candidates = [root / p for p in ('build/agent-phase9/apps/sagomacad-cli/sagomacad-cli.exe', 'build/apps/sagomacad-cli/sagomacad-cli.exe', 'build/apps/sagomacad-cli/sagomacad-cli')]
    cli = next((p for p in candidates if p.is_file()), None)
if cli is None:
    parser.error('Build the CLI or provide its path')
commands = {}
for name, body in re.findall(r"constexpr ToolbarCommand (\w+)\[\] = \{(.*?)\};", source, re.S):
    commands[name] = re.findall(r'\{"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]*)",\s*IconAccent::(\w+)\}', body)
groups = {}
for name, body in re.findall(r"constexpr ToolbarGroup (\w+)\[\] = \{(.*?)\};", source, re.S):
    groups[name] = re.findall(r'\{"([^"]+)",\s*(\w+),\s*\d+\}', body)
with tempfile.TemporaryDirectory() as folder:
    reply = subprocess.run([str(cli.resolve()), "call", "engine.commands", "--headless",
                            str(Path(folder) / "parity.sagomacad")], check=True,
                           capture_output=True, text=True, encoding="utf-8")
    registry = {c["id"]: c for c in json.loads(reply.stdout)["result"]}
folder = root / 'assets/icons/solidcraft'
aliases = tomllib.loads((folder / 'aliases.toml').read_text(encoding='utf-8'))['alias']
names = {p.stem for p in (folder / 'svg').rglob('*.svg')} | aliases.keys()
names |= {p.stem for p in (root / 'assets/icons/sagomacad/svg').rglob('*.svg')}
rows = []
def add(area, entries):
    for command, label, icon, design_icon, accent in entries:
        if design_icon:
            if design_icon not in names:
                raise SystemExit(f'Unknown SolidCraft icon: {design_icon}')
        elif not (root / 'assets/icons/lucide' / (icon + '.svg')).is_file():
            raise SystemExit(f'Unknown Lucide fallback: {icon}')
        artwork = f'SolidCraft/{design_icon} ({accent})' if design_icon else f'Lucide/{icon} (fallback)'
        rows.append((area, label, command, "Registrato" if command in registry else "Non ancora disponibile", artwork))
add("Menu file", commands["file_menu"])
add("Barra documento", commands["edit_actions"])
for tab, contextual, group in re.findall(r'\{"([^"]+)",\s*(true|false),\s*(\w+)\}', source):
    for name, entries in groups.get(group, []):
        add(tab.title() + " / " + name.title(), commands[entries])
ids = {r[2] for r in rows}
active = len(ids & registry.keys())
output = ["# Parità della barra — M5.5", "",
          "Generato da `tools/update_parity.py` leggendo il catalogo e interrogando il registro reale.", "",
          f"**{active} comandi registrati, {len(ids)-active} mancanti** tra gli ID unici della barra. Le disponibilità contestuali (undo, redo, export) sono valutate a ogni frame; le voci mancanti sono disabilitate con il tooltip «Non ancora disponibile».", "",
          "Schizzo è contestuale. I comandi di disegno aprono strumenti, le quote e i vincoli usano la selezione; Fine schizzo chiude la modalità. File, palette, timeline, CLI e MCP passano dallo stesso registro.", "",
          "| Area | Voce | ID | Stato | Icona |", "|---|---|---|---|---|"]
output += [f"| {area} | {label} | `{command}` | {state} | {artwork} |" for area, label, command, state, artwork in rows]
output += ["", "## Altri comandi disponibili da CLI/MCP e palette", "",
           ", ".join(f"`{name}`" for name in sorted(registry.keys()-ids)), ""]
rendered = "\n".join(output)
if args.check:
    if (root / "docs/parity.md").read_text(encoding="utf-8") != rendered:
        raise SystemExit('Parity document stale: run tools/update_parity.py')
else:
    (root / "docs/parity.md").write_text(rendered, encoding="utf-8")
print(f"Parity: {active} registered, {len(ids)-active} missing")
