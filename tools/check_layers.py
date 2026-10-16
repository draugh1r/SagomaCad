#!/usr/bin/env python3
"""Enforce module include boundaries without requiring a configured build."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
LEVEL = {"core": 0, "doc": 1, "solver": 2, "kernel": 2,
         "regen": 3, "mesh": 3, "engine": 4, "automation": 5, "ui": 6}
PATTERN = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)
OCCT = re.compile(r'^(?:AIS_|BRep|Bnd_|Graphic3d_|Geom|GProp|gp_|IGESControl|Interface_|NCollection|Precision|Prs3d_|Shape|Standard_|STEPControl|TCol|TDF_|TDoc|TPrsStd_|TopAbs|TopExp|TopLoc|TopoDS|TopTools|V3d_)')
UI_DEPS = re.compile(r'^(?:imgui|SDL|GL/|OpenGL/|glad|GLFW/)|(?:^|/)clay\.h$')

def violations(source, module, text):
    errors = []
    for inc in PATTERN.findall(text):
        target = inc.split("/")[0]
        if target in LEVEL and target != module and LEVEL[target] >= LEVEL[module]:
            errors.append(f"{source}: {module} cannot include same or higher level {target}/{inc}")
        if OCCT.match(inc) and module != "kernel":
            errors.append(f"{source}: OCCT header outside kernel/: {inc}")
        if UI_DEPS.search(inc) and module != "ui":
            errors.append(f"{source}: UI dependency below ui/: {inc}")
    return errors

errors = []
for source in (ROOT / "src").rglob("*"):
    if source.suffix not in {".hpp", ".h", ".cpp", ".cc", ".c"}:
        continue
    module = source.relative_to(ROOT / "src").parts[0]
    errors.extend(violations(source, module, source.read_text(encoding="utf-8")))
if errors:
    print("\n".join(errors), file=sys.stderr)
    sys.exit(1)
print("Layer check passed")
