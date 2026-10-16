"""Clay must stay above engine/automation, including prefixed includes."""
import runpy
from pathlib import Path

check = runpy.run_path(str(Path(__file__).resolve().parents[1] / "tools/check_layers.py"))["violations"]
for module in ("core", "doc", "solver", "kernel", "regen", "mesh", "engine", "automation"):
    for header in ("clay.h", "third_party/clay/clay.h"):
        assert check("fixture.cpp", module, f'#include <{header}>')
assert not check("fixture.cpp", "ui", '#include <clay.h>')
