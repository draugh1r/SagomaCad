"""Import unchanged SolidCraft artwork and verify provenance, byte for byte."""
import argparse
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'assets/icons/solidcraft'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT.parent / 'solidcraft-ref')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    source = args.source.resolve()
    design = source / 'assets/icons/ui/design'
    files = {p.relative_to(design): p for p in (design / 'svg').rglob('*.svg')}
    files.update({Path(n): design / n for n in ('palette.toml', 'aliases.toml')})
    files.update({Path(n): source / n for n in ('LICENSE-MIT', 'LICENSE-APACHE')})
    if not files or not (design / 'svg/model/extrude.svg').is_file():
        parser.error('SolidCraft source missing')
    revision = subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
    for rel, src in files.items():
        dst = DEST / rel
        if args.check:
            if not dst.is_file() or dst.read_bytes() != src.read_bytes():
                raise SystemExit(f'Missing or modified import: {rel}')
        else:
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(src, dst)
    if args.check:
        if (DEST / 'SOURCE').read_text().strip() != revision:
            raise SystemExit('Source revision differs')
        if set((DEST / 'svg').rglob('*.svg')) != {DEST / f for f in files if f.suffix == '.svg'}:
            raise SystemExit('Unexpected SVG files')
    else:
        (DEST / 'SOURCE').write_text(revision + '\n', encoding='utf-8')
    print(f'SolidCraft: {len(files)-4} SVG masters, commit {revision}')

if __name__ == '__main__':
    main()
