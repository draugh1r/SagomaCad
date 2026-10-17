"""Every top-level ImGui window and its placement must use ui/frame.cpp."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
pattern = re.compile(r'ImGui\s*::\s*(?:Begin\s*\(|SetNextWindow(?:Pos|Size|SizeConstraints)\s*\()')
errors = []
for folder in ('src', 'apps'):
    for path in (root / folder).rglob('*'):
        if path.suffix not in ('.cpp', '.hpp', '.h') or path == root / 'src/ui/frame.cpp':
            continue
        for line, text in enumerate(path.read_text(encoding='utf-8').splitlines(), 1):
            if pattern.search(text):
                errors.append(f'{path.relative_to(root)}:{line}: use ui/frame.cpp')
if errors:
    raise SystemExit('\n'.join(errors))
print('Window factory rule passed')
