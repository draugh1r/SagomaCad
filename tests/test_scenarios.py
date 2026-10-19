"""Scenario diagnostics and bounded hostile input, through the actual CLI."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def main(cli):
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "invalid.json"
        cases = [
            ({"steps": [{"pending": "unfinished assertion"}]}, "step 1"),
            ({"steps": [{"expect": {"bodies": 1}}]}, "expected 1, got 0"),
            ({"steps": [{"until": {"expect": {"bodies": 1}, "frames": 2}}]}, "timed out"),
            ({"steps": [{"shot": "../escape.png"}]}, "cannot escape"),
            ({"steps": [{"call": {"method": "missing"}}]}, "Unknown UI method"),
            ({"steps": [{"drag": {"from": [400, 400], "to": [450, 450], "frames": 1000000}}]}, "outside"),
            ({"size": [2**64, 900], "steps": [{"note": "bad size"}]}, "Invalid size"),
            ({"steps": [{"note": "a", "key": "Enter"}]}, "exactly one"),
        ]
        for spec, message in cases:
            path.write_text(json.dumps(spec), encoding="utf-8")
            result = subprocess.run([cli, "scenario", "run", str(path)], capture_output=True, text=True, timeout=30, cwd=directory)
            assert result.returncode == 1, result.stdout + result.stderr
            assert message in result.stderr, result.stderr
    print("Scenario errors: precise step, numeric mismatch, timeout, path, unknown method and bounds OK")


if __name__ == "__main__":
    main(sys.argv[1])
