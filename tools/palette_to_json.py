"""Compile TOML and unchanged SVG masters into JSON for the C++ icon renderer.

GAP is an eraser, as in SolidCraft icon_svg.rs: each knockout masks only preceding
siblings. Later foreground paint remains untouched. No background colour is used.
"""
import argparse
import copy
import json
from pathlib import Path
import tomllib
import xml.etree.ElementTree as ET

NS = 'http://www.w3.org/2000/svg'
ET.register_namespace('', NS)

def knockouts(text, gap):
    root = ET.fromstring(text)
    defs = ET.Element(f'{{{NS}}}defs')
    counter = [0]
    def visit(parent):
        children = list(parent)
        parent[:] = []
        for node in children:
            tag = node.tag.rsplit('}', 1)[-1]
            if tag in ('defs', 'mask', 'clipPath'):
                parent.append(node)
                continue
            if tag == 'g':
                visit(node)
            if gap in (node.get('fill'), node.get('stroke')):
                counter[0] += 1
                name = f'sagoma_gap_{counter[0]}'
                mask = ET.SubElement(defs, f'{{{NS}}}mask', id=name,
                                     maskUnits='userSpaceOnUse', x='0', y='0', width='64', height='64')
                ET.SubElement(mask, f'{{{NS}}}path', d='M0 0H64V64H0Z', fill='white')
                eraser = copy.deepcopy(node)
                for paint in ('fill', 'stroke'):
                    eraser.set(paint, 'black' if node.get(paint) == gap else 'none')
                mask.append(eraser)
                group = ET.Element(f'{{{NS}}}g', mask=f'url(#{name})')
                group[:] = list(parent)
                parent[:] = [group]
                # A mixed fill/stroke shape can also paint a foreground component.
                for paint in ('fill', 'stroke'):
                    if node.get(paint) == gap:
                        node.set(paint, 'none')
            parent.append(node)
    visit(root)
    root.insert(0, defs)
    return ET.tostring(root, encoding='unicode')

def compile_data(folder):
    palette = tomllib.loads((folder / 'palette.toml').read_text(encoding='utf-8'))
    aliases = tomllib.loads((folder / 'aliases.toml').read_text(encoding='utf-8'))['alias']
    masters = {}
    for path in sorted((folder / 'svg').rglob('*.svg')):
        masters[path.relative_to(folder / 'svg').as_posix()] = knockouts(
            path.read_text(encoding='utf-8'), palette['roles']['gap']['source'])
    return dict(palette=palette, aliases=aliases, masters=masters)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    folder = Path(__file__).resolve().parents[1] / 'assets/icons/solidcraft'
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(compile_data(folder), ensure_ascii=False), encoding='utf-8')
