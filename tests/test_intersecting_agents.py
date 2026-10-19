"""Exact intersecting regions and persistent references through CLI and MCP."""
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile
from test_mcp_integration import Client

cli = str(Path(sys.argv[1]).resolve())


def lens(a, b, d=10):
    return a*a*math.acos((d*d+a*a-b*b)/(2*d*a)) + b*b*math.acos((d*d+b*b-a*a)/(2*d*b)) - .5*math.sqrt((-d+a+b)*(d+a-b)*(d-a+b)*(d+a+b))


def exercise(command):
    command("document.new", {})
    command("param.set", {"name": "diameter", "expression": "20 mm"})
    sketch = command("sketch.create", {"plane": "XY"})["sketch"]
    command("sketch.circle", {"sketch": sketch, "diameter": "diameter"})
    command("sketch.circle", {"sketch": sketch, "x": 10, "diameter": "20 mm"})
    regions = command("sketch.inspect", {"sketch": sketch})["regions"]
    assert len(regions) == 3, regions
    features = [command("feature.extrude", {"sketch": sketch, "profiles": [region["id"]], "distance": "5 mm", "operation": "new"}) for region in regions]
    for diameter in (20, 22):
        command("param.set", {"name": "diameter", "expression": f"{diameter} mm"})
        r = diameter/2
        expected = sorted([5*lens(r, 10), 5*(math.pi*r*r-lens(r, 10)), 5*(100*math.pi-lens(r, 10))])
        actual = sorted(command("model.mass_properties", {"body": feature["body_id"]})["volume_mm3"] for feature in features)
        assert all(abs(a-b) < 1e-5 for a, b in zip(actual, expected)), (actual, expected)
        assert {region["id"] for region in command("sketch.inspect", {"sketch": sketch})["regions"]} == {region["id"] for region in regions}
        for region, feature in zip(regions, features):
            inspected = command("feature.get", {"feature_id": feature["feature_id"]})
            assert inspected["params"]["profiles"] == [region["id"]] and not inspected["error"], inspected
    return sorted(region["id"] for region in regions)


client = Client([cli, "mcp", "--headless"])
try:
    mcp = exercise(lambda name, params: client.ok("command_run", {"command": name, "params": params}))
finally:
    client.close()
with tempfile.TemporaryDirectory() as directory:
    document = Path(directory) / "regions.sagomacad"

    def call(name, params):
        result = subprocess.run([cli, "call", "engine.execute", json.dumps({"command": name, "params": params}), "--headless", str(document)], capture_output=True, text=True, encoding="utf-8", timeout=30)
        assert result.returncode == 0, result.stderr + result.stdout
        return json.loads(result.stdout)["result"]

    assert exercise(call) == mcp
print("CLI/MCP: three regions, exact volumes, stable IDs and regenerated solids after radius edit OK")
