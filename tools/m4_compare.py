"""Place unmodified SagomaCad captures beside the user-provided Fusion references."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parents[1]
output = root / "docs/screenshots/m4-confronto"
scenes = {"07-estrudi": "07-estrudi.png", "08-operazione": "08-estrudi-operazione.png",
          "09-rivoluzione": "09-rivoluzione.png", "10-timeline": "10-timeline-rollback.png"}
for scene, reference in scenes.items():
    for source in output.glob(scene + "-*.png"):
        if "confronto" in source.stem:
            continue
        actual = Image.open(source).convert("RGB")
        expected = Image.open(root / "docs/reference/fusion" / reference).convert("RGB")
        expected = expected.resize((round(expected.width * actual.height / expected.height), actual.height), Image.Resampling.LANCZOS)
        header = 40
        pair = Image.new("RGB", (expected.width + actual.width, actual.height + header), "white")
        pair.paste(expected, (0, header))
        pair.paste(actual, (expected.width, header))
        draw = ImageDraw.Draw(pair)
        font = ImageFont.truetype("C:/Windows/Fonts/segoeui.ttf", 20)
        draw.text((12, 6), "Fusion — riferimento " + reference, font=font, fill="black")
        draw.text((expected.width + 12, 6), "SagomaCad — " + source.stem, font=font, fill="black")
        target = source.with_stem(source.stem + "-confronto")
        pair.save(target)
        print(target.relative_to(root))
