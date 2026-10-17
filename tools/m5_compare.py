"""Place native M5 captures beside the unchanged user-provided Fusion images."""
from pathlib import Path
import sys
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parents[1]
source = Path(sys.argv[1]) if len(sys.argv) > 1 else root / "build/agent-m5/m5-evidence"
output = root / "docs/screenshots/m5-confronto"
output.mkdir(parents=True, exist_ok=True)
scenes = {
    "11-faccia": "11-selezione-01.png",
    "11-spigoli": "11-selezione-02.png",
    "11-seleziona-altro": "11-selezione-03.png",
    "12-schizzo-su-faccia": "12-schizzo-su-faccia.png",
    "13-estrudi-faccia": "13-estrudi-faccia.png",
    "14-raccordo": "14-raccordo.png",
    "15-smusso": "15-smusso.png",
    "16-foro": "16-foro.png",
}
font = ImageFont.truetype(str(root / "assets/fonts/Inter.ttf"), 20)
count = 0
for scene, reference in scenes.items():
    for capture in sorted(source.glob(scene + "-*.png")):
        actual = Image.open(capture).convert("RGB")
        expected = Image.open(root / "docs/reference/fusion" / reference).convert("RGB")
        expected = expected.resize((round(expected.width * actual.height / expected.height), actual.height), Image.Resampling.LANCZOS)
        pair = Image.new("RGB", (expected.width + actual.width, actual.height + 40), "white")
        pair.paste(expected, (0, 40))
        pair.paste(actual, (expected.width, 40))
        draw = ImageDraw.Draw(pair)
        draw.text((12, 6), "Fusion — " + reference, font=font, fill="black")
        draw.text((expected.width + 12, 6), "SagomaCad — " + capture.stem, font=font, fill="black")
        pair.save(output / capture.name)
        count += 1
assert count == 32, f"Expected 32 comparisons, got {count}"
print(f"{count} side-by-side comparisons in {output}")
