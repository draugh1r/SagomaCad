"""All text sources and project documentation must be valid UTF-8."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
paths = [root / name for name in ("CMakeLists.txt", "vcpkg.json")]
for folder in ("src", "apps", "tests", "tools", "docs", "log", ".github"):
    paths.extend(p for p in (root / folder).rglob("*")
                 if p.is_file() and p.suffix in {".cpp", ".hpp", ".h", ".py", ".md", ".yml", ".yaml", ".json"})
for path in paths:
    path.read_text(encoding="utf-8", errors="strict")
print(f"UTF-8 verified: {len(paths)} files")
