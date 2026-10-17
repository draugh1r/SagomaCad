"""Face support is reachable through MCP command_run and the headless CLI."""
import json
import subprocess
import sys
from test_mcp_integration import Client

cli = sys.argv[1]
steps = [
    ("primitive.add", {"kind": "box", "dimensions": {"width": "40 mm", "depth": "40 mm", "height": "20 mm"}}),
    ("sketch.create", {"face": {"body": "body-1", "name": "primitive-1/z_max"}}),
    ("sketch.circle", {"x": 10, "y": 10, "diameter": "10 mm"}),
    ("feature.extrude", {"sketch": "sketch-2", "profiles": [["e1"]], "distance": "6 mm"}),
    ("feature.edit", {"feature_id": "primitive-1", "dimensions": {"height": "35 mm"}}),
    ("sketch.inspect", {"sketch": "sketch-2"}),
    ("model.mass_properties", {"body": "body-3"}),
    ("feature.extrude", {"face": {"body": "body-1", "name": "primitive-1/z_max"}, "distance": "4 mm"}),
    ("feature.edit", {"feature_id": "primitive-1", "dimensions": {"height": "40 mm"}}),
    ("model.mass_properties", {"body": "body-4"}),
]
client = Client([cli, "mcp", "--headless"])
try:
    for command, params in steps:
        result = client.ok("command_run", {"command": command, "params": params})
        if command == "sketch.inspect":
            assert len(result["entities"]) == 5 and result["origin"][2] == 35, result
            assert all(e["construction"] for e in result["entities"] if "reference_name" in e)
    assert abs(result["bbox_mm"]["min"]["z"] - 40) < 1e-5, result
finally:
    client.close()
script = [["engine.execute", {"command": c, "params": p}] for c, p in steps]
completed = subprocess.run([cli, "run", "--script", json.dumps(script)], capture_output=True, text=True, encoding="utf-8", timeout=60)
assert completed.returncode == 0, completed.stderr + completed.stdout
reply = json.loads(completed.stdout)
assert abs(reply["results"][-1]["result"]["bbox_mm"]["min"]["z"] - 40) < 1e-5, reply
print("Face support, projection, upstream change and cylinder position through MCP and CLI OK")
